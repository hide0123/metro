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

  //
  // check if opened the path in any ScriptFileContext
  ScriptFileContext const* get_context(std::string const& path) const;

  ScriptFileContext const* get_current_context() const;

  static void initialize();

  static Application* get_instance();

private:
  ScriptFileContext& add_context(std::string const& path);

  ScriptFileContext const* _cur_ctx;
  std::vector<ScriptFileContext> _contexts;
};
