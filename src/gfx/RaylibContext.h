#pragma once

#include <raylib.h>

namespace gfx {

class RaylibContext {
 public:
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

struct DrawingScope {
  DrawingScope() { BeginDrawing(); }
  ~DrawingScope() { EndDrawing(); }
};

struct TextureModeScope {
  explicit TextureModeScope(RenderTexture2D target) { BeginTextureMode(target); }
  ~TextureModeScope() { EndTextureMode(); }
};

}  // namespace gfx
