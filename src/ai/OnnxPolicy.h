#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include <onnxruntime_cxx_api.h>

#include "app/Config.h"

namespace ai {

class OnnxPolicy {
 public:
  OnnxPolicy(const std::string& modelPath,
             int inputSize,
             int frameStack,
             app::InferenceBackend preferredBackend = app::InferenceBackend::kAuto);

  bool HasModel() const { return hasModel_; }
  std::array<float, 2> Infer(const std::vector<float>& input) const;
  const char* ActiveBackendName() const;
  bool IsUsingCoreMl() const;

 private:
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
