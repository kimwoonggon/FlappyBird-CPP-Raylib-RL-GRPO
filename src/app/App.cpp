#include "app/App.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

// Implements the high-level application loop plus AI instrumentation.

namespace app {
namespace {
constexpr int kFlapDisplayFrames = 15;

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

App::App(const Config& config)
    : config_(config),
      rng_(static_cast<uint32_t>(GetTime() * 1000000.0)),
      context_(config.screen.width, config.screen.height, config.screen.fps, "FlappyBird-RL"),
      policy_(config.ai.modelPath, config.ai.inputSize, config.ai.frameStack),
      aiAgent_(config_, &policy_, &rng_) {
  SetTraceLogLevel(LOG_INFO);
  TraceLog(LOG_INFO,
           "[App] Initializing screen=%dx%d fps=%d aiFrame=%d model=%s",
           config_.screen.width,
           config_.screen.height,
           config_.screen.fps,
           config_.ai.inputSize,
           config_.ai.modelPath.c_str());

  // Load all assets up-front so the main loop stays real-time safe.
  jumpSound_.Load("assets/audio/jump.mp3");
  bg_.Load("assets/sprites/background-black.png");
  base_.Load("assets/sprites/base.png");
  pipe_.Load("assets/sprites/pipe-green.png");
  pipeReverse_.Load("assets/sprites/pipe-green-reverse.png");
  TraceLog(LOG_INFO, "[Assets] Core textures and sound loaded.");

  birdTextures_[0].Load("assets/sprites/redbird-upflap.png");
  birdTextures_[1].Load("assets/sprites/redbird-midflap.png");
  birdTextures_[2].Load("assets/sprites/redbird-downflap.png");
  TraceLog(LOG_INFO, "[Assets] Bird texture set loaded (frames=3).");

  gfx::ImageResource birdUpImage("assets/sprites/redbird-upflap.png");
  gfx::ImageResource birdMidImage("assets/sprites/redbird-midflap.png");
  gfx::ImageResource birdDownImage("assets/sprites/redbird-downflap.png");
  gfx::ImageResource pipeImage("assets/sprites/pipe-green.png");
  gfx::ImageResource pipeReverseImage("assets/sprites/pipe-green-reverse.png");

  // Build masks once so runtime collisions can stay precise yet fast.
  birdMasks_[0] = game::HitMask::FromImage(birdUpImage.Get());
  birdMasks_[1] = game::HitMask::FromImage(birdMidImage.Get());
  birdMasks_[2] = game::HitMask::FromImage(birdDownImage.Get());
  pipeMask_ = game::HitMask::FromImage(pipeImage.Get());
  pipeReverseMask_ = game::HitMask::FromImage(pipeReverseImage.Get());
  TraceLog(LOG_INFO, "[Assets] Hit masks generated for bird frames and pipes.");

  // The gameplay object needs sprite sizes to clamp movement and detect hits.
  game_ = std::make_unique<game::Game>(
      config_,
      rng_,
      birdTextures_[0].Get().width,
      birdTextures_[0].Get().height,
      pipe_.Get().width,
      pipe_.Get().height);
  TraceLog(LOG_INFO,
           "[Game] Game object ready (bird=%dx%d pipe=%dx%d).",
           birdTextures_[0].Get().width,
           birdTextures_[0].Get().height,
           pipe_.Get().width,
           pipe_.Get().height);

  // Off-screen frame buffer used as the AI's observation space.
  frameTarget_.Load(config_.screen.width, config_.screen.height);
  TraceLog(LOG_INFO,
           "[AI] Frame target allocated (%dx%d).",
           config_.screen.width,
           config_.screen.height);

  for (gfx::TextureResource& texture : debugTextures_) {
    Image emptyImage = GenImageColor(config_.ai.frameDisplaySize, config_.ai.frameDisplaySize, BLACK);
    texture.LoadFromImage(emptyImage);
    UnloadImage(emptyImage);
  }
  TraceLog(LOG_INFO,
           "[AI] Debug textures prepared (slots=%zu display=%dx%d).",
           debugTextures_.size(),
           config_.ai.frameDisplaySize,
           config_.ai.frameDisplaySize);
}

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

void App::Update() {
  if (game_->State() != game::GameState::kPlaying) {
    return;
  }

  bool flapRequested = false;

  // When AI control is live, render to the off-screen buffer and query the policy.
  if (aiControl_ && policy_.HasModel()) {
    RenderSceneToTarget();
    Image screenshot = LoadImageFromTexture(frameTarget_.Get().texture);
    ai::AiDecision decision = aiAgent_.Act(screenshot);
    UnloadImage(screenshot);

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

  // Progress the world and immediately terminate the run if a collision is detected.
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

    // Reject obvious misses using AABB checks before pixel-perfect masks.
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

void App::DrawGameScene() const {
  // Draw backdrop, all pipes, scrolling base, then the animated bird.
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

void App::RenderSceneToTarget() {
  gfx::TextureModeScope scope(frameTarget_.Get());
  ClearBackground(BLACK);
  DrawGameScene();
}

void App::WarmupAi() {
  // Populate the frame stack so the policy sees a full history from frame one.
  TraceLog(LOG_INFO, "[AI] Warmup start (frames=%d).", config_.ai.frameStack);
  for (int i = 0; i < config_.ai.frameStack; ++i) {
    RenderSceneToTarget();
    Image screenshot = LoadImageFromTexture(frameTarget_.Get().texture);
    aiAgent_.Act(screenshot);
    UnloadImage(screenshot);
  }
  TraceLog(LOG_INFO, "[AI] Warmup complete; frame stack primed.");
}

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

void App::Draw() {
  if (aiControl_ && policy_.HasModel()) {
    UpdateDebugTextures();
  }

  gfx::DrawingScope drawing;
  ClearBackground(BLACK);

  DrawGameScene();

  // HUD: score, high score, and AI toggle status.
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

  if (showAiFlap_) {
    DrawText("AI FLAP!", config_.screen.width / 2 - MeasureText("AI FLAP!", 24) / 2, 50, 24, YELLOW);
    --aiFlapCounter_;
    if (aiFlapCounter_ <= 0) {
      showAiFlap_ = false;
    }
  }

  if (aiControl_ && policy_.HasModel()) {
    // Display the policy's action probabilities plus the grayscale frame stack.
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
