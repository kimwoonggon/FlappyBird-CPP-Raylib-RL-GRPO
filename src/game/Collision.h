/**
 * @file src/game/Collision.h
 * @brief Declarations for Collision.
 */

#pragma once

#include <cstdint>
#include <vector>

#include <raylib.h>

namespace game {

/**
 * @brief Integer rectangle helper for overlap tests.
 */
struct Rect {
  int x;
  int y;
  int w;
  int h;

  /**
   * @brief Checks if rectangle has positive area.
   * @return True when both width and height are positive.
   */
  bool IsValid() const {
    return w > 0 && h > 0;
  }
};

/**
 * @brief Binary occupancy mask for pixel-perfect collision tests.
 */
class HitMask {
 public:
  HitMask() = default;
  /**
   * @brief Builds a mask from explicit occupancy data.
   * @param width Mask width.
   * @param height Mask height.
   * @param solidPixels Flattened occupancy values.
   */
  HitMask(int width, int height, std::vector<uint8_t> solidPixels);

  /**
   * @brief Builds a mask by thresholding non-transparent image pixels.
   * @param image Source sprite image.
   * @return Generated hit mask.
   */
  static HitMask FromImage(const Image& image);

  /**
   * @brief Reads occupancy bit at given coordinate.
   * @param x Horizontal index.
   * @param y Vertical index.
   * @return True if mask is solid at coordinate.
   */
  bool At(int x, int y) const;
  /**
   * @brief Returns mask width.
   * @return Width in pixels.
   */
  int Width() const { return width_; }
  /**
   * @brief Returns mask height.
   * @return Height in pixels.
   */
  int Height() const { return height_; }

 private:
  int width_ = 0;
  int height_ = 0;
  std::vector<uint8_t> solid_;
};

namespace Collision {

/**
 * @brief Computes overlap rectangle between two axis-aligned rectangles.
 * @param lhs First rectangle.
 * @param rhs Second rectangle.
 * @return Intersection rectangle (may be invalid).
 */
Rect Intersection(const Rect& lhs, const Rect& rhs);

/**
 * @brief Performs pixel-perfect overlap test within a known overlap rectangle.
 * @param lhs First hit mask.
 * @param rhs Second hit mask.
 * @param overlap Rectangle where masks overlap in world space.
 * @param lhsOffsetX World x-position of lhs mask.
 * @param lhsOffsetY World y-position of lhs mask.
 * @param rhsOffsetX World x-position of rhs mask.
 * @param rhsOffsetY World y-position of rhs mask.
 * @return True if any solid pixels overlap.
 */
bool PixelPerfect(const HitMask& lhs,
                  const HitMask& rhs,
                  const Rect& overlap,
                  int lhsOffsetX,
                  int lhsOffsetY,
                  int rhsOffsetX,
                  int rhsOffsetY);

}  // namespace Collision

}  // namespace game
