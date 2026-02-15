#pragma once

#include <deque>
#include <vector>

namespace ai {

class FrameStack {
 public:
  FrameStack(int maxFrames, int frameSize);

  void Push(const std::vector<float>& frame);
  void Clear();

  bool IsFull() const;
  size_t Size() const;

  std::vector<float> ToTensor() const;
  std::vector<std::vector<float>> Frames() const;

 private:
  int maxFrames_ = 0;
  int frameSize_ = 0;
  std::deque<std::vector<float>> frames_;
};

}  // namespace ai
