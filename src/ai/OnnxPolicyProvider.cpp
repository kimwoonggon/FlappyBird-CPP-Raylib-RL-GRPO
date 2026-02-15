/**
 * @file src/ai/OnnxPolicyProvider.cpp
 * @brief Implementation for OnnxPolicyProvider.
 */

#include "ai/OnnxPolicyProvider.h"

#ifndef AI_ENABLE_COREML
#define AI_ENABLE_COREML 1
#endif

#if defined(__APPLE__) && AI_ENABLE_COREML
#include <coreml_provider_factory.h>
#endif

namespace ai {

/**
 * @brief Appends CoreML execution provider when available on this build.
 * @param sessionOptions Session options object to modify.
 * @param errorMessage Optional output for failure reason.
 * @return True when CoreML provider was appended successfully.
 */
bool TryAppendCoreMlExecutionProvider(Ort::SessionOptions* sessionOptions,
                                      std::string* errorMessage) {
  if (sessionOptions == nullptr) {
    if (errorMessage != nullptr) {
      *errorMessage = "session options pointer is null";
    }
    return false;
  }

#if defined(__APPLE__) && AI_ENABLE_COREML
  OrtStatus* coreMlStatus =
      OrtSessionOptionsAppendExecutionProvider_CoreML(*sessionOptions, COREML_FLAG_USE_NONE);
  if (coreMlStatus == nullptr) {
    return true;
  }

  const OrtApi& api = Ort::GetApi();
  const char* message = api.GetErrorMessage(coreMlStatus);
  if (errorMessage != nullptr) {
    *errorMessage = message != nullptr ? message : "unknown CoreML append error";
  }
  api.ReleaseStatus(coreMlStatus);
  return false;
#else
  if (errorMessage != nullptr) {
    *errorMessage = "CoreML support is disabled at build time";
  }
  return false;
#endif
}

}  // namespace ai
