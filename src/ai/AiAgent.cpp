/**
 * @file src/ai/AiAgent.cpp
 * @brief Implementation for AiAgent.
 */

#include "ai/AiAgent.h"

#include <iostream>
#include <vector>

namespace ai {

/**
 * @brief Constructs AI agent and initializes preprocessing + frame stack.
 * @param config Runtime configuration.
 * @param policy Policy inference object.
 * @param rng Random generator used for action sampling.
 */
AiAgent::AiAgent(const app::Config& config, OnnxPolicy* policy, util::Rng* rng)
    : config_(config),
      policy_(policy),
      rng_(rng),
      frameStack_(config.ai.frameStack, config.ai.inputSize * config.ai.inputSize),
      preprocess_(config) {}

/**
 * @brief Runs full preprocessing path from raw frame image.
 * @param frame Raw captured frame.
 * @return Action decision.
 */
AiDecision AiAgent::Act(const Image& frame) {
  const std::vector<float> processedFrame = preprocess_.Process(frame);
  return ActFromProcessedFrame(processedFrame);
}

/**
 * @brief Runs inference path for already preprocessed frame image.
 * @param preprocessedFrame Preprocessed image (model-sized grayscale).
 * @return Action decision.
 */
AiDecision AiAgent::ActPreprocessed(const Image& preprocessedFrame) {
  const std::vector<float> processedFrame = preprocess_.ProcessPreprocessed(preprocessedFrame);
  return ActFromProcessedFrame(processedFrame);
}

/**
 * @brief Updates frame stack and executes policy inference when ready.
 * @param processedFrame Flat preprocessed frame values.
 * @return Action decision or fallback when unavailable.
 */
AiDecision AiAgent::ActFromProcessedFrame(const std::vector<float>& processedFrame) {
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

/**
 * @brief Generates fallback action with neutral probabilities.
 * @return Random binary action and 0.5/0.5 probabilities.
 */
AiDecision AiAgent::FallbackDecision() {
  lastProbabilities_ = {0.5F, 0.5F};
  const int randomAction = rng_ != nullptr ? rng_->NextInt(0, 1) : 0;
  return AiDecision{randomAction, lastProbabilities_};
}

/**
 * @brief Resets frame history and last probability state.
 */
void AiAgent::Reset() {
  frameStack_.Clear();
  lastProbabilities_ = {0.5F, 0.5F};
}

}  // namespace ai
