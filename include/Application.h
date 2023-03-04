#pragma once

#include <string>
#include "ASTfwd.h"

class Application {
  struct ScriptFileContext {
    std::string file_path;
    std::string source_code;

    std::list<Token> token_list;
    AST::Base* ast;

    bool open_file();

    bool lex();
    bool parse();
    bool check();

    Object* evaluate();

    ScriptFileContext(std::string const& path);
    ~ScriptFileContext();
  };

public:
  Application();
  ~Application();

  int main(int argc, char** argv);

  std::string const& get_source_code();

  static void initialize();

  static Application& get_current_instance();

  std::string file_path;
  std::string source_code;
};
