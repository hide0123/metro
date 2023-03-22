// ---------------------------------------------
//  Lexer
// ---------------------------------------------
#pragma once

#include <functional>
#include <string>
#include <list>

struct Token;
class ScriptFileContext;
class Lexer {
public:
  Lexer(ScriptFileContext& context);
  ~Lexer();

  std::list<Token> lex();

private:
  bool check();
  char peek();
  bool match(std::string_view str);

  size_t pass_while(std::function<bool(char)> cond);
  void pass_space();

  bool find_punctuator(Token& token);

  std::string& source;
  size_t position;

  ScriptFileContext& _context;
};