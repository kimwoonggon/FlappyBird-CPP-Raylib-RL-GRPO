/**
 * @file src/app/Config.h
 * @brief Declarations for Config.
 */

#pragma once

#include <string>

namespace app {

/**
 * @brief Preferred inference backend for ONNX Runtime sessions.
 */
enum class InferenceBackend {
  kAuto,
  kCpu,
  kCoreMl
};

/**
 * @brief Preferred preprocessing backend used before inference.
 */
enum class PreprocessBackend {
  kAuto,
  kCpu,
  kGpuShader
};

/**
 * @brief Window and viewport parameters for runtime rendering.
 */
struct ScreenConfig {
  int width = 288;
  int height = 512;
  int fps = 30;
  float baseY = 512.0F * 0.79F;
};

/**
 * @brief Bird physics constants used by the game simulation.
 */
struct PhysicsConfig {
  float gravity = 1.0F;
  float jumpVelocity = -9.0F;
  float maxVelocityY = 10.0F;
};

/**
 * @brief Pipe generation and scoring parameters.
 */
struct PipeConfig {
  int gapSize = 110;
  float velocityX = -4.0F;
  int spawnOffsetX = 10;
  int lowerStartClamp = 260;
  int scoreWindow = 5;
};

/**
 * @brief AI input and backend configuration.
 */
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

/**
 * @brief Root runtime configuration object.
 */
struct Config {
  ScreenConfig screen{};
  PhysicsConfig physics{};
  PipeConfig pipe{};
  AiConfig ai{};

  /**
   * @brief Builds a config object using repository defaults.
   * @return Default-initialized configuration.
   */
  static Config Default() {
    return Config{};
  }
};

}  // namespace app
