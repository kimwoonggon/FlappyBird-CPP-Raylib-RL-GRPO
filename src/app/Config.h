#pragma once

#include <string>

namespace app {

enum class InferenceBackend {
  kAuto,
  kCpu,
  kCoreMl
};

enum class PreprocessBackend {
  kAuto,
  kCpu,
  kGpuShader
};

struct ScreenConfig {
  int width = 288;
  int height = 512;
  int fps = 30;
  float baseY = 512.0F * 0.79F;
};

struct PhysicsConfig {
  float gravity = 1.0F;
  float jumpVelocity = -9.0F;
  float maxVelocityY = 10.0F;
};

struct PipeConfig {
  int gapSize = 110;
  float velocityX = -4.0F;
  int spawnOffsetX = 10;
  int lowerStartClamp = 260;
  int scoreWindow = 5;
};

struct AiConfig {
  int inputSize = 84;
  int frameStack = 4;
  int frameDisplaySize = 40;
  int cropY = 108;
  int cropHeight = 512;
  InferenceBackend inferenceBackend = InferenceBackend::kAuto;
  PreprocessBackend preprocessBackend = PreprocessBackend::kAuto;
  std::string modelPath = "trained_models/flappy_bird_rl.onnx";
};

struct Config {
  ScreenConfig screen{};
  PhysicsConfig physics{};
  PipeConfig pipe{};
  AiConfig ai{};

  static Config Default() {
    return Config{};
  }
};

}  // namespace app
