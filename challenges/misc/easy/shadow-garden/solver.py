#!/usr/bin/env python3
"""
WPCTF Shadow Garden Solver - QR Code Extraction from Audio/Video
============================================================
This solver extracts hidden QR codes from:
1. Audio track (high-frequency spectral analysis)
2. Video frames (XOR-decoded with rolling noise pattern)
"""

import cv2
import numpy as np
import subprocess
import librosa
from pyzbar.pyzbar import decode
from PIL import Image
from collections import Counter
import sys
import warnings
import time
import os
import io
import tempfile
import contextlib
import argparse
from pathlib import Path

# ============================================================================
# Configuration & Setup
# ============================================================================

# Suppress all warnings and logs
warnings.filterwarnings("ignore")
os.environ["OPENCV_LOG_LEVEL"] = "SILENT"
os.environ["PYTHONWARNINGS"] = "ignore"

try:
    cv2.utils.logging.setLogLevel(0)
except AttributeError:
    try:
        cv2.setLogLevel(0)
    except AttributeError:
        pass

class DevNull:
    """Null device for suppressing output."""
    def write(self, msg): pass
    def flush(self): pass

sys.stderr = DevNull()

@contextlib.contextmanager
def suppress_stderr():
    """Redirect low-level stderr (used by zbar) to /dev/null temporarily."""
    with open(os.devnull, 'w') as devnull:
        old_stderr_fd = os.dup(2)
        os.dup2(devnull.fileno(), 2)
        try:
            yield
        finally:
            os.dup2(old_stderr_fd, 2)
            os.close(old_stderr_fd)

# ============================================================================
# Constants
# ============================================================================

SEED = 0 #metadata comment="S33d 0.O p3 ratio Nx0r"
FRAMES_PER_QR = 50 #Up to the user
FRAME_DIFF_THRESHOLD = 30.0 #Up to the user
MIN_STABLE_FRAMES = 3 #Up to the user
AUDIO_THRESHOLD_HZ = 18000 #Up to the user

# ============================================================================
# Audio Processing Functions
# ============================================================================

def extract_audio_bytes(mkv_file):
    """
    Extract audio track from video file using FFmpeg.
    
    Args:
        mkv_file: Path to input video file
        
    Returns:
        BytesIO object containing WAV data
    """
    print("[*] Extracting audio track with FFmpeg...")
    
    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tmp:
        tmp_path = tmp.name
    
    cmd = [
        "ffmpeg", "-y", "-i", mkv_file, "-vn",
        "-acodec", "pcm_s16le", "-ar", "44100", "-ac", "1", tmp_path
    ]
    
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    
    with open(tmp_path, "rb") as f:
        wav_data = f.read()
    os.remove(tmp_path)
    
    return io.BytesIO(wav_data)

def detect_max_frequency(y, sr, hop_length=512):
    """
    Detect high-frequency anomalies in audio signal.
    
    Args:
        y: Audio time series
        sr: Sample rate
        hop_length: STFT hop length
        
    Returns:
        Tuple of (start_sample, end_sample, max_frequency)
    """
    print(f"[*] Analyzing frequency spectrum (threshold: {AUDIO_THRESHOLD_HZ} Hz)...")
    
    S = np.abs(librosa.stft(y, n_fft=2048, hop_length=hop_length))
    freqs = librosa.fft_frequencies(sr=sr, n_fft=2048)
    energy_threshold = 0.01 * np.max(S)
    
    max_freq_per_frame = np.zeros(S.shape[1])
    for i in range(S.shape[1]):
        active = np.where(S[:, i] > energy_threshold)[0]
        if len(active) > 0:
            max_freq_per_frame[i] = freqs[active[-1]]
    
    anomaly_frames = np.where(max_freq_per_frame > AUDIO_THRESHOLD_HZ)[0]
    
    if len(anomaly_frames) == 0:
        return None, None, None
    
    start_frame, end_frame = anomaly_frames[0], anomaly_frames[-1]
    start_sample = start_frame * hop_length
    end_sample = end_frame * hop_length
    max_freq = np.max(max_freq_per_frame[anomaly_frames])
    
    print(f"[+] High-frequency anomaly detected: {max_freq:.0f} Hz ({len(anomaly_frames)} frames)")
    
    return start_sample, end_sample, max_freq

def generate_spectrogram_image(y_segment, sr, freq_max=None):
    """
    Generate spectrogram image from audio segment.
    
    Args:
        y_segment: Audio time series segment
        sr: Sample rate
        freq_max: Maximum frequency to include
        
    Returns:
        PIL Image of spectrogram
    """
    print("[*] Generating spectrogram image...")
    
    n_fft = 4096
    hop_length = 256
    S = np.abs(librosa.stft(y_segment, n_fft=n_fft, hop_length=hop_length))
    freqs = librosa.fft_frequencies(sr=sr, n_fft=n_fft)
    
    if freq_max:
        mask = freqs <= freq_max
        S = S[mask, :]
    
    vmin, vmax = np.percentile(S, [0.5, 99.5])
    S_norm = (S - vmin) / (vmax - vmin + 1e-8)
    S_bin = np.where(S_norm > 0.5, 255, 0).astype(np.uint8)
    
    img = Image.fromarray(S_bin, mode='L')
    side = max(img.width, img.height)
    
    return img.resize((side, side), Image.NEAREST)

def find_grid_size(img):
    """
    Detect QR code module size from spectrogram.
    
    Args:
        img: PIL Image
        
    Returns:
        Estimated grid size in pixels
    """
    gray = np.array(img)
    h, w = gray.shape
    row = gray[h // 2, :]
    binary = (row < 128).astype(int)
    transitions = np.where(np.abs(np.diff(binary)) > 0)[0]
    
    if len(transitions) > 1:
        widths = np.diff(transitions)
        grid_size = int(np.median(widths))
        print(f"[*] Detected QR grid size: {grid_size}px")
        return grid_size
    
    return 10

def binarize_with_grid(img, grid_size, threshold=0.95):
    """
    Binarize spectrogram image using grid-based thresholding.
    
    Args:
        img: PIL Image
        grid_size: Size of each QR module
        threshold: Black pixel ratio threshold
        
    Returns:
        Binarized PIL Image
    """
    print("[*] Binarizing spectrogram with grid-based thresholding...")
    
    gray = np.array(img)
    h, w = gray.shape
    out = np.ones_like(gray) * 255
    rows, cols = h // grid_size, w // grid_size
    
    for i in range(rows):
        for j in range(cols):
            ys, ye = i * grid_size, min((i + 1) * grid_size, h)
            xs, xe = j * grid_size, min((j + 1) * grid_size, w)
            cell = gray[ys:ye, xs:xe]
            black = np.sum(cell < 10)
            if black / cell.size >= threshold:
                out[ys:ye, xs:xe] = 0
    
    return Image.fromarray(out)

def process_audio_qr(mkv_file):
    """
    Extract QR code from audio track.
    
    Args:
        mkv_file: Path to input video file
        
    Returns:
        Decoded QR code string or empty string
    """
    print("\n" + "=" * 70)
    print("STAGE 1: Audio Track Processing")
    print("=" * 70)
    
    audio_data = extract_audio_bytes(mkv_file)
    
    print("[*] Loading audio data...")
    y, sr = librosa.load(audio_data, sr=None)
    print(f"[+] Audio loaded: {len(y)} samples @ {sr} Hz")
    
    start, end, max_freq = detect_max_frequency(y, sr)
    
    if start is None:
        print("[-] No high-frequency QR pattern detected in audio")
        return ""
    
    img = generate_spectrogram_image(y[start:end], sr, freq_max=max_freq)
    grid = find_grid_size(img)
    img_bin = binarize_with_grid(img, grid)
    
    print("[*] Decoding QR code from spectrogram...")
    
    with suppress_stderr():
        decoded = decode(img_bin)
    
    if decoded:
        qr_value = decoded[0].data.decode()
        print(f"[+] Audio QR decoded successfully: {qr_value}")
        return qr_value
    
    print("[-] Failed to decode QR code from audio")
    return ""

# ============================================================================
# Video Processing Functions
# ============================================================================

def calculate_frame_difference(f1, f2):
    """Calculate mean absolute difference between two frames."""
    g1 = cv2.cvtColor(f1, cv2.COLOR_BGR2GRAY)
    g2 = cv2.cvtColor(f2, cv2.COLOR_BGR2GRAY)
    return np.mean(cv2.absdiff(g1, g2))

def detect_glitch_start(video_path):
    """
    Detect the frame where visual glitches begin.
    
    Args:
        video_path: Path to video file
        
    Returns:
        Frame index of glitch start or None
    """
    print(f"[*] Detecting glitch start (threshold: {FRAME_DIFF_THRESHOLD}, min frames: {MIN_STABLE_FRAMES})...")
    
    cap = cv2.VideoCapture(video_path)
    idx, prev, stable, first = 0, None, 0, None
    
    while True:
        ret, frame = cap.read()
        if not ret:
            break
        
        if prev is not None:
            diff = calculate_frame_difference(prev, frame)
            if diff > FRAME_DIFF_THRESHOLD:
                if stable == 0:
                    first = idx
                stable += 1
                if stable >= MIN_STABLE_FRAMES:
                    cap.release()
                    print(f"[+] Glitch detected at frame {first}")
                    return first
            else:
                stable, first = 0, None
        
        prev = frame.copy()
        idx += 1
    
    cap.release()
    return None

def process_video_qr(video_path, audio_flag=""):
    """
    Extract QR codes from video frames.
    
    Args:
        video_path: Path to video file
        audio_flag: Flag prefix from audio track
        
    Returns:
        Complete flag string
    """
    print("\n" + "=" * 70)
    print("STAGE 2: Video Frame Processing")
    print("=" * 70)
    
    print("[*] Opening video file...")
    cap = cv2.VideoCapture(video_path)
    w, h = int(cap.get(3)), int(cap.get(4))
    fps = cap.get(cv2.CAP_PROP_FPS) or 25.0
    total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    
    print(f"[+] Video info: {w}x{h} @ {fps:.2f} FPS ({total_frames} frames)")
    
    glitch_start = detect_glitch_start(video_path)
    
    if glitch_start is None:
        glitch_start = int(22 * fps)
        print(f"[*] Using default glitch start: frame {glitch_start}")
    
    print(f"[*] Generating noise pattern (seed: {SEED})...")
    np.random.seed(SEED)
    noise = np.random.randint(0, 256, (h, w, 3), dtype=np.uint8)
    
    print(f"[*] Processing frames (batch size: {FRAMES_PER_QR} frames per QR)...")
    
    cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
    idx, since, detections = 0, 0, []
    flag = audio_flag
    
    print(f"[*] Flag reconstruction in progress: ", end='', flush=True)
    print(flag, end='', flush=True)
    
    while True:
        ret, frame = cap.read()
        if not ret:
            break
        
        if idx >= glitch_start:
            # Apply XOR decoding with rolling noise
            off = idx - glitch_start
            shift = off % w
            rolled = np.roll(noise, -shift, axis=1)
            frame = cv2.bitwise_xor(frame, rolled)
            
            # Preprocess for QR detection
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            blur = cv2.GaussianBlur(gray, (5, 5), 0)
            _, binarized = cv2.threshold(blur, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)
            
            # Decode QR codes
            with suppress_stderr():
                objs = decode(binarized)
            
            for obj in objs:
                try:
                    detections.append(obj.data.decode("utf-8"))
                except:
                    pass
            
            since += 1
            
            # Process batch of frames
            if since % FRAMES_PER_QR == 0:
                if detections:
                    mc = Counter(detections).most_common(1)[0][0]
                    flag += mc
                    print(mc, end='', flush=True)
                    
                    if mc == "}":
                        print()
                        break
                
                detections.clear()
        
        idx += 1
    
    cap.release()
    print("[+] Video QR extraction completed")
    
    return flag

# ============================================================================
# Main Solver Pipeline
# ============================================================================

def solve_challenge(mkv_file):
    """
    Main solver function.
    
    Args:
        mkv_file: Path to challenge video file
        
    Returns:
        Complete flag string
    """
    print("\n" + "=" * 70)
    print("WPCTF Shadow Garden Solver - QR Code Extraction")
    print("=" * 70)
    print(f"Input file: {mkv_file}")
    print("\nConfiguration:")
    print(f"  - Random seed: {SEED}")
    print(f"  - Frames per QR batch: {FRAMES_PER_QR}")
    print(f"  - Frame difference threshold: {FRAME_DIFF_THRESHOLD}")
    print(f"  - Minimum stable frames: {MIN_STABLE_FRAMES}")
    print(f"  - Audio frequency threshold: {AUDIO_THRESHOLD_HZ} Hz")
    
    start_time = time.time()
    
    # Stage 1: Audio processing
    audio_start = time.time()
    audio_flag = process_audio_qr(mkv_file)
    audio_time = time.time() - audio_start
    
    # Stage 2: Video processing
    video_start = time.time()
    complete_flag = process_video_qr(mkv_file, audio_flag)
    video_time = time.time() - video_start
    
    total_time = time.time() - start_time
    
    # Print results
    print("\n" + "=" * 70)
    print("RESULTS")
    print("=" * 70)
    print(f"[!] FLAG: {complete_flag}")
    print("\nTiming:")
    print(f"  - Audio processing: {audio_time:.2f}s")
    print(f"  - Video processing: {video_time:.2f}s")
    print(f"  - Total time:       {total_time:.2f}s")
    print("=" * 70 + "\n")
    
    return complete_flag

# ============================================================================
# Entry Point
# ============================================================================

def main():
    """Parse arguments and run solver."""
    parser = argparse.ArgumentParser(
        description="WPCTF Shadow Garden Solver - Extract hidden QR codes from audio/video",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Example:
  %(prog)s challenge.mkv
        """
    )
    
    parser.add_argument(
        "video_file",
        type=str,
        help="Path to the challenge video file (.mkv, .mp4, etc.)"
    )
    
    args = parser.parse_args()
    
    # Validate input file
    if not Path(args.video_file).exists():
        print(f"[!] Error: File '{args.video_file}' not found")
        sys.exit(1)
    
    try:
        solve_challenge(args.video_file)
    except KeyboardInterrupt:
        print("\n\n[!] Interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n[!] Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main() 
