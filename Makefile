CUDA_PATH   ?= /usr/local/cuda
NVCC         = $(CUDA_PATH)/bin/nvcc

SAMPLE_DIR  ?= /usr/local/cuda/samples

INCLUDES     = -I. \
               -I$(CUDA_PATH)/include \
               -I$(SAMPLE_DIR)/common/inc\
			   -I/home/coder/project/Common/UtilNPP\
			   -I/home/coder/project/Common\
			   -I/home/coder/project/Common/data\
			   -I/home/coder/project/Common/GL\
			   -I/home/coder/project/Common/lib/x64\

CXXFLAGS     = -std=c++14 -O2
NVCCFLAGS    = $(CXXFLAGS) $(INCLUDES)

LDFLAGS      = -L$(CUDA_PATH)/lib64 \
               -lcudart \
               -lnppc -lnppial -lnppicc -lnppidei \
               -lnppif -lnppig -lnppim -lnppist \
               -lnppisu -lnppitc\
			   -lfreeimage

SRC_DIR      = src
BIN_DIR      = bin
TARGET       = $(BIN_DIR)/batchImageProc
SRC          = $(SRC_DIR)/imageRotationNPP.cpp

.PHONY: all clean help data

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p $(BIN_DIR)
	$(NVCC) $(NVCCFLAGS) $< -o $@ $(LDFLAGS)

data:
	python3 scripts/generate_test_images.py --count 120 --outdir data/input

clean:
	rm -rf $(BIN_DIR)

help:
	@echo ""
	@echo "Usage:"
	@echo "  make"
	@echo "  make data"
	@echo "  make clean"
	@echo "  make help"
	@echo ""
	@echo "Run:"
	@echo "  ./run.sh"
	@echo "  $(TARGET) --input=data/input --output=data/output --filter=all"
	@echo ""