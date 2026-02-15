CC = g++
CFLAGS = -Wall -Wextra -Wno-narrowing -std=c++17
CPU_DEFINES = -DAI_ENABLE_COREML=0
COREML_DEFINES = -DAI_ENABLE_COREML=1
BENCH_OUT ?= bench_results.md
INFER ?= auto
PRE ?= auto

BREW_PREFIX := $(shell brew --prefix)
LDFLAGS = -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
LDFLAGS += -Wl,-rpath,$(BREW_PREFIX)/lib
RAYLIB_INCLUDE = -I$(BREW_PREFIX)/include
ONNX_INCLUDE = -I$(BREW_PREFIX)/include/onnxruntime
ONNX_LIBS = -L$(BREW_PREFIX)/lib -lonnxruntime
INCLUDES = $(RAYLIB_INCLUDE) $(ONNX_INCLUDE) -Isrc

APP_TARGET = flappy_bird
TEST_TARGET = run_tests
APP_TARGET_CPU = flappy_bird_cpu
APP_TARGET_COREML = flappy_bird_coreml
TEST_TARGET_CPU = run_tests_cpu
TEST_TARGET_COREML = run_tests_coreml
BENCH_TARGET_CPU = perf_compare_cpu
BENCH_TARGET_COREML = perf_compare_coreml
RM = rm -f
CP = cp -r
MKDIR = mkdir -p
RMDIR = rm -rf
TAR = tar

DIST_DIR = flappy_bird_dist

APP_SRC = \
	src/main.cpp \
	src/app/App.cpp \
	src/game/Pipe.cpp \
	src/game/Collision.cpp \
	src/game/Game.cpp \
	src/ai/FrameStack.cpp \
	src/ai/OnnxPolicy.cpp \
	src/ai/Preprocess.cpp \
	src/ai/AiAgent.cpp

TEST_SRC = \
	tests/test_main.cpp \
	src/game/Pipe.cpp \
	src/game/Collision.cpp \
	src/game/Game.cpp \
	src/ai/FrameStack.cpp \
	src/ai/OnnxPolicy.cpp \
	src/ai/Preprocess.cpp \
	src/ai/AiAgent.cpp

BENCH_SRC = \
	tests/perf_compare.cpp \
	src/ai/FrameStack.cpp \
	src/ai/OnnxPolicy.cpp \
	src/ai/Preprocess.cpp \
	src/ai/AiAgent.cpp

all: $(APP_TARGET)

$(APP_TARGET): $(APP_SRC)
	$(CC) $(CFLAGS) $(COREML_DEFINES) $(INCLUDES) -o $@ $^ $(LDFLAGS) $(ONNX_LIBS)

$(TEST_TARGET): $(TEST_SRC)
	$(CC) $(CFLAGS) $(COREML_DEFINES) $(INCLUDES) -o $@ $^ $(LDFLAGS) $(ONNX_LIBS)

$(APP_TARGET_CPU): $(APP_SRC)
	$(CC) $(CFLAGS) $(CPU_DEFINES) $(INCLUDES) -o $@ $^ $(LDFLAGS) $(ONNX_LIBS)

$(APP_TARGET_COREML): $(APP_SRC)
	$(CC) $(CFLAGS) $(COREML_DEFINES) $(INCLUDES) -o $@ $^ $(LDFLAGS) $(ONNX_LIBS)

$(TEST_TARGET_CPU): $(TEST_SRC)
	$(CC) $(CFLAGS) $(CPU_DEFINES) $(INCLUDES) -o $@ $^ $(LDFLAGS) $(ONNX_LIBS)

$(TEST_TARGET_COREML): $(TEST_SRC)
	$(CC) $(CFLAGS) $(COREML_DEFINES) $(INCLUDES) -o $@ $^ $(LDFLAGS) $(ONNX_LIBS)

$(BENCH_TARGET_CPU): $(BENCH_SRC)
	$(CC) $(CFLAGS) $(CPU_DEFINES) $(INCLUDES) -o $@ $^ $(LDFLAGS) $(ONNX_LIBS)

$(BENCH_TARGET_COREML): $(BENCH_SRC)
	$(CC) $(CFLAGS) $(COREML_DEFINES) $(INCLUDES) -o $@ $^ $(LDFLAGS) $(ONNX_LIBS)

clean:
	$(RM) $(APP_TARGET) $(TEST_TARGET) $(APP_TARGET_CPU) $(APP_TARGET_COREML)
	$(RM) $(TEST_TARGET_CPU) $(TEST_TARGET_COREML) $(BENCH_TARGET_CPU) $(BENCH_TARGET_COREML)
	$(RMDIR) $(DIST_DIR) $(DIST_DIR).tar.gz

test: $(TEST_TARGET)
	./$(TEST_TARGET)

test_cpu: $(TEST_TARGET_CPU)
	./$(TEST_TARGET_CPU)

test_coreml: $(TEST_TARGET_COREML)
	./$(TEST_TARGET_COREML)

build_cpu: $(APP_TARGET_CPU)

build_coreml: $(APP_TARGET_COREML)

run_cpu: $(APP_TARGET_CPU)
	FLAPPY_INFERENCE=$(INFER) FLAPPY_PREPROCESS=$(PRE) DYLD_LIBRARY_PATH=$(BREW_PREFIX)/lib:$$DYLD_LIBRARY_PATH ./$(APP_TARGET_CPU)

run_coreml: $(APP_TARGET_COREML)
	FLAPPY_INFERENCE=$(INFER) FLAPPY_PREPROCESS=$(PRE) DYLD_LIBRARY_PATH=$(BREW_PREFIX)/lib:$$DYLD_LIBRARY_PATH ./$(APP_TARGET_COREML)

bench_cpu: $(BENCH_TARGET_CPU)
	FLAPPY_BENCH_OUT=$(BENCH_OUT) DYLD_LIBRARY_PATH=$(BREW_PREFIX)/lib:$$DYLD_LIBRARY_PATH ./$(BENCH_TARGET_CPU)

bench_coreml: $(BENCH_TARGET_COREML)
	FLAPPY_BENCH_OUT=$(BENCH_OUT) DYLD_LIBRARY_PATH=$(BREW_PREFIX)/lib:$$DYLD_LIBRARY_PATH ./$(BENCH_TARGET_COREML)

bench_compare: bench_coreml

dist: $(APP_TARGET)
	# Create distribution directory structure
	$(MKDIR) $(DIST_DIR)/lib
	$(MKDIR) $(DIST_DIR)/assets
	$(MKDIR) $(DIST_DIR)/trained_models

	# Copy executable
	$(CP) $(APP_TARGET) $(DIST_DIR)/

	# Copy dynamic libraries (Mac-specific)
	$(CP) $(BREW_PREFIX)/lib/libonnxruntime*.dylib $(DIST_DIR)/lib/ || echo "Warning: Could not find libonnxruntime.dylib"
	$(CP) $(BREW_PREFIX)/lib/libraylib*.dylib $(DIST_DIR)/lib/ || echo "Warning: Could not find libraylib.dylib"

	# Copy assets directory
	$(CP) assets $(DIST_DIR)/

	# Copy trained_models directory
	$(CP) trained_models $(DIST_DIR)/

	# Create launcher script
	echo '#!/bin/bash' > $(DIST_DIR)/run.sh
	echo 'DIR="$$( cd "$$( dirname "$${BASH_SOURCE[0]}" )" && pwd )"' >> $(DIST_DIR)/run.sh
	echo 'export DYLD_LIBRARY_PATH="$$DIR/lib:$$DYLD_LIBRARY_PATH"' >> $(DIST_DIR)/run.sh
	echo '"$$DIR/$(APP_TARGET)"' >> $(DIST_DIR)/run.sh
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
	$(eval BREW_PREFIX ?= $(shell brew --prefix))
	@echo "Run this command in your terminal to set the library path:"
	@echo "export DYLD_LIBRARY_PATH=$(BREW_PREFIX)/lib:$$DYLD_LIBRARY_PATH"

install_deps:
	@echo "Installing dependencies for macOS..."
	$(MAKE) install_raylib
	$(MAKE) install_onnx
	@echo "All dependencies installed for macOS"

run: $(APP_TARGET)
	FLAPPY_INFERENCE=$(INFER) FLAPPY_PREPROCESS=$(PRE) DYLD_LIBRARY_PATH=$(BREW_PREFIX)/lib:$$DYLD_LIBRARY_PATH ./$(APP_TARGET)

.PHONY: all clean test test_cpu test_coreml dist install_raylib install_onnx setup_env install_deps run build_cpu build_coreml run_cpu run_coreml bench_cpu bench_coreml bench_compare
