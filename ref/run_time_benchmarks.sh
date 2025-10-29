#!/bin/bash

# Make the script robust: run from the script directory (where Makefile lives)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

# Log file (in the ref directory)
LOG="benchmark_results.log"

echo "=== SPHINCS+ test_time benchmark run ===" > "$LOG"
echo "Started: $(date -u)" >> "$LOG"
echo "" >> "$LOG"

# Ordered list of PARAMS as requested (start with 128s then 128f, then 192/256 variants)
PARAMS=(
    "sphincs-sha2-128s"
    "sphincs-sha2-128f"
    "sphincs-sha2-192s"
    "sphincs-sha2-192f"
    "sphincs-sha2-256s"
    "sphincs-sha2-256f"
)

for param in "${PARAMS[@]}"; do
    echo "========================================" | tee -a "$LOG"
    echo "Testing $param" | tee -a "$LOG"
    echo "Build start: $(date -u)" >> "$LOG"

    # Clean build artifacts
    echo "Running: make clean" >> "$LOG"
    make clean >> "$LOG" 2>&1

    # Build the test binary for this PARAMS/THASH
    echo "Running: make test/test_time PARAMS=$param THASH=simple EXTRA_CFLAGS=-w" >> "$LOG"
    # Add EXTRA_CFLAGS=-w to silence compiler warnings (-w)
    if ! make test/test_time PARAMS=$param THASH=simple EXTRA_CFLAGS=-w >> "$LOG" 2>&1; then
        echo "Make failed for $param (see $LOG)" | tee -a "$LOG"
        # continue to next parameter
        continue
    fi

    # Run the test binary and append output to the log
    if [ -x ./test/test_time ]; then
        echo "Running: ./test/test_time" >> "$LOG"
        echo "----- OUTPUT BEGIN ($param) -----" >> "$LOG"
        ./test/test_time >> "$LOG" 2>&1
        echo "----- OUTPUT END ($param) -----" >> "$LOG"
    else
        echo "Executable ./test/test_time not found for $param" | tee -a "$LOG"
    fi

    # Clean again to ensure fresh build for next param
    echo "Cleaning after run" >> "$LOG"
    make clean >> "$LOG" 2>&1

    echo "Build end: $(date -u)" >> "$LOG"
    echo "" >> "$LOG"
done

echo "All done: $(date -u)" >> "$LOG"
echo "Results saved to $LOG"