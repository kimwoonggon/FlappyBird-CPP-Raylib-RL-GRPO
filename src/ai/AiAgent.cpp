#include "ai/AiAgent.h"

#include <iostream>
#include <vector>

namespace ai {

AiAgent::AiAgent(const app::Config& config, OnnxPolicy* policy, util::Rng* rng)
    : config_(config),
      policy_(policy),
      rng_(rng),
      frameStack_(config.ai.frameStack, config.ai.inputSize * config.ai.inputSize),
      preprocess_(config) {}

AiDecision AiAgent::Act(const Image& frame) {
  std::vector<float> processedFrame = preprocess_.Process(frame);
  frameStack_.Push(processedFrame);

  if (!frameStack_.IsFull() || policy_ == nullptr || !policy_->HasModel()) {
    return FallbackDecision();
  }

  try {
    const std::vector<float> input = frameStack_.ToTensor();
    lastProbabilities_ = policy_->Infer(input);

    const std::vector<float> weights = {lastProbabilities_[0], lastProbabilities_[1]};
    const int sampledAction = rng_ != nullptr ? rng_->WeightedIndex(weights) : 0;

    return AiDecision{sampledAction, lastProbabilities_};
  } catch (const std::exception& ex) {
    std::cerr << "AI inference failed, fallback to random action: " << ex.what() << std::endl;
    return FallbackDecision();
  }
}

AiDecision AiAgent::FallbackDecision() {
  lastProbabilities_ = {0.5F, 0.5F};
  const int randomAction = rng_ != nullptr ? rng_->NextInt(0, 1) : 0;
  return AiDecision{randomAction, lastProbabilities_};
}

void AiAgent::Reset() {
  frameStack_.Clear();
  lastProbabilities_ = {0.5F, 0.5F};
}

}  // namespace ai
