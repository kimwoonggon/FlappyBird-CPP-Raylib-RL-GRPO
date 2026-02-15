#include <array>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "app/Config.h"
#include "util/Rng.h"
#include "game/Pipe.h"
#include "game/Collision.h"
#include "game/Game.h"
#include "ai/FrameStack.h"
#include "ai/AiAgent.h"
#include "ai/OnnxPolicy.h"

namespace {

struct TestResult {
  std::string name;
  bool passed;
  std::string message;
};

#define ASSERT_TRUE(cond, msg) \
  do {                         \
    if (!(cond)) {             \
      throw std::runtime_error(msg); \
    }                          \
  } while (false)

TestResult Run(const std::string& name, const std::function<void()>& fn) {
  try {
    fn();
    return {name, true, ""};
  } catch (const std::exception& ex) {
    return {name, false, ex.what()};
  }
}

void TestConfigDefaults() {
  const app::Config config = app::Config::Default();
  ASSERT_TRUE(config.screen.width == 288, "screen.width should be 288");
  ASSERT_TRUE(config.screen.height == 512, "screen.height should be 512");
  ASSERT_TRUE(config.screen.fps == 30, "fps should be 30");
  ASSERT_TRUE(static_cast<int>(config.screen.baseY) == 404, "baseY should be 404 (512 * 0.79)");
  ASSERT_TRUE(config.ai.inputSize == 84, "ai inputSize should be 84");
  ASSERT_TRUE(config.ai.frameStack == 4, "ai frameStack should be 4");
}

void TestRngRange() {
  util::Rng rng(7U);
  for (int i = 0; i < 128; ++i) {
    const int value = rng.NextInt(2, 10);
    ASSERT_TRUE(value >= 2 && value <= 10, "NextInt must stay in inclusive range [2, 10]");
  }
}

void TestPipeGeneration() {
  util::Rng rng(9U);
  const app::Config config = app::Config::Default();
  const game::Pipe pipe = game::GeneratePipe(config, 320, rng, 52, 320);

  ASSERT_TRUE(pipe.xUpper == 330, "pipe xUpper should spawn at screenWidth + 10");
  ASSERT_TRUE(pipe.xLower == 330, "pipe xLower should spawn at screenWidth + 10");
  ASSERT_TRUE(pipe.yLower <= 260, "lower pipe start should be clamped to <= 260");
}

void TestHitMaskAndCollision() {
  game::HitMask bird(2, 2, {0, 1, 0, 1});
  game::HitMask pipe(2, 2, {0, 0, 1, 1});

  ASSERT_TRUE(!bird.At(0, 0), "bird(0,0) should be empty");
  ASSERT_TRUE(bird.At(1, 1), "bird(1,1) should be solid");

  const game::Rect overlap{0, 0, 2, 2};
  const bool collided = game::Collision::PixelPerfect(bird, pipe, overlap, 0, 0, 0, 0);
  ASSERT_TRUE(collided, "pixel collision should detect overlapping solid pixels");
}

void TestGameStateTransition() {
  app::Config config = app::Config::Default();
  config.physics.gravity = 1.0F;
  config.physics.jumpVelocity = -9.0F;
  config.physics.maxVelocityY = 10.0F;

  util::Rng rng(11U);
  game::Game game(config, rng, 34, 24, 52, 320);

  ASSERT_TRUE(game.State() == game::GameState::kReady, "initial state should be READY");

  game.Start();
  ASSERT_TRUE(game.State() == game::GameState::kPlaying, "state should be PLAYING after Start()");

  game.Update(false);
  game.Update(true);

  ASSERT_TRUE(game.Bird().velocity <= config.physics.maxVelocityY, "bird velocity should be clamped");
  ASSERT_TRUE(!game.Pipes().empty(), "pipes should be generated");
}

void TestFrameStackOrder() {
  ai::FrameStack stack(4, 4);

  stack.Push(std::vector<float>(4, 1.0F));
  stack.Push(std::vector<float>(4, 2.0F));
  stack.Push(std::vector<float>(4, 3.0F));
  stack.Push(std::vector<float>(4, 4.0F));

  const std::vector<float> tensor = stack.ToTensor();
  ASSERT_TRUE(tensor.size() == 16U, "stacked tensor should be frameStack * frameSize");
  ASSERT_TRUE(tensor.front() == 1.0F, "oldest frame should be first in tensor");
  ASSERT_TRUE(tensor.back() == 4.0F, "newest frame should be last in tensor");
}

void TestOnnxPolicyMissingModel() {
  ai::OnnxPolicy policy("dummy.onnx", 84, 4);
  ASSERT_TRUE(!policy.HasModel(), "missing model should keep policy disabled");
}

void TestAiAgentFallback() {
  app::Config config = app::Config::Default();
  util::Rng rng(13U);
  ai::OnnxPolicy policy("dummy.onnx", config.ai.inputSize, config.ai.frameStack);
  ai::AiAgent agent(config, &policy, &rng);

  ai::AiDecision decision = agent.FallbackDecision();
  ASSERT_TRUE(decision.probabilities[0] == 0.5F, "fallback prob[0] should be 0.5");
  ASSERT_TRUE(decision.probabilities[1] == 0.5F, "fallback prob[1] should be 0.5");
  ASSERT_TRUE((decision.action == 0 || decision.action == 1), "fallback action must be binary");
}

}  // namespace

int main() {
  const std::vector<TestResult> results = {
      Run("Config defaults", TestConfigDefaults),
      Run("Rng generation", TestRngRange),
      Run("Pipe logic", TestPipeGeneration),
      Run("Collision PixelPerfect", TestHitMaskAndCollision),
      Run("Game state logic", TestGameStateTransition),
      Run("FrameStack order", TestFrameStackOrder),
      Run("OnnxPolicy structure", TestOnnxPolicyMissingModel),
      Run("AiAgent logic", TestAiAgentFallback),
  };

  int failed = 0;
  for (const TestResult& result : results) {
    if (result.passed) {
      std::cout << "[PASS] " << result.name << '\n';
    } else {
      ++failed;
      std::cout << "[FAIL] " << result.name << " :: " << result.message << '\n';
    }
  }

  std::cout << "Total: " << results.size() << ", Failed: " << failed << '\n';
  return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
