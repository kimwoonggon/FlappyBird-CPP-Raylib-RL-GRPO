/**
 * @file src/main.cpp
 * @brief Implementation for main.
 */

#include "app/App.h"
#include "app/Config.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

namespace {

/**
 * @brief Reads environment variable and normalizes it to lowercase.
 * @param name Environment variable name.
 * @return Lowercased value or empty string if unset.
 */
std::string ReadEnvLower(const char* name) {
  const char* value = std::getenv(name);
  if (value == nullptr) {
    return "";
  }
  std::string out(value);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

/**
 * @brief Parses inference backend value from environment text.
 * @param value Lowercased backend token.
 * @return Parsed inference backend enum.
 */
app::InferenceBackend ParseInferenceBackend(const std::string& value) {
  if (value == "cpu") {
    return app::InferenceBackend::kCpu;
  }
  if (value == "coreml") {
    return app::InferenceBackend::kCoreMl;
  }
  return app::InferenceBackend::kAuto;
}

/**
 * @brief Parses preprocessing backend value from environment text.
 * @param value Lowercased backend token.
 * @return Parsed preprocess backend enum.
 */
app::PreprocessBackend ParsePreprocessBackend(const std::string& value) {
  if (value == "cpu") {
    return app::PreprocessBackend::kCpu;
  }
  if (value == "shader") {
    return app::PreprocessBackend::kGpuShader;
  }
  return app::PreprocessBackend::kAuto;
}

}  // namespace

/**
 * @brief Program entry point for primary runtime binary.
 * @return Process exit code.
 */
int main() {
  app::Config config = app::Config::Default();
  config.ai.inferenceBackend = ParseInferenceBackend(ReadEnvLower("FLAPPY_INFERENCE"));
  config.ai.preprocessBackend = ParsePreprocessBackend(ReadEnvLower("FLAPPY_PREPROCESS"));

  app::App app(config);
  return app.Run();
}
