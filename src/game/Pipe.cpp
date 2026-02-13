#include "game/Pipe.h"

namespace game {

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
