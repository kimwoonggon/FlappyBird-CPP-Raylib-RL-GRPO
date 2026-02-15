/**
 * @file src/game/Game.h
 * @brief Declarations for Game.
 */

#pragma once

#include <vector>

#include <raylib.h>

#include "app/Config.h"
#include "game/Pipe.h"
#include "util/Rng.h"

namespace game {

/**
 * @brief High-level game state machine.
 */
enum class GameState {
  kReady = 0,
  kPlaying = 1,
  kGameOver = 2,
};

/**
 * @brief Runtime bird state used for physics and animation.
 */
struct BirdState {
  Vector2 position{};
  float velocity = 0.0F;
  int frame = 0;
  int frameCounter = 0;
  bool isFlapped = false;
  int frameIndex = 0;
};

/**
 * @brief Owns gameplay simulation state for one Flappy Bird run.
 */
class Game {
 public:
  /**
   * @brief Creates game simulation object.
   * @param config Runtime configuration.
   * @param rng Shared random generator for pipe generation.
   * @param birdWidth Bird sprite width.
   * @param birdHeight Bird sprite height.
   * @param pipeWidth Pipe sprite width.
   * @param pipeHeight Pipe sprite height.
   */
  Game(const app::Config& config,
       util::Rng& rng,
       int birdWidth,
       int birdHeight,
       int pipeWidth,
       int pipeHeight);

  /**
   * @brief Resets game to ready state.
   */
  void Reset();
  /**
   * @brief Starts simulation from ready state.
   */
  void Start();
  /**
   * @brief Advances game simulation by one frame.
   * @param flapRequested Whether flap action is triggered this frame.
   */
  void Update(bool flapRequested);
  /**
   * @brief Forces game into game-over state.
   */
  void SetGameOver();

  /**
   * @brief Returns current finite-state-machine status.
   * @return Current game state.
   */
  GameState State() const { return state_; }
  /**
   * @brief Returns current bird state.
   * @return Bird state snapshot.
   */
  const BirdState& Bird() const { return bird_; }
  /**
   * @brief Returns active obstacle list.
   * @return Pipe list.
   */
  const std::vector<Pipe>& Pipes() const { return pipes_; }

  /**
   * @brief Returns current score.
   * @return Number of cleared pipes.
   */
  int Score() const { return score_; }
  /**
   * @brief Returns historical best score.
   * @return High score value.
   */
  int HighScore() const { return highScore_; }

 private:
  /** @brief Applies per-frame bird physics update. */
  void StepPhysics(bool flapRequested);
  /** @brief Advances bird animation counters and frame index. */
  void StepAnimation();
  /** @brief Updates pipe positions, spawn, and removal logic. */
  void StepPipes();
  /** @brief Updates score/high-score when passing pipes. */
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
