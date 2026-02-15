#pragma once

#include <array>
#include <vector>

#include <raylib.h>

#include "ai/FrameStack.h"
#include "ai/OnnxPolicy.h"
#include "ai/Preprocess.h"
#include "app/Config.h"
#include "util/Rng.h"

namespace ai {

struct AiDecision {
  int action = 0;
  std::array<float, 2> probabilities{0.5F, 0.5F};
};

class AiAgent {
 public:
  AiAgent(const app::Config& config, OnnxPolicy* policy, util::Rng* rng);

  AiDecision Act(const Image& frame);
  AiDecision ActPreprocessed(const Image& preprocessedFrame);
  AiDecision FallbackDecision();

  void Reset();
  std::array<float, 2> LastProbabilities() const { return lastProbabilities_; }
  std::vector<std::vector<float>> DebugFrames() const { return frameStack_.Frames(); }

 private:
  AiDecision ActFromProcessedFrame(const std::vector<float>& processedFrame);

  app::Config config_;
  OnnxPolicy* policy_ = nullptr;
  util::Rng* rng_ = nullptr;

  FrameStack frameStack_;
  Preprocess preprocess_;

  std::array<float, 2> lastProbabilities_{0.5F, 0.5F};
};

}  // namespace ai
