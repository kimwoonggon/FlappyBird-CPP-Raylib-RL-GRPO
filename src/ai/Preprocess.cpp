#include "ai/Preprocess.h"

namespace ai {

Preprocess::Preprocess(const app::Config& config) : ai_(config.ai) {}

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

}  // namespace ai
