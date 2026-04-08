#!/usr/bin/env bash
# run.sh - Build and execute the GPU batch image processor

set -e

BIN=./bin/batchImageProc
INPUT=data/input
OUTPUT=data/output
FILTER=${1:-all}   # pass filter as first arg, default: all

# Generate test images if input dir is empty
if [ ! "$(ls -A $INPUT 2>/dev/null)" ]; then
  echo "Generating test images..."
  python3 scripts/generate_test_images.py --count 120 --outdir $INPUT
fi

# Build if binary not present
if [ ! -f "$BIN" ]; then
  echo "Building..."
  make
fi

mkdir -p $OUTPUT

echo "Running: $BIN --input=$INPUT --output=$OUTPUT --filter=$FILTER"
$BIN --input=$INPUT --output=$OUTPUT --filter=$FILTER
