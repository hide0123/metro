#pragma once

#include <list>
#include <string>
#include <vector>
#include "ASTfwd.h"

class Lexer;
class Parser;
class Sema;
class Evaluator;
class Application;

class ScriptFileContext {
  friend class Lexer;
  friend class Parser;
  friend class Sema;
  friend class Evaluator;
  friend class Application;

public:
  struct LineRange {
    size_t index;
    size_t begin;
    size_t end;
  };

  explicit ScriptFileContext(std::string const& path);
  ~ScriptFileContext();

  bool is_opened() const;

  bool open_file();
  bool import(std::string const& path, Token const& token,
              AST::Scope* add_to);

  bool lex();
  bool parse();
  bool check();
  Object* evaluate();

  void execute_full();

  std::string const& get_path() const;
  std::string const& get_source_code() const;

  std::vector<ScriptFileContext> const& get_imported_list()
      const;

  ScriptFileContext const* is_imported(
      std::string const& path) const;

private:
  struct SourceData {
    std::string _path;
    std::string _data;
    std::vector<LineRange> _lines;

    LineRange const* find_line_range(size_t srcpos) const
    {
      for (auto&& range : this->_lines) {
        if (range.begin <= srcpos && srcpos <= range.end)
          return &range;
      }

      return nullptr;
    }

    SourceData(std::string const& path);
  };

  bool _is_open;

  SourceData _srcdata;

  std::list<Token> _token_list;
  AST::Scope* _ast;

  //
  // if other file importing this, the pointer to that file
  ScriptFileContext const* _owner;

  Token const* _importer_token;

  std::vector<ScriptFileContext> _imported;
};
