/**
 * @file src/game/Game.cpp
 * @brief Implementation for Game.
 */

#include "game/Game.h"

#include <algorithm>

namespace game {
namespace {
constexpr int kAnimationFrames[4] = {0, 1, 2, 1};
constexpr int kAnimationLength = 4;
}  // namespace

/**
 * @brief Constructs game simulation container and initializes run state.
 * @param config Runtime configuration.
 * @param rng Shared random generator.
 * @param birdWidth Bird sprite width.
 * @param birdHeight Bird sprite height.
 * @param pipeWidth Pipe sprite width.
 * @param pipeHeight Pipe sprite height.
 */
Game::Game(const app::Config& config,
           util::Rng& rng,
           int birdWidth,
           int birdHeight,
           int pipeWidth,
           int pipeHeight)
    : config_(config),
      rng_(rng),
      birdWidth_(birdWidth),
      birdHeight_(birdHeight),
      pipeWidth_(pipeWidth),
      pipeHeight_(pipeHeight) {
  Reset();
}

/**
 * @brief Resets bird, score, and pipes to initial ready-state values.
 */
void Game::Reset() {
  bird_.position = {static_cast<float>(config_.screen.width) / 5.0F,
                    static_cast<float>(config_.screen.height) / 2.0F};
  bird_.velocity = 0.0F;
  bird_.frame = 0;
  bird_.frameCounter = 0;
  bird_.isFlapped = false;
  bird_.frameIndex = 0;

  score_ = 0;
  state_ = GameState::kReady;

  pipes_.clear();
  pipes_.push_back(GeneratePipe(config_, config_.screen.width, rng_, pipeWidth_, pipeHeight_));
  pipes_.push_back(GeneratePipe(config_, static_cast<int>(config_.screen.width * 1.5F), rng_, pipeWidth_, pipeHeight_));
}

/**
 * @brief Switches game state from ready to playing.
 */
void Game::Start() {
  if (state_ == GameState::kReady) {
    state_ = GameState::kPlaying;
  }
}

/**
 * @brief Executes one simulation tick while in playing state.
 * @param flapRequested Whether flap action is requested.
 */
void Game::Update(bool flapRequested) {
  if (state_ != GameState::kPlaying) {
    return;
  }

  StepPhysics(flapRequested);
  StepAnimation();
  StepPipes();
  StepScore();

  if (bird_.position.y + static_cast<float>(birdHeight_) >= config_.screen.baseY) {
    state_ = GameState::kGameOver;
  }
}

/**
 * @brief Forces state to game over.
 */
void Game::SetGameOver() {
  state_ = GameState::kGameOver;
}

/**
 * @brief Applies flap impulse and gravity integration for the bird.
 * @param flapRequested Whether flap is applied this frame.
 */
void Game::StepPhysics(bool flapRequested) {
  if (flapRequested) {
    bird_.velocity = config_.physics.jumpVelocity;
    bird_.isFlapped = true;
  }

  if (bird_.velocity < config_.physics.maxVelocityY && !bird_.isFlapped) {
    bird_.velocity += config_.physics.gravity;
  }

  if (bird_.isFlapped) {
    bird_.isFlapped = false;
  }

  bird_.position.y = std::max(0.0F, bird_.position.y + bird_.velocity);
}

/**
 * @brief Advances bird wing animation frame sequence.
 */
void Game::StepAnimation() {
  ++bird_.frameCounter;
  if (bird_.frameCounter >= 3) {
    bird_.frameIndex = (bird_.frameIndex + 1) % kAnimationLength;
    bird_.frame = kAnimationFrames[bird_.frameIndex];
    bird_.frameCounter = 0;
  }
}

/**
 * @brief Scrolls pipes, spawns new obstacles, and removes off-screen ones.
 */
void Game::StepPipes() {
  for (Pipe& pipe : pipes_) {
    pipe.xUpper += static_cast<int>(config_.pipe.velocityX);
    pipe.xLower += static_cast<int>(config_.pipe.velocityX);
  }

  if (!pipes_.empty() && (0 < pipes_[0].xLower && pipes_[0].xLower < 5)) {
    pipes_.push_back(GeneratePipe(config_, config_.screen.width, rng_, pipeWidth_, pipeHeight_));
  }

  if (!pipes_.empty() && pipes_[0].xLower < -pipeWidth_) {
    pipes_.erase(pipes_.begin());
  }
}

/**
 * @brief Updates score and high-score when bird crosses a pipe center.
 */
void Game::StepScore() {
  const float birdCenterX = bird_.position.x + static_cast<float>(birdWidth_) / 2.0F;
  for (const Pipe& pipe : pipes_) {
    const float pipeCenterX = static_cast<float>(pipe.xUpper + pipeWidth_ / 2);
    if (pipeCenterX < birdCenterX && birdCenterX < pipeCenterX + static_cast<float>(config_.pipe.scoreWindow)) {
      ++score_;
      highScore_ = std::max(highScore_, score_);
      break;
    }
  }
}

}  // namespace game
