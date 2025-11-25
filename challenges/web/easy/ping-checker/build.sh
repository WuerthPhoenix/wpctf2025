#!/bin/bash

# PingChecker Pro - Build Script
# WPCTF 2025

CHALLENGE_NAME=${PWD##*/}
SCRIPT_DIR=$(pwd)
WORK_DIR=$(mktemp -d)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Cleanup function
function cleanup {
    rm -rf "$WORK_DIR"
    echo -e "${GREEN}[+] Cleaned up temporary directory${NC}"
}

trap cleanup EXIT

# Verify required files exist
echo -e "${YELLOW}[*] Verifying required files...${NC}"

REQUIRED_FILES=("Dockerfile" "flag.txt" ".zipignore" "src/index.php" "src/ping.php")
for file in "${REQUIRED_FILES[@]}"; do
    if [[ ! -f "$file" ]]; then
        echo -e "${RED}[-] Missing required file: $file${NC}"
        exit 1
    fi
done

echo -e "${GREEN}[+] All required files present${NC}"

# Read and validate flag
FLAG=$(cat flag.txt | tr -d '\n')

if [[ ! "$FLAG" =~ ^WPCTF\{.*\}$ ]]; then
    echo -e "${RED}[-] Invalid flag format. Must be WPCTF{...}${NC}"
    exit 1
fi

echo -e "${YELLOW}[*] Building challenge: $CHALLENGE_NAME${NC}"
echo -e "${YELLOW}[*] Flag: ${FLAG:0:10}...${FLAG: -5}${NC}"

# Copy files to temp directory
cp -r . "$WORK_DIR"
cd "$WORK_DIR"

# Create distribution ZIP (excluding sensitive files)
echo -e "${YELLOW}[*] Creating distribution archive...${NC}"

if [[ -f .zipignore ]]; then
    # Remove files listed in .zipignore
    while IFS= read -r pattern || [[ -n "$pattern" ]]; do
        # Skip empty lines and comments
        [[ -z "$pattern" || "$pattern" =~ ^# ]] && continue
        
        # Remove matching files/directories
        find . -name "$pattern" -exec rm -rf {} + 2>/dev/null
    done < .zipignore
fi

# Create the zip file
zip -r "$SCRIPT_DIR/$CHALLENGE_NAME.zip" . -q

if [[ -f "$SCRIPT_DIR/$CHALLENGE_NAME.zip" ]]; then
    echo -e "${GREEN}[+] Distribution archive created: $CHALLENGE_NAME.zip${NC}"
    ZIP_SIZE=$(ls -lh "$SCRIPT_DIR/$CHALLENGE_NAME.zip" | awk '{print $5}')
    echo -e "${GREEN}    Size: $ZIP_SIZE${NC}"
else
    echo -e "${RED}[-] Failed to create distribution archive${NC}"
    exit 1
fi

# Build Docker image
cd "$SCRIPT_DIR"
echo -e "${YELLOW}[*] Building Docker image...${NC}"

docker build \
    --no-cache \
    --build-arg FLAG="$FLAG" \
    --tag "wpctf2025-challenges:$CHALLENGE_NAME" \
    -f Dockerfile \
    . 2>&1 | while IFS= read -r line; do
        if [[ "$line" =~ "Successfully" ]]; then
            echo -e "${GREEN}[+] $line${NC}"
        elif [[ "$line" =~ "Error" || "$line" =~ "error" ]]; then
            echo -e "${RED}[-] $line${NC}"
        else
            echo "    $line"
        fi
    done

# Check if build was successful
if docker image inspect "wpctf2025-challenges:$CHALLENGE_NAME" &>/dev/null; then
    echo -e "${GREEN}[+] Docker build completed successfully!${NC}"
    
    # Get image details
    IMAGE_ID=$(docker images -q "wpctf2025-challenges:$CHALLENGE_NAME")
    IMAGE_SIZE=$(docker images --format "table {{.Size}}" "wpctf2025-challenges:$CHALLENGE_NAME" | tail -1)
    
    echo -e "${GREEN}[+] Image ID: $IMAGE_ID${NC}"
    echo -e "${GREEN}[+] Image Size: $IMAGE_SIZE${NC}"
    
    echo ""
    echo -e "${GREEN}════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}[!] Build completed successfully!${NC}"
    echo -e "${GREEN}════════════════════════════════════════════════════${NC}"
    echo ""
    echo -e "${YELLOW}To run the challenge locally:${NC}"
    echo -e "  ${GREEN}docker run -d -p 8080:80 wpctf2025-challenges:$CHALLENGE_NAME${NC}"
    echo ""
    echo -e "${YELLOW}To test the challenge:${NC}"
    echo -e "  ${GREEN}curl http://localhost:8080/${NC}"
    echo -e "  ${GREEN}python3 exploit.py http://localhost:8080${NC}"
    echo ""
else
    echo -e "${RED}[!] Docker build failed!${NC}"
    exit 1
fi
