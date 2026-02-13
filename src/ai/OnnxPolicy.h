#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include <onnxruntime_cxx_api.h>

namespace ai {

class OnnxPolicy {
 public:
  OnnxPolicy(const std::string& modelPath, int inputSize, int frameStack);

  bool HasModel() const { return hasModel_; }
  std::array<float, 2> Infer(const std::vector<float>& input) const;

 private:
  void TryLoad();

  std::string modelPath_;
  int inputSize_ = 0;
  int frameStack_ = 0;
  bool hasModel_ = false;

  std::unique_ptr<Ort::Env> env_;
  Ort::SessionOptions sessionOptions_;
  std::unique_ptr<Ort::Session> session_;

  std::array<int64_t, 4> inputShape_{};
  size_t inputTensorSize_ = 0;
};

}  // namespace ai
