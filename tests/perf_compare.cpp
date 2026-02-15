/**
 * @file tests/perf_compare.cpp
 * @brief Implementation for perf_compare.
 */

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <raylib.h>

#include "ai/AiAgent.h"
#include "ai/OnnxPolicy.h"
#include "app/Config.h"
#include "gfx/RaylibContext.h"
#include "gfx/Resources.h"
#include "util/Rng.h"

namespace {

constexpr const char* kPreprocessFragmentShader = R"glsl(
#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;
uniform float cropStart;
uniform float cropRange;

void main() {
  vec2 uv = vec2(fragTexCoord.x, cropStart + (fragTexCoord.y * cropRange));
  vec4 src = texture(texture0, uv);
  float gray = dot(src.rgb, vec3(0.299, 0.587, 0.114));
  finalColor = vec4(gray, gray, gray, 1.0);
}
)glsl";

/**
 * @brief One benchmark row for report output.
 */
struct PerfResult {
  std::string name;
  std::string requestedBackend;
  std::string activeBackend;
  std::string preprocess;
  double avgMs = 0.0;
  double fps = 0.0;
};

/**
 * @brief Reads positive integer env var with fallback.
 * @param name Environment variable name.
 * @param fallback Value used when unset/invalid.
 * @return Parsed positive integer.
 */
int ReadEnvInt(const char* name, int fallback) {
  const char* value = std::getenv(name);
  if (value == nullptr) {
    return fallback;
  }
  try {
    const int parsed = std::stoi(value);
    return parsed > 0 ? parsed : fallback;
  } catch (...) {
    return fallback;
  }
}

/**
 * @brief Reads string env var with fallback.
 * @param name Environment variable name.
 * @param fallback Value used when unset/empty.
 * @return Parsed string.
 */
std::string ReadEnvString(const char* name, const std::string& fallback) {
  const char* value = std::getenv(name);
  if (value == nullptr || value[0] == '\0') {
    return fallback;
  }
  return std::string(value);
}

/**
 * @brief Draws a synthetic scene into source render target for benchmarking.
 * @param config Runtime config for screen/base geometry.
 * @param target Off-screen render target.
 * @param frameIndex Frame counter used to animate primitives.
 */
void RenderSyntheticScene(const app::Config& config,
                          const gfx::RenderTextureResource& target,
                          int frameIndex) {
  gfx::TextureModeScope scope(target.Get());
  ClearBackground(BLACK);

  const int birdX = 50 + (frameIndex % 40);
  const int birdY = 180 + ((frameIndex / 3) % 90);
  const int pipeX = config.screen.width - ((frameIndex * 4) % (config.screen.width + 80));

  DrawRectangle(pipeX, 0, 52, 170, GREEN);
  DrawRectangle(pipeX, 280, 52, config.screen.height - 280, GREEN);
  DrawCircle(birdX, birdY, 12.0F, RED);
  DrawRectangle(0, static_cast<int>(config.screen.baseY), config.screen.width, 30, DARKGRAY);
}

/**
 * @brief Runs shader preprocess pass from source target into model-sized target.
 * @param config Runtime config containing input dimensions.
 * @param srcTarget Source render target.
 * @param preprocessTarget Destination preprocess target.
 * @param preprocessShader Shader used for crop/resize/grayscale pass.
 */
void RenderPreprocessedFrame(const app::Config& config,
                             const gfx::RenderTextureResource& srcTarget,
                             const gfx::RenderTextureResource& preprocessTarget,
                             const gfx::ShaderResource& preprocessShader) {
  gfx::TextureModeScope scope(preprocessTarget.Get());
  ClearBackground(BLACK);

  BeginShaderMode(preprocessShader.Get());
  Rectangle source = {0.0F,
                      0.0F,
                      static_cast<float>(srcTarget.Get().texture.width),
                      -static_cast<float>(srcTarget.Get().texture.height)};
  Rectangle target = {0.0F,
                      0.0F,
                      static_cast<float>(config.ai.inputSize),
                      static_cast<float>(config.ai.inputSize)};
  DrawTexturePro(srcTarget.Get().texture, source, target, Vector2{0.0F, 0.0F}, 0.0F, WHITE);
  EndShaderMode();
}

/**
 * @brief Loads preprocess shader and initializes crop uniform values.
 * @param config Runtime config containing crop parameters.
 * @param shader Output shader resource to initialize.
 */
void SetupPreprocessShader(const app::Config& config,
                           gfx::ShaderResource* shader) {
  if (shader == nullptr) {
    throw std::invalid_argument("shader setup args must not be null");
  }

  shader->LoadFromMemory(nullptr, kPreprocessFragmentShader);
  if (!shader->IsValid()) {
    throw std::runtime_error("failed to load preprocess shader");
  }

  const int cropStartLoc = GetShaderLocation(shader->Get(), "cropStart");
  const int cropRangeLoc = GetShaderLocation(shader->Get(), "cropRange");
  if (cropStartLoc < 0 || cropRangeLoc < 0) {
    throw std::runtime_error("failed to get preprocess shader uniform locations");
  }

  const float sourceHeight = static_cast<float>(config.screen.height);
  const float cropStart =
      std::clamp(static_cast<float>(config.ai.cropY) / sourceHeight, 0.0F, 1.0F);
  const float cropEnd = std::clamp(
      static_cast<float>(config.ai.cropY + config.ai.cropHeight) / sourceHeight, 0.0F, 1.0F);
  const float cropRange = std::max(cropEnd - cropStart, 1.0F / sourceHeight);

  SetShaderValue(shader->Get(), cropStartLoc, &cropStart, SHADER_UNIFORM_FLOAT);
  SetShaderValue(shader->Get(), cropRangeLoc, &cropRange, SHADER_UNIFORM_FLOAT);
}

/**
 * @brief Executes one benchmark configuration and returns timing metrics.
 * @param config Runtime configuration.
 * @param caseName Display name of benchmark case.
 * @param backend Requested inference backend.
 * @param useShaderPreprocess Whether shader preprocess path is used.
 * @param warmupFrames Warmup frame count.
 * @param measureFrames Measured frame count.
 * @return Benchmark result row.
 */
PerfResult RunCase(const app::Config& config,
                   const std::string& caseName,
                   app::InferenceBackend backend,
                   bool useShaderPreprocess,
                   int warmupFrames,
                   int measureFrames) {
  std::cerr << "[bench] start case=" << caseName
            << " warmup=" << warmupFrames
            << " measure=" << measureFrames << std::endl;

  util::Rng rng(20260215U);
  ai::OnnxPolicy policy(
      config.ai.modelPath, config.ai.inputSize, config.ai.frameStack, backend);
  if (!policy.HasModel()) {
    throw std::runtime_error("model load failed: " + config.ai.modelPath);
  }

  ai::AiAgent agent(config, &policy, &rng);
  gfx::RenderTextureResource frameTarget(config.screen.width, config.screen.height);
  if (!frameTarget.IsValid()) {
    throw std::runtime_error("failed to allocate frameTarget");
  }

  gfx::RenderTextureResource preprocessTarget;
  gfx::ShaderResource preprocessShader;

  if (useShaderPreprocess) {
    preprocessTarget.Load(config.ai.inputSize, config.ai.inputSize);
    if (!preprocessTarget.IsValid()) {
      throw std::runtime_error("failed to allocate preprocessTarget");
    }
    SetupPreprocessShader(config, &preprocessShader);
  }

  const int totalFrames = warmupFrames + measureFrames;
  const auto begin = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < totalFrames; ++i) {
    BeginDrawing();
    EndDrawing();
    RenderSyntheticScene(config, frameTarget, i);

    if (useShaderPreprocess) {
      RenderPreprocessedFrame(config, frameTarget, preprocessTarget, preprocessShader);
      Image preprocessed = LoadImageFromTexture(preprocessTarget.Get().texture);
      agent.ActPreprocessed(preprocessed);
      UnloadImage(preprocessed);
    } else {
      Image screenshot = LoadImageFromTexture(frameTarget.Get().texture);
      agent.Act(screenshot);
      UnloadImage(screenshot);
    }
  }

  const auto end = std::chrono::high_resolution_clock::now();
  const double totalMs =
      std::chrono::duration<double, std::milli>(end - begin).count();
  const double measureMs = totalMs *
                           (static_cast<double>(measureFrames) /
                            static_cast<double>(totalFrames));
  const double avgMs = measureMs / static_cast<double>(measureFrames);

  PerfResult result;
  result.name = caseName;
  result.requestedBackend =
      backend == app::InferenceBackend::kCoreMl ? "coreml" :
      backend == app::InferenceBackend::kCpu ? "cpu" : "auto";
  result.activeBackend = policy.ActiveBackendName();
  result.preprocess = useShaderPreprocess ? "shader" : "cpu";
  result.avgMs = avgMs;
  result.fps = avgMs > 0.0 ? 1000.0 / avgMs : 0.0;
  std::cerr << "[bench] done case=" << caseName
            << " avg_ms=" << result.avgMs
            << " fps=" << result.fps
            << " active_backend=" << result.activeBackend << std::endl;
  return result;
}

/**
 * @brief Prints benchmark results to stdout.
 * @param results Benchmark rows.
 */
void PrintResults(const std::vector<PerfResult>& results) {
  std::cout << "\n=== AI Pipeline Perf Compare ===\n";
  std::cout << "case | requested_backend | active_backend | preprocess | avg_ms | fps\n";

  for (const PerfResult& result : results) {
    std::cout << result.name << " | "
              << result.requestedBackend << " | "
              << result.activeBackend << " | "
              << result.preprocess << " | "
              << result.avgMs << " | "
              << result.fps << "\n";
  }

  if (results.size() >= 2) {
    const double base = results[0].avgMs;
    for (size_t i = 1; i < results.size(); ++i) {
      const double speedup = results[i].avgMs > 0.0 ? base / results[i].avgMs : 0.0;
      std::cout << "speedup(" << results[i].name << " vs " << results[0].name << ") = "
                << speedup << "x\n";
    }
  }
}

/**
 * @brief Writes benchmark results to markdown report file.
 * @param outputPath Output markdown path.
 * @param results Benchmark rows.
 * @param warmupFrames Warmup frame count used in run.
 * @param measureFrames Measure frame count used in run.
 */
void WriteMarkdownResults(const std::string& outputPath,
                          const std::vector<PerfResult>& results,
                          int warmupFrames,
                          int measureFrames) {
  std::ofstream out(outputPath);
  if (!out.is_open()) {
    throw std::runtime_error("failed to open benchmark output file: " + outputPath);
  }

  const auto now = std::chrono::system_clock::now();
  const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

  out << "# Bench Results\n\n";
  out << "- Generated: " << std::put_time(std::localtime(&nowTime), "%Y-%m-%d %H:%M:%S") << "\n";
  out << "- Warmup frames: " << warmupFrames << "\n";
  out << "- Measure frames: " << measureFrames << "\n\n";

  out << "| case | requested_backend | active_backend | preprocess | avg_ms | fps |\n";
  out << "| --- | --- | --- | --- | ---: | ---: |\n";

  out << std::fixed << std::setprecision(4);
  for (const PerfResult& result : results) {
    out << "| " << result.name
        << " | " << result.requestedBackend
        << " | " << result.activeBackend
        << " | " << result.preprocess
        << " | " << result.avgMs
        << " | " << result.fps
        << " |\n";
  }

  if (results.size() >= 2) {
    out << "\n## Speedup\n\n";
    const double base = results[0].avgMs;
    for (size_t i = 1; i < results.size(); ++i) {
      const double speedup = results[i].avgMs > 0.0 ? base / results[i].avgMs : 0.0;
      out << "- " << results[i].name << " vs " << results[0].name << ": "
          << std::setprecision(3) << speedup << "x\n";
    }
  }
}

}  // namespace

/**
 * @brief Entry point for benchmark comparison executable.
 * @return EXIT_SUCCESS on successful benchmark execution.
 */
int main() {
  try {
    app::Config config = app::Config::Default();

    std::cerr << "[bench] init window..." << std::endl;
    InitWindow(config.screen.width, config.screen.height, "perf_compare");
    SetTargetFPS(0);
    std::cerr << "[bench] window ready" << std::endl;

    const int warmupFrames = ReadEnvInt("FLAPPY_BENCH_WARMUP", 30);
    const int measureFrames = ReadEnvInt("FLAPPY_BENCH_FRAMES", 120);
    const std::string outputPath = ReadEnvString("FLAPPY_BENCH_OUT", "bench_results.md");
    std::cerr << "[bench] params warmup=" << warmupFrames
              << " frames=" << measureFrames
              << " out=" << outputPath << std::endl;

    std::vector<PerfResult> results;
    results.push_back(RunCase(config,
                              "cpu+cpu",
                              app::InferenceBackend::kCpu,
                              false,
                              warmupFrames,
                              measureFrames));

    results.push_back(RunCase(config,
                              "cpu+shader",
                              app::InferenceBackend::kCpu,
                              true,
                              warmupFrames,
                              measureFrames));

    results.push_back(RunCase(config,
                              "coreml+shader",
                              app::InferenceBackend::kCoreMl,
                              true,
                              warmupFrames,
                              measureFrames));

    PrintResults(results);
    WriteMarkdownResults(outputPath, results, warmupFrames, measureFrames);
    std::cout << "Benchmark markdown written to: " << outputPath << "\n";
    CloseWindow();
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "perf_compare failed: " << ex.what() << std::endl;
    if (IsWindowReady()) {
      CloseWindow();
    }
    return 1;
  }
}
