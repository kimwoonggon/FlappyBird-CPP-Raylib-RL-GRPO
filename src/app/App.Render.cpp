/**
 * @file src/app/App.Render.cpp
 * @brief Implementation for App.Render.
 */

#include "app/App.h"

#include <array>
#include <cmath>
#include <cstdio>

namespace app {

/**
 * @brief Draws the full in-game scene (background, pipes, base, bird).
 */
void App::DrawGameScene() const {
  DrawTexture(bg_.Get(), 0, 0, WHITE);

  for (const game::Pipe& pipe : game_->Pipes()) {
    DrawTexture(pipeReverse_.Get(), pipe.xUpper, pipe.yUpper, WHITE);
    DrawTexture(pipe_.Get(), pipe.xLower, pipe.yLower, WHITE);
  }

  const float baseX = -(fmod(100.0F, static_cast<float>(base_.Get().width - bg_.Get().width)));
  DrawTexture(base_.Get(), static_cast<int>(baseX), static_cast<int>(config_.screen.baseY), WHITE);

  const game::BirdState& bird = game_->Bird();
  DrawTexture(birdTextures_[bird.frame].Get(), static_cast<int>(bird.position.x), static_cast<int>(bird.position.y), WHITE);
}

/**
 * @brief Draws score, high-score, and AI availability labels.
 */
void App::DrawHud() const {
  char scoreText[32];
  std::snprintf(scoreText, sizeof(scoreText), "Score: %d", game_->Score());
  DrawText(scoreText, 10, 40, 20, WHITE);

  char highScoreText[32];
  std::snprintf(highScoreText, sizeof(highScoreText), "High: %d", game_->HighScore());
  DrawText(highScoreText, 10, 65, 18, WHITE);

  if (policy_.HasModel()) {
    DrawText(aiControl_ ? "AI: ON [A key]" : "AI: OFF [A key]", 10, 10, 20, aiControl_ ? GREEN : RED);
  } else {
    DrawText("AI: OFF (model missing)", 10, 10, 20, RED);
  }
}

/**
 * @brief Draws AI probabilities and frame-stack previews when AI is active.
 */
void App::DrawAiDebugHud() {
  if (showAiFlap_) {
    DrawText("AI FLAP!", config_.screen.width / 2 - MeasureText("AI FLAP!", 24) / 2, 50, 24, YELLOW);
    --aiFlapCounter_;
    if (aiFlapCounter_ <= 0) {
      showAiFlap_ = false;
    }
  }

  if (!(aiControl_ && policy_.HasModel())) {
    return;
  }

  const std::array<float, 2> probs = aiAgent_.LastProbabilities();
  char probText[64];

  DrawRectangle(config_.screen.width - 130, 10, 120, 40, Fade(BLACK, 0.7F));

  std::snprintf(probText, sizeof(probText), "Do Nothing: %.2f", probs[0]);
  DrawText(probText, config_.screen.width - 125, 15, 10, probs[0] > probs[1] ? GREEN : WHITE);

  std::snprintf(probText, sizeof(probText), "Flap: %.2f", probs[1]);
  DrawText(probText, config_.screen.width - 125, 30, 10, probs[1] > probs[0] ? GREEN : WHITE);

  DrawRectangle(config_.screen.width - 125, 25, static_cast<int>(110.0F * probs[0]), 2, probs[0] > probs[1] ? GREEN : WHITE);
  DrawRectangle(config_.screen.width - 125, 40, static_cast<int>(110.0F * probs[1]), 2, probs[1] > probs[0] ? GREEN : WHITE);

  int frameX = 10;
  for (size_t i = 0; i < debugTextures_.size(); ++i) {
    char frameText[8];
    std::snprintf(frameText, sizeof(frameText), "%zu", i);
    DrawText(frameText, frameX + config_.ai.frameDisplaySize / 2 - 5, 90, 10, WHITE);

    if (debugTextures_[i].IsValid()) {
      DrawTexture(debugTextures_[i].Get(), frameX, 100, WHITE);
    }

    frameX += config_.ai.frameDisplaySize + 5;
  }
}

}  // namespace app
