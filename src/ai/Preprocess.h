#pragma once

#include <vector>

#include <raylib.h>

#include "app/Config.h"

namespace ai {

class Preprocess {
 public:
  explicit Preprocess(const app::Config& config);

  std::vector<float> Process(const Image& source) const;
  std::vector<float> ProcessPreprocessed(const Image& preprocessed) const;

 private:
  app::AiConfig ai_;
};

}  // namespace ai
