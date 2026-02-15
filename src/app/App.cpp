/**
 * @file src/app/App.cpp
 * @brief Implementation for App.
 */

#include "app/App.h"

#include <cstdio>

namespace app {
namespace {
constexpr int kFlapDisplayFrames = 15;

/**
 * @brief Converts game state enum to a human-readable label.
 * @param state Game state enum value.
 * @return Static text label for logs.
 */
const char* GameStateToString(game::GameState state) {
  switch (state) {
    case game::GameState::kReady:
      return "Ready";
    case game::GameState::kPlaying:
      return "Playing";
    case game::GameState::kGameOver:
      return "GameOver";
  }
  return "Unknown";
}
}  // namespace

/**
 * @brief Builds application runtime and initializes all subsystems.
 * @param config Runtime configuration object.
 */
App::App(const Config& config)
    : config_(config),
      rng_(static_cast<uint32_t>(GetTime() * 1000000.0)),
      context_(config.screen.width, config.screen.height, config.screen.fps, "FlappyBird-RL"),
      policy_(config.ai.modelPath,
              config.ai.inputSize,
              config.ai.frameStack,
              config.ai.inferenceBackend),
      aiAgent_(config_, &policy_, &rng_) {
  SetTraceLogLevel(LOG_INFO);
  LogStartupConfig();
  InitializeAssets();
  InitializeCollisionMasks();
  InitializeGame();
  InitializeAiPipeline();
  InitializeDebugTextures();
}

/**
 * @brief Runs the main loop until the window is closed.
 * @return Process exit code.
 */
int App::Run() {
  // Standard game loop: process input, advance simulation, render frame.
  TraceLog(LOG_INFO, "[App] Entering Run loop.");
  while (!WindowShouldClose()) {
    HandleGlobalInput();
    Update();
    Draw();
  }
  TraceLog(LOG_INFO, "[App] Run loop terminated (window closed).");
  return 0;
}

/**
 * @brief Handles global one-shot input keys such as AI toggle and reset.
 */
void App::HandleGlobalInput() {
  // Allow the user to toggle AI control or reset the run without leaving play mode.
  if (policy_.HasModel() && IsKeyPressed(KEY_A)) {
    aiControl_ = !aiControl_;
    aiAgent_.Reset();
    TraceLog(LOG_INFO, "[Input] AI control %s.", aiControl_ ? "enabled" : "disabled");
    if (aiControl_) {
      WarmupAi();
    }
  }

  if (IsKeyPressed(KEY_SPACE)) {
    if (game_->State() == game::GameState::kReady) {
      game_->Start();
      TraceLog(LOG_INFO, "[Input] Game start requested (state=%s -> Playing).", GameStateToString(game::GameState::kReady));
    } else if (game_->State() == game::GameState::kGameOver) {
      game_->Reset();
      aiAgent_.Reset();
      showAiFlap_ = false;
      aiFlapCounter_ = 0;
      TraceLog(LOG_INFO,
               "[Input] Game reset after GameOver (high=%d).",
               game_->HighScore());
    }
  }
}

/**
 * @brief Advances one gameplay frame including AI action resolution.
 */
void App::Update() {
  if (game_->State() != game::GameState::kPlaying) {
    return;
  }

  bool flapRequested = false;

  // When AI control is live, render to the off-screen buffer and query the policy.
  if (aiControl_ && policy_.HasModel()) {
    const ai::AiDecision decision = EvaluateAiDecision();

    flapRequested = decision.action == 1;
    TraceLog(LOG_DEBUG,
             "[AI] Decision action=%d probs=[%.3f, %.3f]",
             decision.action,
             decision.probabilities[0],
             decision.probabilities[1]);

    if (flapRequested) {
      showAiFlap_ = true;
      aiFlapCounter_ = kFlapDisplayFrames;
      TraceLog(LOG_INFO,
               "[AI] Flap triggered (prob=%.3f score=%d).",
               decision.probabilities[1],
               game_->Score());
    }
  } else if (IsKeyPressed(KEY_SPACE)) {
    flapRequested = true;
    TraceLog(LOG_INFO, "[Input] Player flap request accepted (score=%d).", game_->Score());
  }

  if (flapRequested && jumpSound_.IsValid()) {
    PlaySound(jumpSound_.Get());
  }

  UpdateWorld(flapRequested);
}

/**
 * @brief Draws one complete frame, including HUD and overlays.
 */
void App::Draw() {
  if (aiControl_ && policy_.HasModel()) {
    UpdateDebugTextures();
  }

  gfx::DrawingScope drawing;
  ClearBackground(BLACK);

  DrawGameScene();
  DrawHud();
  DrawAiDebugHud();

  if (game_->State() == game::GameState::kReady) {
    DrawText("Press SPACE to start",
             config_.screen.width / 2 - MeasureText("Press SPACE to start", 20) / 2,
             config_.screen.height / 2,
             20,
             WHITE);
    TraceLog(LOG_DEBUG, "[HUD] Showing Ready prompt.");
  }

  if (game_->State() == game::GameState::kGameOver) {
    DrawText("Press SPACE to restart",
             config_.screen.width / 2 - MeasureText("Press SPACE to restart", 20) / 2,
             config_.screen.height / 2,
             20,
             WHITE);
    TraceLog(LOG_DEBUG, "[HUD] Showing GameOver prompt.");
  }
}

}  // namespace app
