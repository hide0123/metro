#pragma once

namespace AST {

struct Argument : Base {
  std::string_view name;
  AST::Type* type;

  Argument(std::string_view const& name, Token const& colon,
           AST::Type* type);
  ~Argument();
};

struct Function : Base {
  Token const& name;  // 名前
  std::vector<Argument*> args;  // 引数

  Type* result_type;  // 戻り値の型
  Scope* code;  // 処理

  /**
   * @brief 引数を追加する
   *
   * @param name
   * @param type
   * @return Argument&
   */
  Argument*& append_argument(std::string_view const& name,
                             Token const& colon, AST::Type* type)
  {
    return this->args.emplace_back(
        new Argument(name, colon, type));
  }

  /**
   * @brief Construct a new Function object
   *
   * @param token
   * @param name
   */
  explicit Function(Token const& token, Token const& name);

  /**
   * @brief Destruct
   */
  ~Function();
};

}  // namespace AST
