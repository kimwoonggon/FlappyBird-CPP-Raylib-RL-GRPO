#include "game/Game.h"

#include <algorithm>

namespace game {
namespace {
constexpr int kAnimationFrames[4] = {0, 1, 2, 1};
constexpr int kAnimationLength = 4;
}  // namespace

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

void Game::Start() {
  if (state_ == GameState::kReady) {
    state_ = GameState::kPlaying;
  }
}

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

void Game::SetGameOver() {
  state_ = GameState::kGameOver;
}

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

void Game::StepAnimation() {
  ++bird_.frameCounter;
  if (bird_.frameCounter >= 3) {
    bird_.frameIndex = (bird_.frameIndex + 1) % kAnimationLength;
    bird_.frame = kAnimationFrames[bird_.frameIndex];
    bird_.frameCounter = 0;
  }
}

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
