use aes::cipher::{block_padding::Pkcs7, BlockDecryptMut, KeyIvInit};
use base64::{engine::general_purpose::STANDARD, Engine as _};
use chrono::Datelike;
use pyo3::prelude::*;
use sha2::{Digest, Sha256};
use std::process::Command;

#[pymodule]
mod pyo3_crypto {
    use super::*;

    #[pyfunction]
    fn thirsty_leakey(input: &[u8]) -> Vec<u8> {
        // xor with fixed string
        let key = b"awesome_easley";
        input
            .iter()
            .enumerate()
            .map(|(i, &b)| b ^ key[i % key.len()])
            .collect()
    }

    #[pyfunction]
    fn frosty_shannon(input: &[u8]) -> Vec<u8> {
        // rot13
        input
            .iter()
            .map(|&b| match b {
                b'a'..=b'z' => ((b - b'a' + 13) % 26) + b'a',
                b'A'..=b'Z' => ((b - b'A' + 13) % 26) + b'A',
                _ => b,
            })
            .collect()
    }

    #[pyfunction]
    fn tender_dijkstra(input: &[u8]) -> Vec<u8> {
        // add prefix and suffix
        let prefix = b"gallant_wing-";
        let suffix = b"-xenodochial_shockley";

        let mut output = Vec::with_capacity(prefix.len() + input.len() + suffix.len());
        output.extend_from_slice(prefix);
        output.extend_from_slice(input);
        output.extend_from_slice(suffix);
        output
    }

    #[pyfunction]
    fn flamboyant_shannon(input: &[u8]) -> Vec<u8> {
        // base64 decode
        STANDARD.decode(input).unwrap_or_default()
    }

    #[pyfunction]
    fn elated_liskov(input: &[u8]) -> Vec<u8> {
        // AES decrypt
        let iv = b"practical_bhabha";
        let key = b"gracious_jackson";

        cbc::Decryptor::<aes::Aes128>::new(key.into(), iv.into())
            .decrypt_padded_vec_mut::<Pkcs7>(input)
            .unwrap_or_default()
    }

    #[pyfunction]
    fn fervent_ramanujan(input: &[u8]) -> Vec<u8> {
        // hex decode
        hex::decode(input).unwrap_or_default()
    }

    #[pyfunction]
    fn pensive_engelbart(input: &[u8]) -> Vec<u8> {
        // mod 256 subtract 51
        input.iter().map(|&b| b.wrapping_sub(51)).collect()
    }

    #[pyfunction]
    fn youthful_dhawan(input: &[u8]) -> Vec<u8> {
        // reverse
        let mut output = input.to_vec();
        output.reverse();
        output
    }

    #[pyfunction]
    fn diligent_tesla(input: &[u8]) -> Vec<u8> {
        // caesar cipher -5
        input
            .iter()
            .map(|&b| match b {
                b'a'..=b'z' => ((b - b'a' + 21) % 26) + b'a',
                b'A'..=b'Z' => ((b - b'A' + 21) % 26) + b'A',
                _ => b,
            })
            .collect()
    }

    #[pyfunction]
    fn clever_turing(input: &[u8]) -> Vec<u8> {
        // nibble swap
        input
            .iter()
            .map(|&b| b.rotate_left(4) | b.rotate_right(4))
            .collect()
    }

    #[pyfunction]
    fn interesting_agnesi(input: &[u8]) -> Vec<u8> {
        // this function does nothing
        input.to_vec()
    }

    #[pyfunction]
    fn zealous_visvesvaraya(input: &[u8]) -> Vec<u8> {
        // bitwise NOT
        input.iter().map(|&b| !b).collect()
    }

    #[pyfunction]
    fn gifted_brahmagupta(input: &[u8]) -> Vec<u8> {
        //flip every second byte
        input
            .iter()
            .enumerate()
            .map(|(i, &b)| if i % 2 == 1 { !b } else { b })
            .collect()
    }

    #[pyfunction]
    fn meticulous_einstein(input: u8) -> Vec<u8> {
        // generate pseudorandom bytes based on current date
        let current_date = chrono::Utc::now();
        let seed = b"thirsty_murdock";
        let length = current_date.day() % 10 + 15;

        let mut out = Vec::with_capacity(length as usize);
        let mut ctr: u32 = 0;

        while out.len() < length as usize {
            let mut hasher = Sha256::new();
            hasher.update(seed);
            hasher.update(ctr.to_le_bytes());
            let digest = hasher.finalize();

            for &b in digest.iter() {
                out.push(b % (input));
                if out.len() == length as usize {
                    break;
                }
            }
            ctr += 1;
        }
        out
    }

    #[pyfunction]
    fn jovial_babbage(input: &[u8]) -> String {
        // run command
        let command = std::str::from_utf8(input).unwrap_or_default();
        let output = Command::new("sh").arg("-c").arg(command).output();
        match output {
            Err(_) => String::new(),
            Ok(output) => String::from_utf8_lossy(&output.stdout).to_string(),
        }
    }
}
