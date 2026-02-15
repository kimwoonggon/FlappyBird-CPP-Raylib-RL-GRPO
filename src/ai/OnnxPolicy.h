/**
 * @file src/ai/OnnxPolicy.h
 * @brief Declarations for OnnxPolicy.
 */

#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include <onnxruntime_cxx_api.h>

#include "app/Config.h"

namespace ai {

/**
 * @brief Thin wrapper around ONNX Runtime session for FlappyBird policy inference.
 */
class OnnxPolicy {
 public:
  /**
   * @brief Creates and loads policy session from ONNX model path.
   * @param modelPath Filesystem path to ONNX model.
   * @param inputSize Spatial size of one preprocessed frame.
   * @param frameStack Number of frames stacked in one input tensor.
   * @param preferredBackend Preferred runtime backend selection.
   */
  OnnxPolicy(const std::string& modelPath,
             int inputSize,
             int frameStack,
             app::InferenceBackend preferredBackend = app::InferenceBackend::kAuto);

  /**
   * @brief Checks if model session was loaded successfully.
   * @return True when inference can be executed.
   */
  bool HasModel() const { return hasModel_; }

  /**
   * @brief Runs policy inference for one stacked tensor.
   * @param input Flat NCHW tensor values.
   * @return Two action probabilities [no-op, flap].
   */
  std::array<float, 2> Infer(const std::vector<float>& input) const;

  /**
   * @brief Returns active backend name used by current session.
   * @return Backend label string.
   */
  const char* ActiveBackendName() const;

  /**
   * @brief Indicates whether CoreML provider is active.
   * @return True when CoreML execution provider is in use.
   */
  bool IsUsingCoreMl() const;

 private:
  /** @brief Returns whether CoreML should be attempted for this policy. */
  bool WantsCoreMl() const;
  /** @brief Tries to append CoreML execution provider to session options. */
  bool TryEnableCoreMlProvider();
  /** @brief Logs active backend after model/session initialization. */
  void LogBackendSelection() const;
  /** @brief Loads model and builds ONNX Runtime session. */
  void TryLoad();

  std::string modelPath_;
  int inputSize_ = 0;
  int frameStack_ = 0;
  app::InferenceBackend preferredBackend_ = app::InferenceBackend::kAuto;
  app::InferenceBackend activeBackend_ = app::InferenceBackend::kCpu;
  bool hasModel_ = false;

  std::unique_ptr<Ort::Env> env_;
  Ort::SessionOptions sessionOptions_;
  std::unique_ptr<Ort::Session> session_;

  std::array<int64_t, 4> inputShape_{};
  size_t inputTensorSize_ = 0;
};

}  // namespace ai
