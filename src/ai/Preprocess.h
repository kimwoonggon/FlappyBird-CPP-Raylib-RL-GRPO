/**
 * @file src/ai/Preprocess.h
 * @brief Declarations for Preprocess.
 */

#pragma once

#include <vector>

#include <raylib.h>

#include "app/Config.h"

namespace ai {

/**
 * @brief Converts captured frames into model-ready tensor slices.
 */
class Preprocess {
 public:
  /**
   * @brief Constructs preprocessing helper from app config.
   * @param config Runtime configuration containing AI preprocessing params.
   */
  explicit Preprocess(const app::Config& config);

  /**
   * @brief Applies full CPU preprocess (crop, resize, grayscale, threshold, flip).
   * @param source Raw captured frame.
   * @return Flat frame tensor values.
   */
  std::vector<float> Process(const Image& source) const;

  /**
   * @brief Applies lightweight postprocess for already preprocessed images.
   * @param preprocessed Input image already resized and grayscaled.
   * @return Flat frame tensor values.
   */
  std::vector<float> ProcessPreprocessed(const Image& preprocessed) const;

 private:
  app::AiConfig ai_;
};

}  // namespace ai
