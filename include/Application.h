#pragma once

#include <list>
#include <string>
#include <vector>
#include "ASTfwd.h"

class ScriptFileContext;
class Application {
public:
  Application();
  ~Application();

  int main(int argc, char** argv);

  void execute_full(ScriptFileContext& context);

  static void initialize();

  static Application& get_current_instance();

private:
  std::vector<ScriptFileContext> scripts;
};
