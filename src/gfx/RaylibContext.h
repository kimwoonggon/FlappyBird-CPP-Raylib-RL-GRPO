/**
 * @file src/gfx/RaylibContext.h
 * @brief Declarations for RaylibContext.
 */

#pragma once

#include <raylib.h>

namespace gfx {

/**
 * @brief Owns raylib window/audio lifecycle using RAII.
 */
class RaylibContext {
 public:
  /**
   * @brief Initializes window, target FPS, and audio device.
   * @param width Window width.
   * @param height Window height.
   * @param fps Target FPS cap.
   * @param title Window title.
   */
  RaylibContext(int width, int height, int fps, const char* title) {
    InitWindow(width, height, title);
    SetTargetFPS(fps);
    InitAudioDevice();
  }

  ~RaylibContext() {
    if (IsAudioDeviceReady()) {
      CloseAudioDevice();
    }
    if (!WindowShouldClose()) {
      CloseWindow();
    } else {
      CloseWindow();
    }
  }

  RaylibContext(const RaylibContext&) = delete;
  RaylibContext& operator=(const RaylibContext&) = delete;
  RaylibContext(RaylibContext&&) = delete;
  RaylibContext& operator=(RaylibContext&&) = delete;
};

/**
 * @brief RAII helper for `BeginDrawing` / `EndDrawing`.
 */
struct DrawingScope {
  DrawingScope() { BeginDrawing(); }
  ~DrawingScope() { EndDrawing(); }
};

/**
 * @brief RAII helper for `BeginTextureMode` / `EndTextureMode`.
 */
struct TextureModeScope {
  explicit TextureModeScope(RenderTexture2D target) { BeginTextureMode(target); }
  ~TextureModeScope() { EndTextureMode(); }
};

}  // namespace gfx
