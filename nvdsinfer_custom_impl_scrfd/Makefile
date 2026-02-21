CUDA_VER?=12.6
ifeq ($(CUDA_VER),)
  $(error "CUDA_VER is not set")
endif

CC:= g++
NVCC:=/usr/local/cuda-$(CUDA_VER)/bin/nvcc

CFLAGS:= -Wall -std=c++11 -shared -fPIC
CFLAGS+= -I../includes
CFLAGS+= -I/opt/nvidia/deepstream/deepstream-7.1/sources/includes
CFLAGS+= -I/usr/local/cuda-$(CUDA_VER)/include
CFLAGS+= -I/usr/local/cuda/include
CFLAGS+= -I/usr/include/x86_64-linux-gnu

LIBS:= -lnvinfer -lnvonnxparser
LIBS+= -L/usr/lib/x86_64-linux-gnu
LIBS+= -L/usr/local/cuda-$(CUDA_VER)/lib64
LIBS+= -L/usr/local/cuda/lib64
LIBS+= -L/opt/nvidia/deepstream/deepstream-7.1/lib
LFLAGS:= -shared -Wl,--start-group $(LIBS) -Wl,--end-group

SRCFILES:= ../src/nvdsinfer_scrfd_parser.cpp
TARGET_LIB:= libnvdsinfer_custom_impl_scrfd.so

all: $(TARGET_LIB)

$(TARGET_LIB) : $(SRCFILES)
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)

clean:
	rm -rf $(TARGET_LIB) 