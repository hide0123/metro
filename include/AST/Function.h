#pragma once

namespace AST {

struct Argument : Base {
  std::string_view name;
  AST::Type* type;

  Argument(std::string_view const& name, Token const& colon, AST::Type* type)
    : Base(AST_Argument, colon),
      name(name),
      type(type)
  {
  }

  ~Argument()
  {
    delete this->type;
  }
};

struct Function : Base {
  Token const& name;  // 名前
  std::vector<Argument*> args;  // 引数

  bool have_self;
  Typeable* self_type;

  Type* result_type;  // 戻り値の型
  Scope* code;  // 処理

  Base* return_binder;

  /**
   * @brief 引数を追加する
   *
   * @param name
   * @param type
   * @return Argument&
   */
  Argument*& append_argument(std::string_view const& name, Token const& colon,
                             AST::Type* type)
  {
    return this->args.emplace_back(new Argument(name, colon, type));
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
      have_self(false),
      self_type(nullptr),
      result_type(nullptr),
      return_binder(nullptr)
  {
  }

  ~Function()
  {
    for (auto&& arg : this->args) {
      delete arg;
    }

    if (this->result_type)
      delete this->result_type;

    delete this->code;
  }
};

}  // namespace AST
