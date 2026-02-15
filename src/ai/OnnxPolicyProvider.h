/**
 * @file src/ai/OnnxPolicyProvider.h
 * @brief Declarations for OnnxPolicyProvider.
 */

#pragma once

#include <string>

#include <onnxruntime_cxx_api.h>

namespace ai {

/**
 * @brief Appends CoreML execution provider to ONNX Runtime session options.
 * @param sessionOptions Session options to modify.
 * @param errorMessage Optional output string for failure reason.
 * @return True if CoreML provider was successfully enabled.
 */
bool TryAppendCoreMlExecutionProvider(Ort::SessionOptions* sessionOptions,
                                      std::string* errorMessage);

}  // namespace ai
