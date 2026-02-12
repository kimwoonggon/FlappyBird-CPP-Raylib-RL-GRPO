CC = g++
CFLAGS = -Wall -std=c++17 -Wno-narrowing

BREW_PREFIX := $(shell brew --prefix)
LDFLAGS = -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
LDFLAGS += -Wl,-rpath,$(BREW_PREFIX)/lib
RAYLIB_INCLUDE = -I$(BREW_PREFIX)/include
ONNX_INCLUDE = -I$(BREW_PREFIX)/include/onnxruntime
ONNX_LIBS = -L$(BREW_PREFIX)/lib -lonnxruntime
INCLUDES = $(RAYLIB_INCLUDE) $(ONNX_INCLUDE)

TARGET = flappy_bird
RM = rm -f
CP = cp -r
MKDIR = mkdir -p
RMDIR = rm -rf
TAR = tar

SRC = main.cpp
DIST_DIR = flappy_bird_dist

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS) $(ONNX_LIBS)

clean:
	$(RM) $(TARGET)
	$(RMDIR) $(DIST_DIR) $(DIST_DIR).tar.gz

dist: $(TARGET)
	# Create distribution directory structure
	$(MKDIR) $(DIST_DIR)/lib
	$(MKDIR) $(DIST_DIR)/assets
	$(MKDIR) $(DIST_DIR)/trained_models
	
	# Copy executable
	$(CP) $(TARGET) $(DIST_DIR)/
	
	# Copy dynamic libraries (Mac-specific)
	$(CP) $(BREW_PREFIX)/lib/libonnxruntime*.dylib $(DIST_DIR)/lib/ || echo "Warning: Could not find libonnxruntime.dylib"
	$(CP) $(BREW_PREFIX)/lib/libraylib*.dylib $(DIST_DIR)/lib/ || echo "Warning: Could not find libraylib.dylib"
	
	# Copy assets directory
	$(CP) assets $(DIST_DIR)/
	
	# Copy trained_models directory
	$(CP) trained_models $(DIST_DIR)/
	
	# Create launcher script
	echo '#!/bin/bash' > $(DIST_DIR)/run.sh
	echo 'DIR="$( cd "$( dirname "$${BASH_SOURCE[0]}" )" && pwd )"' >> $(DIST_DIR)/run.sh
	echo 'export DYLD_LIBRARY_PATH="$$DIR/lib:$$DYLD_LIBRARY_PATH"' >> $(DIST_DIR)/run.sh
	echo '"$$DIR/$(TARGET)"' >> $(DIST_DIR)/run.sh
	chmod +x $(DIST_DIR)/run.sh
	
	# Create distribution archive
	$(TAR) -czf $(DIST_DIR).tar.gz $(DIST_DIR)
	@echo "Distribution package created at $(DIST_DIR).tar.gz"

install_raylib:
	brew update && \
	brew install raylib
	@echo "raylib installed successfully on macOS"

install_onnx:
	brew install wget
	@echo "Detecting processor architecture..."
	@ARCH=$$(uname -m); \
	BREW_PREFIX=$$(brew --prefix); \
	echo "Detected architecture: $$ARCH"; \
	echo "Removing any existing ONNX installation..."; \
	sudo rm -rf $$BREW_PREFIX/include/onnxruntime; \
	sudo rm -f $$BREW_PREFIX/lib/libonnxruntime*; \
	if [ "$$ARCH" = "arm64" ]; then \
		echo "Downloading arm64 version..."; \
		wget https://github.com/microsoft/onnxruntime/releases/download/v1.15.1/onnxruntime-osx-arm64-1.15.1.tgz; \
		tar -xzf onnxruntime-osx-arm64-1.15.1.tgz; \
		sudo mkdir -p $$BREW_PREFIX/include/onnxruntime; \
		sudo mkdir -p $$BREW_PREFIX/lib; \
		sudo cp -r onnxruntime-osx-arm64-1.15.1/include/* $$BREW_PREFIX/include/onnxruntime/; \
		sudo cp -r onnxruntime-osx-arm64-1.15.1/lib/* $$BREW_PREFIX/lib/; \
		rm -rf onnxruntime-osx-arm64-1.15.1 onnxruntime-osx-arm64-1.15.1.tgz; \
	else \
		echo "Downloading x86_64 version..."; \
		wget https://github.com/microsoft/onnxruntime/releases/download/v1.15.1/onnxruntime-osx-x86_64-1.15.1.tgz; \
		tar -xzf onnxruntime-osx-x86_64-1.15.1.tgz; \
		sudo mkdir -p $$BREW_PREFIX/include/onnxruntime; \
		sudo mkdir -p $$BREW_PREFIX/lib; \
		sudo cp -r onnxruntime-osx-x86_64-1.15.1/include/* $$BREW_PREFIX/include/onnxruntime/; \
		sudo cp -r onnxruntime-osx-x86_64-1.15.1/lib/* $$BREW_PREFIX/lib/; \
		rm -rf onnxruntime-osx-x86_64-1.15.1 onnxruntime-osx-x86_64-1.15.1.tgz; \
	fi; \
	echo "ONNX Runtime installed successfully to $$BREW_PREFIX"

setup_env:
	# Setting up macOS environment
	$(eval BREW_PREFIX ?= $(shell brew --prefix))
	@echo "Run this command in your terminal to set the library path:"
	@echo "export DYLD_LIBRARY_PATH=$(BREW_PREFIX)/lib:$$DYLD_LIBRARY_PATH"

install_deps:
	@echo "Installing dependencies for macOS..."
	$(MAKE) install_raylib
	$(MAKE) install_onnx
	@echo "All dependencies installed for macOS"

.PHONY: all clean dist install_raylib install_onnx setup_env install_deps run

run: $(TARGET)
	DYLD_LIBRARY_PATH=$(BREW_PREFIX)/lib:$$DYLD_LIBRARY_PATH ./$(TARGET) 