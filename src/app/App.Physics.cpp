/**
 * @file src/app/App.Physics.cpp
 * @brief Implementation for App.Physics.
 */

#include "app/App.h"

#include <algorithm>

namespace app {

/**
 * @brief Updates gameplay state and applies collision-driven transitions.
 * @param flapRequested Whether flap should be applied this frame.
 */
void App::UpdateWorld(bool flapRequested) {
  game_->Update(flapRequested);
  TraceLog(LOG_DEBUG,
           "[Update] Frame advanced (score=%d y=%.2f vel=%.2f).",
           game_->Score(),
           game_->Bird().position.y,
           game_->Bird().velocity);

  if (CheckPipeCollisions()) {
    game_->SetGameOver();
    TraceLog(LOG_WARNING,
             "[Game] Collision -> GameOver (score=%d high=%d).",
             game_->Score(),
             game_->HighScore());
  }
}

/**
 * @brief Performs bounding-box and pixel-perfect collision checks vs pipes.
 * @return True when the bird collides with any pipe.
 */
bool App::CheckPipeCollisions() {
  if (game_->State() != game::GameState::kPlaying) {
    return false;
  }

  const game::BirdState& bird = game_->Bird();
  const int birdFrame = std::clamp(bird.frame, 0, 2);

  game::Rect birdRect{static_cast<int>(bird.position.x),
                      static_cast<int>(bird.position.y),
                      birdTextures_[birdFrame].Get().width,
                      birdTextures_[birdFrame].Get().height};

  for (const game::Pipe& pipe : game_->Pipes()) {
    game::Rect upperRect{pipe.xUpper, pipe.yUpper, pipe_.Get().width, pipe_.Get().height};
    game::Rect lowerRect{pipe.xLower, pipe.yLower, pipe_.Get().width, pipe_.Get().height};

    Rectangle birdBox{static_cast<float>(birdRect.x), static_cast<float>(birdRect.y), static_cast<float>(birdRect.w), static_cast<float>(birdRect.h)};
    Rectangle upperBox{static_cast<float>(upperRect.x), static_cast<float>(upperRect.y), static_cast<float>(upperRect.w), static_cast<float>(upperRect.h)};
    Rectangle lowerBox{static_cast<float>(lowerRect.x), static_cast<float>(lowerRect.y), static_cast<float>(lowerRect.w), static_cast<float>(lowerRect.h)};

    if (CheckCollisionRecs(birdBox, upperBox)) {
      const game::Rect overlap = game::Collision::Intersection(birdRect, upperRect);
      if (game::Collision::PixelPerfect(
              birdMasks_[birdFrame], pipeReverseMask_, overlap, birdRect.x, birdRect.y, upperRect.x, upperRect.y)) {
        TraceLog(LOG_INFO,
                 "[Collision] Upper pipe hit (pipeX=%d bird=(%d,%d) overlap=%dx%d).",
                 pipe.xUpper,
                 birdRect.x,
                 birdRect.y,
                 overlap.w,
                 overlap.h);
        return true;
      }
    }

    if (CheckCollisionRecs(birdBox, lowerBox)) {
      const game::Rect overlap = game::Collision::Intersection(birdRect, lowerRect);
      if (game::Collision::PixelPerfect(
              birdMasks_[birdFrame], pipeMask_, overlap, birdRect.x, birdRect.y, lowerRect.x, lowerRect.y)) {
        TraceLog(LOG_INFO,
                 "[Collision] Lower pipe hit (pipeX=%d bird=(%d,%d) overlap=%dx%d).",
                 pipe.xLower,
                 birdRect.x,
                 birdRect.y,
                 overlap.w,
                 overlap.h);
        return true;
      }
    }
  }

  return false;
}

}  // namespace app
