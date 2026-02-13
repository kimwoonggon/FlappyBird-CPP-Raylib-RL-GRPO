#pragma once

#include <vector>

#include <raylib.h>

#include "app/Config.h"
#include "game/Pipe.h"
#include "util/Rng.h"

namespace game {

enum class GameState {
  kReady = 0,
  kPlaying = 1,
  kGameOver = 2,
};

struct BirdState {
  Vector2 position{};
  float velocity = 0.0F;
  int frame = 0;
  int frameCounter = 0;
  bool isFlapped = false;
  int frameIndex = 0;
};

class Game {
 public:
  Game(const app::Config& config,
       util::Rng& rng,
       int birdWidth,
       int birdHeight,
       int pipeWidth,
       int pipeHeight);

  void Reset();
  void Start();
  void Update(bool flapRequested);
  void SetGameOver();

  GameState State() const { return state_; }
  const BirdState& Bird() const { return bird_; }
  const std::vector<Pipe>& Pipes() const { return pipes_; }

  int Score() const { return score_; }
  int HighScore() const { return highScore_; }

 private:
  void StepPhysics(bool flapRequested);
  void StepAnimation();
  void StepPipes();
  void StepScore();

  const app::Config config_;
  util::Rng& rng_;

  BirdState bird_{};
  std::vector<Pipe> pipes_;
  GameState state_ = GameState::kReady;

  int score_ = 0;
  int highScore_ = 0;

  int birdWidth_ = 0;
  int birdHeight_ = 0;
  int pipeWidth_ = 0;
  int pipeHeight_ = 0;
};

}  // namespace game
