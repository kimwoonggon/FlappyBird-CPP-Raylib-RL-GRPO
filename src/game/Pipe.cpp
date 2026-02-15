/**
 * @file src/game/Pipe.cpp
 * @brief Implementation for Pipe.
 */

#include "game/Pipe.h"

namespace game {

/**
 * @brief Generates one upper/lower pipe pair at the next spawn position.
 * @param config Runtime configuration for pipe layout.
 * @param screenWidth Screen width used as spawn origin.
 * @param rng Random generator for gap jitter.
 * @param pipeWidth Pipe width (unused in this implementation).
 * @param pipeHeight Pipe height used for upper-pipe y placement.
 * @return Generated pipe coordinates.
 */
Pipe GeneratePipe(const app::Config& config,
                  int screenWidth,
                  util::Rng& rng,
                  int pipeWidth,
                  int pipeHeight) {
  (void)pipeWidth;

  const int x = screenWidth + config.pipe.spawnOffsetX;
  const int gapY = rng.NextInt(2, 10) * 10 + static_cast<int>(config.screen.baseY / 5.0F);
  const int lowerUp = rng.NextInt(2, 10) * 5 * (-1);

  int lowerStart = gapY + config.pipe.gapSize;
  int diff = 0;

  if (lowerStart > config.pipe.lowerStartClamp) {
    lowerStart = config.pipe.lowerStartClamp + lowerUp;
    diff = lowerStart - (gapY + config.pipe.gapSize);
  }

  return Pipe{x, gapY - pipeHeight + diff, x, lowerStart};
}

}  // namespace game
