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

  //
  // check if opened the path in any ScriptFileContext
  static ScriptFileContext const* get_context_with_path(
      std::string const& path);

private:
  std::vector<ScriptFileContext> _scripts;
};
