#pragma once

#include <list>
#include <string>
#include <vector>
#include "ASTfwd.h"

class ScriptFileContext {
public:
  ScriptFileContext(std::string const& path);
  ~ScriptFileContext();

  bool open_file();
  bool import(std::string const& path);

  bool lex();
  bool parse();
  bool check();

  Object* evaluate();

  std::string const& get_path() const;
  std::string const& get_source_code() const;

private:
  bool is_open;

  std::string file_path;
  std::string source_code;

  std::list<Token> token_list;
  AST::Scope* ast;

  std::vector<ScriptFileContext> imported;
};

class Application {
public:
  Application();
  ~Application();

  int main(int argc, char** argv);

  Object* execute_full(ScriptFileContext& context);

  static void initialize();

  static Application& get_current_instance();

private:
  std::vector<ScriptFileContext> scripts;
};
