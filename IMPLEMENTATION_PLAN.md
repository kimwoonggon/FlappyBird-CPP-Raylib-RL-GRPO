# implementation_plan.md

## Goal
Implement a refactoring of the Flappy Bird C++ codebase, transitioning from a single monolithic file to a modular, object-oriented design using modern C++ features and adhering to TDD principles.

## User Review Required
> [!IMPORTANT]
> This plan involves significant structural changes. We will proceed iteratively, verifying functionality at each step using unit tests.
> The existing `main.cpp` will remain as a reference until the new structure is fully functional.

## Proposed Changes

### Phase 1: Infrastructure & Utils
#### [NEW] [Config.h](file:///Users/wgkim/Downloads/FlappyBird-CPP-Raylib-RL-GRPO-main/src/Config.h)
-   Define `Config` struct for game constants.

#### [NEW] [Util Modules](file:///Users/wgkim/Downloads/FlappyBird-CPP-Raylib-RL-GRPO-main/src/util/)
-   `Log.h`: Simple logging macros.
-   `Rng.h`: Random Number Generator wrapper.
-   `Time.h`: Time/Frame utilities.

### Phase 2: Graphics & Resources
#### [NEW] [gfx Modules](file:///Users/wgkim/Downloads/FlappyBird-CPP-Raylib-RL-GRPO-main/src/gfx/)
-   `RaylibContext.h/cpp`: RAII wrapper for Raylib initialization/shutdown.
-   `Resources.h/cpp`: RAII wrapper for loading/unloading assets.
-   `DebugOverlay.h/cpp`: Extracted debug drawing logic.

### Phase 3: Game Logic
#### [NEW] [game Modules](file:///Users/wgkim/Downloads/FlappyBird-CPP-Raylib-RL-GRPO-main/src/game/)
-   `Game.h/cpp`: Core game loop and state management.
-   `Bird.h/cpp`: Bird physics and state.
-   `Pipe.h/cpp`: Pipe generation and movement.
-   `Collision.h/cpp`: Pixel-perfect collision logic. (Refactor `CreateHitmask` here).

### Phase 4: AI Integration
#### [NEW] [ai Modules](file:///Users/wgkim/Downloads/FlappyBird-CPP-Raylib-RL-GRPO-main/src/ai/)
-   `AiAgent.h/cpp`: High-level AI controller.
-   `OnnxPolicy.h/cpp`: Wrapper for ONNX Runtime session and inference.
-   `Preprocess.h/cpp`: Image processing for AI input.
-   `FrameStack.h/cpp`: Frame buffer management.

### Phase 5: Application Entry
#### [NEW] [App Modules](file:///Users/wgkim/Downloads/FlappyBird-CPP-Raylib-RL-GRPO-main/src/app/)
-   `App.h/cpp`: Main application class binding everything together.

#### [MODIFY] [main.cpp](file:///Users/wgkim/Downloads/FlappyBird-CPP-Raylib-RL-GRPO-main/src/main.cpp)
-   Replace entire content with minimal bootstrap code using `App`.

### Build System
#### [MODIFY] [Makefile](file:///Users/wgkim/Downloads/FlappyBird-CPP-Raylib-RL-GRPO-main/Makefile)
-   Update source file list and include paths.

## Verification Plan
### Automated Tests
-   Unit tests will be written for `Collision`, `Game`, `OnnxPolicy`, and `Utils` using a C++ testing framework (e.g., doctest or Google Test).
-   `make test` command will be added to run tests.

### Manual Verification
-   Compare gameplay feel with the original version.
-   Verify resource usage (check for leaks).
-   Verify AI behavior (it should play competently).
