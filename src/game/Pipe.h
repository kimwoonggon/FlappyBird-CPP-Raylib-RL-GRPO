/**
 * @file src/game/Pipe.h
 * @brief Declarations for Pipe.
 */

#pragma once

#include "app/Config.h"
#include "util/Rng.h"

namespace game {

/**
 * @brief Pair of upper/lower pipe coordinates for one obstacle.
 */
struct Pipe {
  int xUpper;
  int yUpper;
  int xLower;
  int yLower;
};

/**
 * @brief Generates the next pipe pair based on config and RNG.
 * @param config Runtime configuration.
 * @param screenWidth Window width used as spawn reference.
 * @param rng Random source for vertical gap placement.
 * @param pipeWidth Width of a pipe sprite.
 * @param pipeHeight Height of a pipe sprite.
 * @return Spawned pipe coordinates.
 */
Pipe GeneratePipe(const app::Config& config,
                  int screenWidth,
                  util::Rng& rng,
                  int pipeWidth,
                  int pipeHeight);

}  // namespace game
