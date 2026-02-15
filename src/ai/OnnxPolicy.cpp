/**
 * @file src/ai/OnnxPolicy.cpp
 * @brief Implementation for OnnxPolicy.
 */

#include "ai/OnnxPolicy.h"
#include "ai/OnnxPolicyProvider.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace ai {
namespace {
constexpr const char* kInputName = "input";
constexpr const char* kOutputName = "output";

/**
 * @brief Converts inference backend enum to display text.
 * @param backend Backend enum.
 * @return Backend label string.
 */
const char* BackendToString(app::InferenceBackend backend) {
  switch (backend) {
    case app::InferenceBackend::kCoreMl:
      return "CoreML";
    case app::InferenceBackend::kCpu:
      return "CPU";
    case app::InferenceBackend::kAuto:
      return "Auto";
  }
  return "Unknown";
}
}  // namespace

/**
 * @brief Constructs policy wrapper and tries to load ONNX model session.
 * @param modelPath Model path.
 * @param inputSize Model input width/height.
 * @param frameStack Number of stacked frames in input tensor.
 * @param preferredBackend Preferred inference backend.
 */
OnnxPolicy::OnnxPolicy(const std::string& modelPath,
                       int inputSize,
                       int frameStack,
                       app::InferenceBackend preferredBackend)
    : modelPath_(modelPath),
      inputSize_(inputSize),
      frameStack_(frameStack),
      preferredBackend_(preferredBackend),
      activeBackend_(app::InferenceBackend::kCpu) {
  inputShape_ = {1, frameStack_, inputSize_, inputSize_};
      inputTensorSize_ = static_cast<size_t>(frameStack_) * static_cast<size_t>(inputSize_) * static_cast<size_t>(inputSize_);
  TryLoad();
}

/**
 * @brief Creates ONNX Runtime environment/session and selects backend.
 */
void OnnxPolicy::TryLoad() {
  hasModel_ = false;
  activeBackend_ = app::InferenceBackend::kCpu;
  if (!std::filesystem::exists(modelPath_)) {
    std::cerr << "Failed to load ONNX model: " << modelPath_ << " does not exist" << std::endl;
    return;
  }

  try {
    env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "FlappyBird-RL");
    sessionOptions_.SetIntraOpNumThreads(1);
    sessionOptions_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    if (WantsCoreMl()) {
      TryEnableCoreMlProvider();
    }

    session_ = std::make_unique<Ort::Session>(*env_, modelPath_.c_str(), sessionOptions_);
    hasModel_ = true;
    LogBackendSelection();
  } catch (const Ort::Exception& ex) {
    std::cerr << "Failed to load ONNX model: " << ex.what() << std::endl;
    hasModel_ = false;
    session_.reset();
  }
}

/**
 * @brief Returns whether caller allows attempting CoreML provider.
 * @return True unless explicit CPU mode was requested.
 */
bool OnnxPolicy::WantsCoreMl() const {
  return preferredBackend_ != app::InferenceBackend::kCpu;
}

/**
 * @brief Attempts to append CoreML execution provider to session options.
 * @return True when CoreML provider was enabled.
 */
bool OnnxPolicy::TryEnableCoreMlProvider() {
  std::string errorMessage;
  const bool enabled = TryAppendCoreMlExecutionProvider(&sessionOptions_, &errorMessage);
  if (enabled) {
    activeBackend_ = app::InferenceBackend::kCoreMl;
    return true;
  }

  const bool strictCoreMlRequest = preferredBackend_ == app::InferenceBackend::kCoreMl;
  if (strictCoreMlRequest) {
    std::cerr << "CoreML backend requested but unavailable, fallback to CPU: "
              << errorMessage << std::endl;
  } else if (!errorMessage.empty()) {
    std::cerr << "CoreML auto-selection skipped, fallback to CPU: "
              << errorMessage << std::endl;
  }
  return false;
}

/**
 * @brief Logs requested and active backend selection.
 */
void OnnxPolicy::LogBackendSelection() const {
  std::cout << "ONNX Runtime active backend: " << BackendToString(activeBackend_)
            << " (requested=" << BackendToString(preferredBackend_) << ")" << std::endl;
}

/**
 * @brief Runs policy inference for one stacked NCHW tensor.
 * @param input Input tensor values.
 * @return Two action probabilities [no-op, flap].
 */
std::array<float, 2> OnnxPolicy::Infer(const std::vector<float>& input) const {
  if (!hasModel_ || !session_) {
    throw std::runtime_error("ONNX model is not loaded");
  }
  if (input.size() != inputTensorSize_) {
    throw std::invalid_argument("input tensor size mismatch");
  }

  try {
    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    std::vector<float> mutableInput(input.begin(), input.end());
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memoryInfo,
                                                              mutableInput.data(),
                                                              mutableInput.size(),
                                                              inputShape_.data(),
                                                              inputShape_.size());

    const char* inputNames[] = {kInputName};
    const char* outputNames[] = {kOutputName};

    std::vector<Ort::Value> outputs = session_->Run(Ort::RunOptions{nullptr},
                                                     inputNames,
                                                     &inputTensor,
                                                     1,
                                                     outputNames,
                                                     1);
    float* outputData = outputs.front().GetTensorMutableData<float>();
    return {outputData[0], outputData[1]};
  } catch (const Ort::Exception& ex) {
    throw std::runtime_error(std::string("ONNX inference failed for model ") + modelPath_ + ": " + ex.what());
  }
}

/**
 * @brief Returns currently active backend label.
 * @return Backend label.
 */
const char* OnnxPolicy::ActiveBackendName() const {
  return BackendToString(activeBackend_);
}

/**
 * @brief Returns whether CoreML provider is actively used.
 * @return True when active backend is CoreML.
 */
bool OnnxPolicy::IsUsingCoreMl() const {
  return activeBackend_ == app::InferenceBackend::kCoreMl;
}

}  // namespace ai
