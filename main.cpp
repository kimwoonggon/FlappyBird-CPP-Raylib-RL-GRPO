/**
 * @file main.cpp
 * @brief Implementation for main.
 */

#include "app/App.h"
#include "app/Config.h"

/**
 * @brief Program entry point for legacy top-level binary target.
 * @return Process exit code.
 */
int main() {
  const app::Config config = app::Config::Default();
  app::App app(config);
  return app.Run();
}
