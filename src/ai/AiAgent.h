/**
 * @file src/ai/AiAgent.h
 * @brief Declarations for AiAgent.
 */

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

/**
 * @brief Action and probability output produced by the agent each frame.
 */
struct AiDecision {
  int action = 0;
  std::array<float, 2> probabilities{0.5F, 0.5F};
};

/**
 * @brief Coordinates preprocessing, frame stacking, and policy inference.
 */
class AiAgent {
 public:
  /**
   * @brief Constructs an AI agent bound to runtime config and optional policy.
   * @param config Global app configuration.
   * @param policy Inference policy, can be null to force fallback actions.
   * @param rng Random generator for stochastic sampling.
   */
  AiAgent(const app::Config& config, OnnxPolicy* policy, util::Rng* rng);

  /**
   * @brief Runs full CPU preprocess + inference path from raw image.
   * @param frame Captured frame image.
   * @return Sampled action and current action probabilities.
   */
  AiDecision Act(const Image& frame);

  /**
   * @brief Runs inference path when frame is already preprocessed.
   * @param preprocessedFrame Input image already resized/grayscaled.
   * @return Sampled action and current action probabilities.
   */
  AiDecision ActPreprocessed(const Image& preprocessedFrame);

  /**
   * @brief Produces a random fallback action with neutral probabilities.
   * @return Fallback action decision.
   */
  AiDecision FallbackDecision();

  /**
   * @brief Clears frame history and resets probabilities.
   */
  void Reset();

  /**
   * @brief Returns probabilities from latest inference or fallback.
   * @return Two-element probability vector [no-op, flap].
   */
  std::array<float, 2> LastProbabilities() const { return lastProbabilities_; }

  /**
   * @brief Returns internal stacked frames for on-screen debug.
   * @return Copy of currently buffered frames.
   */
  std::vector<std::vector<float>> DebugFrames() const { return frameStack_.Frames(); }

 private:
  /**
   * @brief Pushes processed frame and resolves action via policy or fallback.
   * @param processedFrame Flat preprocessed frame values.
   * @return Action decision.
   */
  AiDecision ActFromProcessedFrame(const std::vector<float>& processedFrame);

  app::Config config_;
  OnnxPolicy* policy_ = nullptr;
  util::Rng* rng_ = nullptr;

  FrameStack frameStack_;
  Preprocess preprocess_;

  std::array<float, 2> lastProbabilities_{0.5F, 0.5F};
};

}  // namespace ai
