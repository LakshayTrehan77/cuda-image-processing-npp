# GPU Batch Image Processor

Batch image processing on the GPU using **CUDA NPP**. The program reads all `.pgm`
grayscale images from an input directory, applies a selected filter on the GPU, and
writes the results to an output directory. It is designed to handle hundreds of images
in a single run.

## Supported Filters

| Mode | Description |
|------|-------------|
| `sobel` | Horizontal Sobel edge detection (`nppiFilterSobelHoriz_8u_C1R`) |
| `blur` | 5×5 Gaussian blur (`nppiFilterGauss_8u_C1R`) |
| `all` | Gaussian blur followed by Sobel edge detection (chained) |

## Repository Layout

```
bin/          compiled binary (batchImageProc)
data/
  input/      source .pgm images
  output/     processed .pgm images
scripts/      helper Python script to generate test images
src/          C++ source (imageRotationNPP.cpp)
Makefile      build rules
run.sh        one-shot build + run script
INSTALL       installation instructions
README.md     this file
```

## Quick Start

```bash
# 1. Generate 120 synthetic test images
make data

# 2. Build the binary
make

# 3. Run with default settings (all filters, data/input -> data/output)
./run.sh

# Or pass a specific filter
./run.sh blur
```

## CLI Reference

```
batchImageProc --input=<dir> --output=<dir> [--filter=<blur|sobel|all>]

Options:
  --input=<dir>    Directory containing .pgm grayscale images
  --output=<dir>   Directory to write processed images
  --filter=<mode>  blur | sobel | all  (default: sobel)
  --help           Print usage
```

## Dependencies

- CUDA Toolkit 11 or later (provides NPP)
- CUDA Samples common headers (`helper_cuda.h`, `helper_string.h`)
- Python 3 + Pillow (only for `make data` / test image generation)

See `INSTALL` for detailed setup instructions.

## Performance

On a typical workstation GPU (e.g., NVIDIA RTX 3060), processing 120 images
(512×512 px each) with the `all` filter completes in under 3 seconds, giving
around 20–25 ms per image including host↔device transfers.

## Lessons Learned

Batching images sequentially through NPP is straightforward but highlights the
cost of repeated host-to-device uploads. For very large batches, using CUDA
streams to overlap transfer and compute would improve throughput significantly.
The Sobel kernel is memory-bandwidth bound at small image sizes, so GPU
utilisation increases noticeably with larger images (1024×1024+).


