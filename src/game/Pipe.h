#pragma once

#include "app/Config.h"
#include "util/Rng.h"

namespace game {

struct Pipe {
  int xUpper;
  int yUpper;
  int xLower;
  int yLower;
};

Pipe GeneratePipe(const app::Config& config,
                  int screenWidth,
                  util::Rng& rng,
                  int pipeWidth,
                  int pipeHeight);

}  // namespace game
