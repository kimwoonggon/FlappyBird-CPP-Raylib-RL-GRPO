/**
 * @file src/ai/Preprocess.cpp
 * @brief Implementation for Preprocess.
 */

#include "ai/Preprocess.h"

#include <stdexcept>

namespace ai {

/**
 * @brief Stores preprocessing-related AI config snapshot.
 * @param config Runtime configuration.
 */
Preprocess::Preprocess(const app::Config& config) : ai_(config.ai) {}

/**
 * @brief Runs CPU preprocessing from raw scene frame.
 * @param source Source frame image.
 * @return Binary, vertically flipped model input frame.
 */
std::vector<float> Preprocess::Process(const Image& source) const {
  Image cutCopy = ImageCopy(source);

  // We remove the upper area (score/sky margin) to match the training input distribution.
  Rectangle cropRect = {0.0F,
                        static_cast<float>(ai_.cropY),
                        static_cast<float>(cutCopy.width),
                        static_cast<float>(ai_.cropHeight)};
  ImageCrop(&cutCopy, cropRect);

  ImageResize(&cutCopy, ai_.inputSize, ai_.inputSize);
  ImageColorGrayscale(&cutCopy);

  std::vector<float> values(static_cast<size_t>(ai_.inputSize * ai_.inputSize), 0.0F);
  Color* pixels = LoadImageColors(cutCopy);

  if (pixels != nullptr) {
    for (int y = 0; y < ai_.inputSize; ++y) {
      for (int x = 0; x < ai_.inputSize; ++x) {
        const int sourceIndex = y * ai_.inputSize + x;
        const float binary = pixels[sourceIndex].r > 1 ? 255.0F : 0.0F;

        // The model expects vertically flipped frames because training used texture-space coordinates.
        const int flippedY = ai_.inputSize - 1 - y;
        const int targetIndex = flippedY * ai_.inputSize + x;
        values[static_cast<size_t>(targetIndex)] = binary;
      }
    }
    UnloadImageColors(pixels);
  }

  UnloadImage(cutCopy);
  return values;
}

/**
 * @brief Finalizes lightweight postprocess for GPU-preprocessed frame.
 * @param preprocessed Preprocessed grayscale image with model dimensions.
 * @return Binary, vertically flipped model input frame.
 */
std::vector<float> Preprocess::ProcessPreprocessed(const Image& preprocessed) const {
  if (preprocessed.width != ai_.inputSize || preprocessed.height != ai_.inputSize) {
    throw std::invalid_argument("preprocessed frame size mismatch");
  }

  std::vector<float> values(static_cast<size_t>(ai_.inputSize * ai_.inputSize), 0.0F);
  Color* pixels = LoadImageColors(preprocessed);

  if (pixels != nullptr) {
    for (int y = 0; y < ai_.inputSize; ++y) {
      for (int x = 0; x < ai_.inputSize; ++x) {
        const int sourceIndex = y * ai_.inputSize + x;
        const float binary = pixels[sourceIndex].r > 1 ? 255.0F : 0.0F;

        // Keep the same vertical orientation as the original CPU preprocessing path.
        const int flippedY = ai_.inputSize - 1 - y;
        const int targetIndex = flippedY * ai_.inputSize + x;
        values[static_cast<size_t>(targetIndex)] = binary;
      }
    }
    UnloadImageColors(pixels);
  }

  return values;
}

}  // namespace ai
