#include "ai/FrameStack.h"

#include <stdexcept>

namespace ai {

FrameStack::FrameStack(int maxFrames, int frameSize)
    : maxFrames_(maxFrames), frameSize_(frameSize) {
  if (maxFrames_ <= 0 || frameSize_ <= 0) {
    throw std::invalid_argument("maxFrames and frameSize must be positive");
  }
}

void FrameStack::Push(const std::vector<float>& frame) {
  if (static_cast<int>(frame.size()) != frameSize_) {
    throw std::invalid_argument("frame size mismatch");
  }

  frames_.push_back(frame);
  while (static_cast<int>(frames_.size()) > maxFrames_) {
    frames_.pop_front();
  }
}

void FrameStack::Clear() {
  frames_.clear();
}

bool FrameStack::IsFull() const {
  return static_cast<int>(frames_.size()) == maxFrames_;
}

size_t FrameStack::Size() const {
  return frames_.size();
}

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

std::vector<std::vector<float>> FrameStack::Frames() const {
  return std::vector<std::vector<float>>(frames_.begin(), frames_.end());
}

}  // namespace ai
