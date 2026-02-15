/**
 * @file src/app/App.Ai.cpp
 * @brief Implementation for App.Ai.
 */

#include "app/App.h"

namespace app {

/**
 * @brief Captures current frame, preprocesses it, and queries AI policy.
 * @return Action decision produced by AI agent.
 */
ai::AiDecision App::EvaluateAiDecision() {
  RenderSceneToTarget();

  if (ShouldUseGpuPreprocess()) {
    RenderPreprocessToTarget();
    Image preprocessed = LoadImageFromTexture(preprocessTarget_.Get().texture);
    ai::AiDecision decision = aiAgent_.ActPreprocessed(preprocessed);
    UnloadImage(preprocessed);
    return decision;
  }

  Image screenshot = LoadImageFromTexture(frameTarget_.Get().texture);
  ai::AiDecision decision = aiAgent_.Act(screenshot);
  UnloadImage(screenshot);
  return decision;
}

/**
 * @brief Renders the visible game scene into the source off-screen target.
 */
void App::RenderSceneToTarget() {
  gfx::TextureModeScope scope(frameTarget_.Get());
  ClearBackground(BLACK);
  DrawGameScene();
}

/**
 * @brief Runs shader preprocess pass from source target to 84x84 target.
 */
void App::RenderPreprocessToTarget() {
  if (!HasGpuPreprocessPath()) {
    return;
  }

  gfx::TextureModeScope scope(preprocessTarget_.Get());
  ClearBackground(BLACK);

  BeginShaderMode(preprocessShader_.Get());
  Rectangle source = {0.0F,
                      0.0F,
                      static_cast<float>(frameTarget_.Get().texture.width),
                      -static_cast<float>(frameTarget_.Get().texture.height)};
  Rectangle target = {0.0F,
                      0.0F,
                      static_cast<float>(config_.ai.inputSize),
                      static_cast<float>(config_.ai.inputSize)};
  DrawTexturePro(frameTarget_.Get().texture, source, target, Vector2{0.0F, 0.0F}, 0.0F, WHITE);
  EndShaderMode();
}

/**
 * @brief Checks whether shader preprocess resources are fully initialized.
 * @return True when render target, shader, and uniforms are valid.
 */
bool App::HasGpuPreprocessPath() const {
  return preprocessTarget_.IsValid() && preprocessShader_.IsValid() &&
         preprocessCropStartLoc_ >= 0 && preprocessCropRangeLoc_ >= 0;
}

/**
 * @brief Resolves whether current frame should use GPU preprocess path.
 * @return True when shader preprocess is enabled for this run.
 */
bool App::ShouldUseGpuPreprocess() const {
  if (!HasGpuPreprocessPath()) {
    return false;
  }

  if (config_.ai.preprocessBackend == PreprocessBackend::kCpu) {
    return false;
  }
  return true;
}

/**
 * @brief Primes stacked-frame AI state before active control starts.
 */
void App::WarmupAi() {
  TraceLog(LOG_INFO, "[AI] Warmup start (frames=%d).", config_.ai.frameStack);
  for (int i = 0; i < config_.ai.frameStack; ++i) {
    EvaluateAiDecision();
  }
  TraceLog(LOG_INFO, "[AI] Warmup complete; frame stack primed.");
}

/**
 * @brief Rebuilds debug overlay textures from latest stacked AI frames.
 */
void App::UpdateDebugTextures() {
  const std::vector<std::vector<float>> frames = aiAgent_.DebugFrames();
  if (frames.size() != static_cast<size_t>(config_.ai.frameStack)) {
    TraceLog(LOG_WARNING,
             "[AI] Debug frame mismatch (expected=%d actual=%zu).",
             config_.ai.frameStack,
             frames.size());
    return;
  }

  for (size_t i = 0; i < frames.size(); ++i) {
    Image image = GenImageColor(config_.ai.inputSize, config_.ai.inputSize, BLACK);

    for (int y = 0; y < config_.ai.inputSize; ++y) {
      for (int x = 0; x < config_.ai.inputSize; ++x) {
        const int index = y * config_.ai.inputSize + x;
        const float value = frames[i][static_cast<size_t>(index)];
        const Color color = value > 127.0F ? WHITE : BLACK;
        ImageDrawPixel(&image, x, y, color);
      }
    }

    ImageResize(&image, config_.ai.frameDisplaySize, config_.ai.frameDisplaySize);
    debugTextures_[i].LoadFromImage(image);
    UnloadImage(image);
  }
}

}  // namespace app
