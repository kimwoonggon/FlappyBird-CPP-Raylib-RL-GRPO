#include "app/App.h"
#include "app/Config.h"

int main() {
  const app::Config config = app::Config::Default();
  app::App app(config);
  return app.Run();
}
