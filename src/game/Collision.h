#pragma once

#include <cstdint>
#include <vector>

#include <raylib.h>

namespace game {

struct Rect {
  int x;
  int y;
  int w;
  int h;

  bool IsValid() const {
    return w > 0 && h > 0;
  }
};

class HitMask {
 public:
  HitMask() = default;
  HitMask(int width, int height, std::vector<uint8_t> solidPixels);

  static HitMask FromImage(const Image& image);

  bool At(int x, int y) const;
  int Width() const { return width_; }
  int Height() const { return height_; }

 private:
  int width_ = 0;
  int height_ = 0;
  std::vector<uint8_t> solid_;
};

namespace Collision {

Rect Intersection(const Rect& lhs, const Rect& rhs);

bool PixelPerfect(const HitMask& lhs,
                  const HitMask& rhs,
                  const Rect& overlap,
                  int lhsOffsetX,
                  int lhsOffsetY,
                  int rhsOffsetX,
                  int rhsOffsetY);

}  // namespace Collision

}  // namespace game
