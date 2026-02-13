# Flappy Bird Refactoring Specification

## Goal
Refactor the existing monolithic `main.cpp` Flappy Bird clone into a modern C++17 modular architecture.
Maintain identical gameplay behavior while improving code quality, memory management, and extensibility.

## Core Requirements
1.  **Modular Architecture**: Split `main.cpp` into `App`, `Game`, `AiAgent`, `Graphics`, and `Utils` modules.
2.  **Resource Management**: Use RAII for all Raylib resources (Textures, Sounds, Fonts, etc.). No manual `Unload*` calls.
3.  **Memory Management**: Eliminate `malloc/free`. Use `std::vector` and smart pointers.
4.  **Global State**: Remove all mutable global variables. Pass dependencies explicitly or use a `Context` object.
5.  **AI Integration**: Encapsulate ONNX Runtime logic within `AiAgent` and `OnnxPolicy`.
6.  **Configuration**: Load game settings (screen size, physics, etc.) from a `Config` struct.
7.  **Build System**: Update `Makefile` to support multi-file compilation.

## Technical Constraints
-   **Language**: C++17 or later.
-   **Libraries**: Raylib, ONNX Runtime (Cpp API).
-   **Testing**: Unit tests for logic (Game, AI, Utils).

## Refactoring Steps (Summary)
1.  Setup Project Structure & Build System.
2.  Implement `Config` and `Utils` (Logging, RNG).
3.  Implement `RaylibContext` and `Resources` (RAII).
4.  Implement `Game` Logic (State, Physics, Collision).
5.  Implement `AiAgent` and `OnnxPolicy`.
6.  Integrate `App` and Main Entry Point.
7.  Verify feature parity.
