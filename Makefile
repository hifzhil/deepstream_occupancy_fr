################################################################################
# Copyright (c) 2019-2020, NVIDIA CORPORATION. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
################################################################################

# ==============================================================================
# Build Configuration
# ==============================================================================

# CUDA Version - Required for building
CUDA_VER:=12.6

# Force CUDA paths explicitly
CUDA_PATH:=/usr/local/cuda-12.6
CUDA_INC_PATH:=/usr/local/cuda-12.6/include
CUDA_LIB_PATH:=/usr/local/cuda-12.6/lib64

# DeepStream SDK root directory
# This should point to your DeepStream installation directory
# Can be overridden from command line: make DEEPSTREAM_SDK_ROOT=/path/to/deepstream
DEEPSTREAM_SDK_ROOT?=/opt/nvidia/deepstream/deepstream-7.1
ifeq ($(wildcard $(DEEPSTREAM_SDK_ROOT)),)
  $(error "DEEPSTREAM_SDK_ROOT is not set correctly. Please set it to your DeepStream installation directory")
endif

# Application name
APP:= deepstream-test5-analytics

# Target device detection
TARGET_DEVICE = $(shell gcc -dumpmachine | cut -f1 -d -)
DS_VER = $(shell deepstream-app -v | awk '$$1~/DeepStreamSDK/ {print substr($$2,1,3)}' )

# Installation directories
LIB_INSTALL_DIR?=$(DEEPSTREAM_SDK_ROOT)/lib/
APP_INSTALL_DIR?=$(DEEPSTREAM_SDK_ROOT)/bin/

# Platform-specific flags
ifeq ($(TARGET_DEVICE),aarch64)
  CFLAGS:= -DPLATFORM_TEGRA
endif

# ==============================================================================
# Source and Include Directories
# ==============================================================================

# Define source directories from DeepStream SDK
DEEPSTREAM_APP_DIR=$(DEEPSTREAM_SDK_ROOT)/sources/apps/sample_apps/deepstream-app
DEEPSTREAM_TEST5_DIR=$(DEEPSTREAM_SDK_ROOT)/sources/apps/sample_apps/deepstream-test5
APPS_COMMON_DIR=$(DEEPSTREAM_SDK_ROOT)/sources/apps/apps-common

# Local object directory for build artifacts
OBJ_DIR := obj
$(shell mkdir -p $(OBJ_DIR))

# Source files
# Note: We include files from both local directory and DeepStream SDK
SRCS:= deepstream_test5_app_main.c
SRCS+= $(DEEPSTREAM_TEST5_DIR)/deepstream_utc.c
SRCS+= $(DEEPSTREAM_APP_DIR)/deepstream_app.c $(DEEPSTREAM_APP_DIR)/deepstream_app_config_parser.c
SRCS+= $(wildcard $(APPS_COMMON_DIR)/src/*.c)

# Header files
INCS= $(wildcard *.h)
INC_DIR=$(DEEPSTREAM_TEST5_DIR)

# Required packages for pkg-config
PKGS:= gstreamer-1.0 gstreamer-video-1.0 x11 json-glib-1.0

# ==============================================================================
# Build Rules
# ==============================================================================

# Object files will be placed in the local obj directory
# Using notdir to strip paths and addprefix to add obj/ prefix
OBJS:= $(addprefix $(OBJ_DIR)/,$(notdir $(SRCS:.c=.o)))
OBJS+= $(OBJ_DIR)/deepstream_nvdsanalytics_meta.o

# Compiler flags
CFLAGS:= -I$(APPS_COMMON_DIR)/includes -I./includes -I$(DEEPSTREAM_SDK_ROOT)/sources/includes -I$(DEEPSTREAM_APP_DIR)/ -DDS_VERSION_MINOR=1 -DDS_VERSION_MAJOR=5
CFLAGS+= -I$(INC_DIR)
CFLAGS+= -I$(CUDA_INC_PATH)
CFLAGS+= -D_GNU_SOURCE -DINT64=4 -include stdint.h

# Linker flags and libraries
LIBS+= -L$(LIB_INSTALL_DIR) -lnvdsgst_meta -lnvds_meta -lnvdsgst_helper \
       -lnvdsgst_customhelper -lnvdsgst_smartrecord -lnvds_utils -lnvds_msgbroker -lm \
       -lgstrtspserver-1.0 -ldl
LIBS+= -L$(CUDA_LIB_PATH) -lcudart

# Add pkg-config flags
PKG_CONFIG_FLAGS:= `pkg-config --cflags $(PKGS)`
CFLAGS+= $(PKG_CONFIG_FLAGS)
LIBS+= `pkg-config --libs $(PKGS)`

# Add runtime library path
LDFLAGS+= -Wl,-rpath,$(LIB_INSTALL_DIR):$(CUDA_LIB_PATH)

# ==============================================================================
# Build Targets
# ==============================================================================

# Default target
all: $(APP)

# Compile C++ source file
$(OBJ_DIR)/deepstream_nvdsanalytics_meta.o: deepstream_nvdsanalytics_meta.cpp $(INCS) Makefile
	$(CXX) -c -o $@ -Wall -Werror $(CFLAGS) $<

# Pattern rule for compiling local .c files
$(OBJ_DIR)/%.o: %.c $(INCS) Makefile
	$(CC) -c -o $@ $(CFLAGS) $<

# Pattern rule for compiling .c files from deepstream-test5 directory
$(OBJ_DIR)/%.o: $(DEEPSTREAM_TEST5_DIR)/%.c $(INCS) Makefile
	$(CC) -c -o $@ $(CFLAGS) $<

# Pattern rule for compiling .c files from deepstream-app directory
$(OBJ_DIR)/%.o: $(DEEPSTREAM_APP_DIR)/%.c $(INCS) Makefile
	$(CC) -c -o $@ $(CFLAGS) $<

# Pattern rule for compiling .c files from apps-common directory
$(OBJ_DIR)/%.o: $(APPS_COMMON_DIR)/src/%.c $(INCS) Makefile
	$(CC) -c -o $@ $(CFLAGS) $<

# Link all object files into final executable
$(APP): $(OBJS) Makefile
	$(CXX) $(LDFLAGS) -o $(APP) $(OBJS) $(LIBS)

# Install target - copies the executable to DeepStream bin directory
install: $(APP)
	cp -rv $(APP) $(APP_INSTALL_DIR)

# Clean target - removes all build artifacts
clean:
	rm -rf $(OBJ_DIR) $(APP)

# ==============================================================================
# Usage Instructions
# ==============================================================================
# To build:
#   make
#
# To build with custom CUDA version:
#   make CUDA_VER=12.6
#
# To build with custom DeepStream SDK location:
#   make DEEPSTREAM_SDK_ROOT=/path/to/deepstream
#
# To install:
#   make install
#
# To clean:
#   make clean
#
# Note: Installation requires write permissions to the DeepStream SDK directory
# ==============================================================================

