// ------------------------------------------ //
//  semantics analyzer
// ------------------------------------------ //

#pragma once

#include <functional>
#include <optional>
#include <tuple>
#include <list>
#include <map>

#include "AST.h"
#include "TypeInfo.h"

class Evaluator;
class Sema {
  friend class Evaluator;

  using TypeVector = std::vector<TypeInfo>;

  struct LocalVar {
    TypeInfo type;
    std::string_view name;

    size_t step;
    size_t index;

    bool is_global = 0;

    explicit LocalVar(TypeInfo const& type, std::string_view name)
      : type(type),
        name(name),
        step(0),
        index(0)
    {
    }
  };

  struct LocalVarList {
    std::vector<LocalVar> variables;

    LocalVar* find_var(std::string_view name)
    {
      for (auto&& var : this->variables)
        if (var.name == name)
          return &var;

      return nullptr;
    }

    LocalVar& append(TypeInfo const& type, std::string_view name)
    {
      return this->variables.emplace_back(type, name);
    }
  };

  struct SemaScope {
    AST::Base* ast;
    LocalVarList lvar;

    bool is_loop;
    bool is_breakable;
    bool is_continueable;

    SemaScope(AST::Base* ast)
      : ast(ast),
        is_loop(false),
        is_breakable(false),
        is_continueable(false)
    {
    }
  };

  struct FunctionContext {
    AST::Function* ast;

    TypeInfo result_type;

    std::map<AST::Return*, TypeInfo> return_stmt_types;
  };

  class TypeRecursionDetector {
    Sema& S;

  public:
    TypeRecursionDetector(Sema& S)
      : S(S)
    {
    }

    void walk(AST::Typeable* ast);

    std::vector<AST::Typeable*> stack;
  };

  struct FunctionFindResult {
    enum FunctionType {
      FN_Builtin,
      FN_UserDefined,
    };

    FunctionType type;
    BuiltinFunc const* builtin;
    AST::Function* userdef;

    explicit FunctionFindResult(BuiltinFunc const* b = nullptr)
      : type(FN_Builtin),
        builtin(b),
        userdef(nullptr)
    {
    }

    FunctionFindResult(AST::Function* f)
      : type(FN_UserDefined),
        builtin(nullptr),
        userdef(f)
    {
    }
  };

public:
  Sema(AST::Scope* root);
  ~Sema();

  void do_check();

  void compare_argument(TypeVector const& formal, TypeVector const& actual);

  TypeInfo check(AST::Base* ast);
  TypeInfo check_function_call(AST::CallFunc* ast);

  TypeInfo& check_as_left(AST::Base* ast);

  TypeInfo check_indexref(AST::IndexRef* ast);

  //
  // 構造体
  void check_struct(AST::Struct* ast);

  //
  // 演算子の型の組み合わせが正しいかチェックする
  std::optional<TypeInfo> is_valid_expr(AST::ExprKind kind, TypeInfo const& lhs,
                                        TypeInfo const& rhs);

  //
  // 関数を探す
  FunctionFindResult find_function(std::string_view name, bool have_self,
                                   TypeInfo const& self_type,
                                   std::vector<TypeInfo> const& args);

  AST::Typeable* find_usertype(std::string_view name);

  //
  // ビルトイン関数を探す
  // BuiltinFunc const* find_builtin_func(std::string_view name);

  //
  // 名前から型を探す
  std::optional<TypeInfo> get_type_from_name(std::string_view name);

private:
  using CaptureFunction = std::function<void(AST::Base*)>;
  using ReturnCaptureFunction =
    std::function<void(TypeInfo const&, AST::Base*)>;

  struct CaptureContext {
    CaptureFunction func;

    CaptureContext(CaptureFunction f)
      : func(f)
    {
    }
  };

  void begin_capture(CaptureFunction func);
  void end_capture();

  void begin_return_capture(ReturnCaptureFunction func);
  void end_return_capture();

  int find_member(TypeInfo const& type, std::string_view name)
  {
    for (int i = 0; auto&& m : ((AST::Struct*)type.userdef_type)->members) {
      if (m.name == name)
        return i;

      i++;
    }

    return -1;
  }

  SemaScope& get_cur_scope()
  {
    return *this->scope_list.begin();
  }

  SemaScope& enter_scope(AST::Scope* ast)
  {
    return this->scope_list.emplace_front(ast);
  }

  void leave_scope()
  {
    this->scope_list.pop_front();
  }

  // 今いる関数を返す
  // 関数の中にいなければ nullptr を返す
  AST::Function* get_cur_func();

  TypeInfo expect(TypeInfo const& type, AST::Base* ast);

  AST::Scope* root;

  std::list<SemaScope> scope_list;
  std::list<AST::Function*> function_history;

  std::vector<AST::Typeable*> type_check_stack;

  // captures
  std::vector<CaptureContext> captures;
  std::vector<ReturnCaptureFunction> return_captures;

  static std::map<AST::Base*, TypeInfo> value_type_cache;
};
