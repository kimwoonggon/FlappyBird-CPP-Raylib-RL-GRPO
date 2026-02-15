/**
 * @file src/ai/FrameStack.h
 * @brief Declarations for FrameStack.
 */

#pragma once

#include <deque>
#include <vector>

namespace ai {

/**
 * @brief Fixed-size frame history buffer used to build model input tensors.
 */
class FrameStack {
 public:
  /**
   * @brief Creates a frame stack with fixed capacity and frame size.
   * @param maxFrames Maximum number of frames retained.
   * @param frameSize Number of float values in one frame.
   */
  FrameStack(int maxFrames, int frameSize);

  /**
   * @brief Appends a new frame and evicts oldest frames if full.
   * @param frame Flat frame values to push.
   */
  void Push(const std::vector<float>& frame);

  /**
   * @brief Removes all stored frames.
   */
  void Clear();

  /**
   * @brief Checks whether stack currently has max frame count.
   * @return True when no more warmup frames are needed.
   */
  bool IsFull() const;

  /**
   * @brief Returns current number of stored frames.
   * @return Number of frames in the stack.
   */
  size_t Size() const;

  /**
   * @brief Exports frames in model tensor order with zero-padding for missing history.
   * @return Contiguous tensor values of size maxFrames * frameSize.
   */
  std::vector<float> ToTensor() const;

  /**
   * @brief Returns a copy of stored frames for debugging visualization.
   * @return Per-frame vectors in chronological order.
   */
  std::vector<std::vector<float>> Frames() const;

 private:
  int maxFrames_ = 0;
  int frameSize_ = 0;
  std::deque<std::vector<float>> frames_;
};

}  // namespace ai
