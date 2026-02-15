#include "ai/OnnxPolicy.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

#ifndef AI_ENABLE_COREML
#define AI_ENABLE_COREML 1
#endif

#if defined(__APPLE__) && AI_ENABLE_COREML
#include <coreml_provider_factory.h>
#endif

namespace ai {
namespace {
constexpr const char* kInputName = "input";
constexpr const char* kOutputName = "output";

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

#if defined(__APPLE__) && AI_ENABLE_COREML
    const bool wantsCoreMl = preferredBackend_ != app::InferenceBackend::kCpu;
    if (wantsCoreMl) {
      OrtStatus* coreMlStatus = OrtSessionOptionsAppendExecutionProvider_CoreML(sessionOptions_, COREML_FLAG_USE_NONE);
      if (coreMlStatus != nullptr) {
        const OrtApi& api = Ort::GetApi();
        const char* message = api.GetErrorMessage(coreMlStatus);
        std::cerr << "CoreML EP unavailable, fallback to CPU: "
                  << (message != nullptr ? message : "unknown error") << std::endl;
        api.ReleaseStatus(coreMlStatus);
      } else {
        activeBackend_ = app::InferenceBackend::kCoreMl;
      }
    }
#else
    if (preferredBackend_ == app::InferenceBackend::kCoreMl) {
      std::cerr << "CoreML backend requested but this binary was built without CoreML support; fallback to CPU."
                << std::endl;
    }
#endif

    session_ = std::make_unique<Ort::Session>(*env_, modelPath_.c_str(), sessionOptions_);
    hasModel_ = true;
    std::cout << "ONNX Runtime active backend: " << BackendToString(activeBackend_)
              << " (requested=" << BackendToString(preferredBackend_) << ")" << std::endl;
  } catch (const Ort::Exception& ex) {
    std::cerr << "Failed to load ONNX model: " << ex.what() << std::endl;
    hasModel_ = false;
    session_.reset();
  }
}

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

const char* OnnxPolicy::ActiveBackendName() const {
  return BackendToString(activeBackend_);
}

bool OnnxPolicy::IsUsingCoreMl() const {
  return activeBackend_ == app::InferenceBackend::kCoreMl;
}

}  // namespace ai
