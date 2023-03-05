#pragma once

#include <list>
#include <string>
#include <vector>
#include "ASTfwd.h"

class Application;
class ScriptFileContext {
public:
  explicit ScriptFileContext(std::string const& path);
  ~ScriptFileContext();

  bool open_file();
  bool import(std::string const& path, Token const& token,
              AST::Scope* add_to);

  bool lex();
  bool parse();
  bool check();

  void evaluate();

  std::string const& get_path() const;
  std::string const& get_source_code() const;

  std::vector<ScriptFileContext> const& get_imported_list()
      const;

  ScriptFileContext const* is_imported(
      std::string const& path) const;

private:
  bool _is_open;

  std::string _file_path;
  std::string _source_code;

  std::list<Token> _token_list;
  AST::Scope* _ast;

  //
  // if other file importing this, the pointer to that file
  ScriptFileContext const* _owner;

  std::vector<ScriptFileContext> _imported;
};
