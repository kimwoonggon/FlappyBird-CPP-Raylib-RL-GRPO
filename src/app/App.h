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

// App wires together the renderer, gameplay loop, and optional ONNX-powered AI control.
class App {
 public:
  explicit App(const Config& config);
  int Run();

 private:
  // Handles toggles such as AI enablement and restarting the run.
  void HandleGlobalInput();
  // Advances the simulation and feeds actions from either the player or policy.
  void Update();
  // Renders the current scene plus overlays in the main window.
  void Draw();

  // Draws all world elements (background, pipes, bird) to the active render target.
  void DrawGameScene() const;
  // Renders the scene into an off-screen texture that feeds the AI policy.
  void RenderSceneToTarget();
  // Steps the agent a few frames so its frame stack is fully populated.
  void WarmupAi();
  // Uploads the latest preprocessed AI frames to textures for on-screen debugging.
  void UpdateDebugTextures();

  // Runs bounding-box and pixel-perfect checks against every active pipe.
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
  std::array<gfx::TextureResource, 4> debugTextures_;

  // Runtime flags for AI control and its temporary UI cues.
  bool aiControl_ = false;
  bool showAiFlap_ = false;
  int aiFlapCounter_ = 0;
};

}  // namespace app
