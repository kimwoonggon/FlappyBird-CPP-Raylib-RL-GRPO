#pragma once

#include <cstdint>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

namespace util {

class Rng {
 public:
  explicit Rng(uint32_t seed = std::random_device{}()) : engine_(seed) {}

  int NextInt(int minInclusive, int maxInclusive) {
    if (minInclusive > maxInclusive) {
      throw std::invalid_argument("minInclusive must be <= maxInclusive");
    }
    std::uniform_int_distribution<int> distribution(minInclusive, maxInclusive);
    return distribution(engine_);
  }

  bool NextBool() {
    return NextInt(0, 1) == 1;
  }

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
