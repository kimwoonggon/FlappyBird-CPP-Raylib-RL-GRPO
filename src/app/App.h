/**
 * @file src/app/App.h
 * @brief Declarations for App.
 */

#pragma once

#include <array>
#include <memory>

#include "ai/AiAgent.h"
#include "ai/OnnxPolicy.h"
#include "app/Config.h"
#include "game/Collision.h"
#include "game/Game.h"
#include "gfx/RaylibContext.h"
#include "gfx/Resources.h"
#include "util/Rng.h"

namespace app {

/**
 * @brief Coordinates runtime loop, rendering, gameplay, and AI control.
 */
class App {
 public:
  /**
   * @brief Constructs full application runtime.
   * @param config Runtime configuration.
   */
  explicit App(const Config& config);

  /**
   * @brief Executes main game loop until window closes.
   * @return Process exit code.
   */
  int Run();

 private:
  /** @brief Handles global key input such as AI toggle and restart. */
  void HandleGlobalInput();
  /** @brief Logs startup configuration for diagnostics. */
  void LogStartupConfig() const;
  /** @brief Loads textures and audio resources. */
  void InitializeAssets();
  /** @brief Builds pixel masks for collision detection. */
  void InitializeCollisionMasks();
  /** @brief Creates gameplay object after assets are ready. */
  void InitializeGame();
  /** @brief Initializes render targets and shader used by AI preprocessing. */
  void InitializeAiPipeline();
  /** @brief Creates textures used by AI debug HUD. */
  void InitializeDebugTextures();
  /** @brief Advances one simulation step. */
  void Update();
  /** @brief Evaluates one AI decision for current frame. */
  ai::AiDecision EvaluateAiDecision();
  /** @brief Applies physics update and collision transition checks. */
  void UpdateWorld(bool flapRequested);
  /** @brief Draws one frame to the window. */
  void Draw();
  /** @brief Draws non-AI HUD elements. */
  void DrawHud() const;
  /** @brief Draws AI probability and frame stack overlay. */
  void DrawAiDebugHud();

  /** @brief Draws gameplay scene to currently active render target. */
  void DrawGameScene() const;
  /** @brief Renders scene into AI source render target. */
  void RenderSceneToTarget();
  /** @brief Applies GPU preprocessing and writes into model-sized render target. */
  void RenderPreprocessToTarget();
  /** @brief Checks whether GPU preprocessing resources are usable. */
  bool HasGpuPreprocessPath() const;
  /** @brief Resolves runtime choice between CPU and GPU preprocess paths. */
  bool ShouldUseGpuPreprocess() const;
  /** @brief Warms AI frame stack before active gameplay. */
  void WarmupAi();
  /** @brief Updates AI debug textures from latest frame stack. */
  void UpdateDebugTextures();

  /** @brief Runs bounding-box and pixel-perfect collision checks. */
  bool CheckPipeCollisions();

  // Core runtime state and dependencies.
  Config config_;
  util::Rng rng_;
  gfx::RaylibContext context_;

  // Cached textures/audio for rendering and SFX.
  gfx::TextureResource bg_;
  gfx::TextureResource base_;
  gfx::TextureResource pipe_;
  gfx::TextureResource pipeReverse_;
  std::array<gfx::TextureResource, 3> birdTextures_;
  gfx::SoundResource jumpSound_;

  // Hit masks for pixel-perfect collision detection.
  std::array<game::HitMask, 3> birdMasks_;
  game::HitMask pipeMask_;
  game::HitMask pipeReverseMask_;

  // Main gameplay and AI controllers.
  std::unique_ptr<game::Game> game_;

  ai::OnnxPolicy policy_;
  ai::AiAgent aiAgent_;

  // Off-screen render target plus textures shown in the AI debug HUD.
  gfx::RenderTextureResource frameTarget_;
  gfx::RenderTextureResource preprocessTarget_;
  gfx::ShaderResource preprocessShader_;
  int preprocessCropStartLoc_ = -1;
  int preprocessCropRangeLoc_ = -1;
  std::array<gfx::TextureResource, 4> debugTextures_;

  // Runtime flags for AI control and its temporary UI cues.
  bool aiControl_ = false;
  bool showAiFlap_ = false;
  int aiFlapCounter_ = 0;
};

}  // namespace app
