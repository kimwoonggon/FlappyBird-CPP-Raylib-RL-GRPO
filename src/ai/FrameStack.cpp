/**
 * @file src/ai/FrameStack.cpp
 * @brief Implementation for FrameStack.
 */

#include "ai/FrameStack.h"

#include <stdexcept>

namespace ai {

/**
 * @brief Creates bounded frame stack and validates dimensions.
 * @param maxFrames Maximum number of frames to retain.
 * @param frameSize Number of float elements per frame.
 */
FrameStack::FrameStack(int maxFrames, int frameSize)
    : maxFrames_(maxFrames), frameSize_(frameSize) {
  if (maxFrames_ <= 0 || frameSize_ <= 0) {
    throw std::invalid_argument("maxFrames and frameSize must be positive");
  }
}

/**
 * @brief Pushes a frame into stack and evicts oldest frames when full.
 * @param frame Flat frame data.
 */
void FrameStack::Push(const std::vector<float>& frame) {
  if (static_cast<int>(frame.size()) != frameSize_) {
    throw std::invalid_argument("frame size mismatch");
  }

  frames_.push_back(frame);
  while (static_cast<int>(frames_.size()) > maxFrames_) {
    frames_.pop_front();
  }
}

/**
 * @brief Clears all buffered frames.
 */
void FrameStack::Clear() {
  frames_.clear();
}

/**
 * @brief Returns whether the frame stack reached capacity.
 * @return True when stack size equals configured max frame count.
 */
bool FrameStack::IsFull() const {
  return static_cast<int>(frames_.size()) == maxFrames_;
}

/**
 * @brief Returns current number of buffered frames.
 * @return Frame count.
 */
size_t FrameStack::Size() const {
  return frames_.size();
}

/**
 * @brief Serializes stack as padded contiguous tensor values.
 * @return Tensor data in chronological frame order.
 */
std::vector<float> FrameStack::ToTensor() const {
  std::vector<float> tensor;
  tensor.reserve(static_cast<size_t>(maxFrames_ * frameSize_));

  const size_t missing = static_cast<size_t>(maxFrames_) - frames_.size();
  for (size_t i = 0; i < missing; ++i) {
    tensor.insert(tensor.end(), static_cast<size_t>(frameSize_), 0.0F);
  }

  for (const std::vector<float>& frame : frames_) {
    tensor.insert(tensor.end(), frame.begin(), frame.end());
  }

  return tensor;
}

/**
 * @brief Returns buffered frames as copy for debugging.
 * @return List of per-frame vectors.
 */
std::vector<std::vector<float>> FrameStack::Frames() const {
  return std::vector<std::vector<float>>(frames_.begin(), frames_.end());
}

}  // namespace ai
