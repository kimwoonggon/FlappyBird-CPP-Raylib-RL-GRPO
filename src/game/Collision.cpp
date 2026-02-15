/**
 * @file src/game/Collision.cpp
 * @brief Implementation for Collision.
 */

#include "game/Collision.h"

#include <algorithm>
#include <stdexcept>

namespace game {

/**
 * @brief Constructs a hit mask from dimensions and binary solid pixels.
 * @param width Mask width in pixels.
 * @param height Mask height in pixels.
 * @param solidPixels Flat solid-pixel flags.
 */
HitMask::HitMask(int width, int height, std::vector<uint8_t> solidPixels)
    : width_(width), height_(height), solid_(std::move(solidPixels)) {
  if (width_ < 0 || height_ < 0) {
    throw std::invalid_argument("mask dimensions must be non-negative");
  }
  const size_t expected = static_cast<size_t>(width_) * static_cast<size_t>(height_);
  if (solid_.size() != expected) {
    throw std::invalid_argument("solid pixel size mismatch");
  }
}

/**
 * @brief Builds hit mask from image alpha channel.
 * @param image Source image.
 * @return Hit mask where non-zero alpha pixels are solid.
 */
HitMask HitMask::FromImage(const Image& image) {
  std::vector<uint8_t> solid;
  solid.resize(static_cast<size_t>(image.width) * static_cast<size_t>(image.height), 0);

  Color* pixels = LoadImageColors(image);
  if (pixels == nullptr) {
    return HitMask(image.width, image.height, std::move(solid));
  }

  for (int y = 0; y < image.height; ++y) {
    for (int x = 0; x < image.width; ++x) {
      const int index = y * image.width + x;
      solid[static_cast<size_t>(index)] = pixels[index].a > 0 ? 1U : 0U;
    }
  }

  UnloadImageColors(pixels);
  return HitMask(image.width, image.height, std::move(solid));
}

/**
 * @brief Returns whether a local mask coordinate is solid.
 * @param x Local x coordinate.
 * @param y Local y coordinate.
 * @return True when coordinate is within bounds and solid.
 */
bool HitMask::At(int x, int y) const {
  if (x < 0 || y < 0 || x >= width_ || y >= height_) {
    return false;
  }
  const int index = y * width_ + x;
  return solid_[static_cast<size_t>(index)] != 0;
}

namespace Collision {

/**
 * @brief Computes intersection rectangle between two axis-aligned rectangles.
 * @param lhs Left-hand rectangle.
 * @param rhs Right-hand rectangle.
 * @return Overlap rectangle (may be empty).
 */
Rect Intersection(const Rect& lhs, const Rect& rhs) {
  const int x1 = std::max(lhs.x, rhs.x);
  const int y1 = std::max(lhs.y, rhs.y);
  const int x2 = std::min(lhs.x + lhs.w, rhs.x + rhs.w);
  const int y2 = std::min(lhs.y + lhs.h, rhs.y + rhs.h);

  return Rect{x1, y1, std::max(0, x2 - x1), std::max(0, y2 - y1)};
}

/**
 * @brief Checks overlap region for any pair of solid pixels.
 * @param lhs Left hit mask.
 * @param rhs Right hit mask.
 * @param overlap Overlap rectangle in world coordinates.
 * @param lhsOffsetX Left mask world x origin.
 * @param lhsOffsetY Left mask world y origin.
 * @param rhsOffsetX Right mask world x origin.
 * @param rhsOffsetY Right mask world y origin.
 * @return True when any overlapping solid pixels collide.
 */
bool PixelPerfect(const HitMask& lhs,
                  const HitMask& rhs,
                  const Rect& overlap,
                  int lhsOffsetX,
                  int lhsOffsetY,
                  int rhsOffsetX,
                  int rhsOffsetY) {
  if (!overlap.IsValid()) {
    return false;
  }

  for (int y = 0; y < overlap.h; ++y) {
    for (int x = 0; x < overlap.w; ++x) {
      const int worldX = overlap.x + x;
      const int worldY = overlap.y + y;

      const int lhsX = worldX - lhsOffsetX;
      const int lhsY = worldY - lhsOffsetY;
      const int rhsX = worldX - rhsOffsetX;
      const int rhsY = worldY - rhsOffsetY;

      if (lhs.At(lhsX, lhsY) && rhs.At(rhsX, rhsY)) {
        return true;
      }
    }
  }

  return false;
}

}  // namespace Collision

}  // namespace game
