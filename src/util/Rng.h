/**
 * @file src/util/Rng.h
 * @brief Declarations for Rng.
 */

#pragma once

#include <cstdint>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

namespace util {

/**
 * @brief Utility wrapper around `std::mt19937`.
 */
class Rng {
 public:
  /**
   * @brief Constructs RNG with explicit or random seed.
   * @param seed Initial seed value.
   */
  explicit Rng(uint32_t seed = std::random_device{}()) : engine_(seed) {}

  /**
   * @brief Draws uniform integer in inclusive range.
   * @param minInclusive Lower bound.
   * @param maxInclusive Upper bound.
   * @return Sampled integer.
   */
  int NextInt(int minInclusive, int maxInclusive) {
    if (minInclusive > maxInclusive) {
      throw std::invalid_argument("minInclusive must be <= maxInclusive");
    }
    std::uniform_int_distribution<int> distribution(minInclusive, maxInclusive);
    return distribution(engine_);
  }

  /**
   * @brief Draws random boolean value.
   * @return True or false with equal probability.
   */
  bool NextBool() {
    return NextInt(0, 1) == 1;
  }

  /**
   * @brief Samples index from non-negative weight vector.
   * @param weights Per-index sampling weights.
   * @return Selected index.
   */
  int WeightedIndex(const std::vector<float>& weights) {
    if (weights.empty()) {
      throw std::invalid_argument("weights must not be empty");
    }
    std::vector<double> safeWeights(weights.begin(), weights.end());
    const double total = std::accumulate(safeWeights.begin(), safeWeights.end(), 0.0);
    if (total <= 0.0) {
      return 0;
    }
    std::discrete_distribution<int> distribution(safeWeights.begin(), safeWeights.end());
    return distribution(engine_);
  }

 private:
  std::mt19937 engine_;
};

}  // namespace util
