/**
 * @file src/app/App.Init.cpp
 * @brief Implementation for App.Init.
 */

#include "app/App.h"

#include <algorithm>

namespace app {
namespace {
constexpr const char* kPreprocessFragmentShader = R"glsl(
#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;
uniform float cropStart;
uniform float cropRange;

void main() {
  vec2 uv = vec2(fragTexCoord.x, cropStart + (fragTexCoord.y * cropRange));
  vec4 src = texture(texture0, uv);
  float gray = dot(src.rgb, vec3(0.299, 0.587, 0.114));
  finalColor = vec4(gray, gray, gray, 1.0);
}
)glsl";

/**
 * @brief Converts inference backend enum to log-friendly text.
 * @param backend Inference backend enum.
 * @return Backend label string.
 */
const char* InferenceBackendToString(InferenceBackend backend) {
  switch (backend) {
    case InferenceBackend::kAuto:
      return "auto";
    case InferenceBackend::kCpu:
      return "cpu";
    case InferenceBackend::kCoreMl:
      return "coreml";
  }
  return "unknown";
}

/**
 * @brief Converts preprocess backend enum to log-friendly text.
 * @param backend Preprocess backend enum.
 * @return Backend label string.
 */
const char* PreprocessBackendToString(PreprocessBackend backend) {
  switch (backend) {
    case PreprocessBackend::kAuto:
      return "auto";
    case PreprocessBackend::kCpu:
      return "cpu";
    case PreprocessBackend::kGpuShader:
      return "shader";
  }
  return "unknown";
}
}  // namespace

/**
 * @brief Emits startup configuration values and requested AI backends.
 */
void App::LogStartupConfig() const {
  TraceLog(LOG_INFO,
           "[App] Initializing screen=%dx%d fps=%d aiFrame=%d model=%s",
           config_.screen.width,
           config_.screen.height,
           config_.screen.fps,
           config_.ai.inputSize,
           config_.ai.modelPath.c_str());
  TraceLog(LOG_INFO,
           "[AI] Requested inference=%s preprocess=%s",
           InferenceBackendToString(config_.ai.inferenceBackend),
           PreprocessBackendToString(config_.ai.preprocessBackend));
}

/**
 * @brief Loads sprite and audio assets required by the game.
 */
void App::InitializeAssets() {
  jumpSound_.Load("assets/audio/jump.mp3");
  bg_.Load("assets/sprites/background-black.png");
  base_.Load("assets/sprites/base.png");
  pipe_.Load("assets/sprites/pipe-green.png");
  pipeReverse_.Load("assets/sprites/pipe-green-reverse.png");
  TraceLog(LOG_INFO, "[Assets] Core textures and sound loaded.");

  birdTextures_[0].Load("assets/sprites/redbird-upflap.png");
  birdTextures_[1].Load("assets/sprites/redbird-midflap.png");
  birdTextures_[2].Load("assets/sprites/redbird-downflap.png");
  TraceLog(LOG_INFO, "[Assets] Bird texture set loaded (frames=3).");
}

/**
 * @brief Generates alpha hit masks from loaded bird and pipe images.
 */
void App::InitializeCollisionMasks() {
  gfx::ImageResource birdUpImage("assets/sprites/redbird-upflap.png");
  gfx::ImageResource birdMidImage("assets/sprites/redbird-midflap.png");
  gfx::ImageResource birdDownImage("assets/sprites/redbird-downflap.png");
  gfx::ImageResource pipeImage("assets/sprites/pipe-green.png");
  gfx::ImageResource pipeReverseImage("assets/sprites/pipe-green-reverse.png");

  birdMasks_[0] = game::HitMask::FromImage(birdUpImage.Get());
  birdMasks_[1] = game::HitMask::FromImage(birdMidImage.Get());
  birdMasks_[2] = game::HitMask::FromImage(birdDownImage.Get());
  pipeMask_ = game::HitMask::FromImage(pipeImage.Get());
  pipeReverseMask_ = game::HitMask::FromImage(pipeReverseImage.Get());
  TraceLog(LOG_INFO, "[Assets] Hit masks generated for bird frames and pipes.");
}

/**
 * @brief Creates gameplay state object using loaded asset dimensions.
 */
void App::InitializeGame() {
  game_ = std::make_unique<game::Game>(
      config_,
      rng_,
      birdTextures_[0].Get().width,
      birdTextures_[0].Get().height,
      pipe_.Get().width,
      pipe_.Get().height);
  TraceLog(LOG_INFO,
           "[Game] Game object ready (bird=%dx%d pipe=%dx%d).",
           birdTextures_[0].Get().width,
           birdTextures_[0].Get().height,
           pipe_.Get().width,
           pipe_.Get().height);
}

/**
 * @brief Initializes AI render targets and shader preprocessing pipeline.
 */
void App::InitializeAiPipeline() {
  frameTarget_.Load(config_.screen.width, config_.screen.height);
  TraceLog(LOG_INFO,
           "[AI] Frame target allocated (%dx%d).",
           config_.screen.width,
           config_.screen.height);

  preprocessTarget_.Load(config_.ai.inputSize, config_.ai.inputSize);
  preprocessShader_.LoadFromMemory(nullptr, kPreprocessFragmentShader);

  if (!preprocessTarget_.IsValid() || !preprocessShader_.IsValid()) {
    if (config_.ai.preprocessBackend == PreprocessBackend::kGpuShader) {
      TraceLog(LOG_WARNING,
               "[AI] GPU preprocess was requested but unavailable, forcing CPU preprocessing.");
    } else {
      TraceLog(LOG_WARNING, "[AI] GPU preprocess unavailable, falling back to CPU preprocessing.");
    }
    return;
  }

  preprocessCropStartLoc_ = GetShaderLocation(preprocessShader_.Get(), "cropStart");
  preprocessCropRangeLoc_ = GetShaderLocation(preprocessShader_.Get(), "cropRange");

  const float sourceHeight = static_cast<float>(config_.screen.height);
  const float cropStart = std::clamp(static_cast<float>(config_.ai.cropY) / sourceHeight, 0.0F, 1.0F);
  const float cropEnd = std::clamp(
      static_cast<float>(config_.ai.cropY + config_.ai.cropHeight) / sourceHeight, 0.0F, 1.0F);
  const float cropRange = std::max(cropEnd - cropStart, 1.0F / sourceHeight);

  if (preprocessCropStartLoc_ >= 0) {
    SetShaderValue(
        preprocessShader_.Get(), preprocessCropStartLoc_, &cropStart, SHADER_UNIFORM_FLOAT);
  }
  if (preprocessCropRangeLoc_ >= 0) {
    SetShaderValue(
        preprocessShader_.Get(), preprocessCropRangeLoc_, &cropRange, SHADER_UNIFORM_FLOAT);
  }

  TraceLog(LOG_INFO,
           "[AI] GPU preprocess enabled (target=%dx%d cropStart=%.3f cropRange=%.3f).",
           config_.ai.inputSize,
           config_.ai.inputSize,
           cropStart,
           cropRange);
}

/**
 * @brief Allocates per-frame debug textures used by AI HUD.
 */
void App::InitializeDebugTextures() {
  for (gfx::TextureResource& texture : debugTextures_) {
    Image emptyImage = GenImageColor(config_.ai.frameDisplaySize, config_.ai.frameDisplaySize, BLACK);
    texture.LoadFromImage(emptyImage);
    UnloadImage(emptyImage);
  }
  TraceLog(LOG_INFO,
           "[AI] Debug textures prepared (slots=%zu display=%dx%d).",
           debugTextures_.size(),
           config_.ai.frameDisplaySize,
           config_.ai.frameDisplaySize);
}

}  // namespace app
