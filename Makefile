# Makefile for GPU Batch Image Processor
# Tested on Linux with CUDA 11+

CUDA_PATH   ?= /usr/local/cuda
NVCC         = $(CUDA_PATH)/bin/nvcc

# CUDA sample helpers (adjust path if needed)
SAMPLE_DIR  ?= /usr/local/cuda/samples
INCLUDES     = -I$(CUDA_PATH)/include \
                -I$(SAMPLE_DIR)/common/inc

CXXFLAGS     = -std=c++14 -O2
NVCCFLAGS    = $(CXXFLAGS) $(INCLUDES)

LDFLAGS      = -L$(CUDA_PATH)/lib64 \
                -lcudart \
                -lnppc -lnppial -lnppicc -lnppidei \
                -lnppif -lnppig -lnppim -lnppist \
                -lnppisu -lnppitc

SRC_DIR      = src
BIN_DIR      = bin
TARGET       = $(BIN_DIR)/batchImageProc
SRC          = $(SRC_DIR)/imageRotationNPP.cpp

.PHONY: all clean help data

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p $(BIN_DIR)
	$(NVCC) $(NVCCFLAGS) $< -o $@ $(LDFLAGS)

# Generate synthetic test images (requires Python3 + Pillow)
data:
	python3 scripts/generate_test_images.py --count 120 --outdir data/input

clean:
	rm -rf $(BIN_DIR)

help:
	@echo ""
	@echo "Usage:"
	@echo "  make              Build the binary"
	@echo "  make data         Generate 120 synthetic .pgm test images"
	@echo "  make clean        Remove build artifacts"
	@echo "  make help         Show this message"
	@echo ""
	@echo "Run:"
	@echo "  ./run.sh"
	@echo "  $(TARGET) --input=data/input --output=data/output --filter=all"
	@echo ""
