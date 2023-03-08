#pragma once

namespace AST {

struct Function : Base {
  struct Argument {
    Token const& name;
    AST::Type* type;

    explicit Argument(Token const& name, AST::Type* type)
        : name(name),
          type(type)
    {
    }

    ~Argument()
    {
      delete this->type;
    }
  };

  Token const& name;  // 名前
  std::vector<Argument> args;  // 引数

  Type* result_type;  // 戻り値の型
  Scope* code;  // 処理

  /**
   * @brief 引数を追加する
   *
   * @param name
   * @param type
   * @return Argument&
   */
  Argument& append_argument(Token const& name, AST::Type* type)
  {
    return this->args.emplace_back(name, type);
  }

  /**
   * @brief Construct a new Function object
   *
   * @param token
   * @param name
   */
  explicit Function(Token const& token, Token const& name)
      : Base(AST_Function, token),
        name(name),
        result_type(nullptr)
  {
  }

  ~Function();
};

}  // namespace AST
