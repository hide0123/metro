#include "Utils.h"
#include "Debug.h"

#include "AST/AST.h"
#include "Object.h"
#include "BuiltinFunc.h"

#include "Error.h"
#include "Sema.h"

#include "ASTWalker.h"

void Sema::TypeRecursionDetector::walk(AST::Typeable* ast)
{
  switch (ast->kind) {
    case AST_Type: {
      if (ast->name == "vector") {
        return;
      }

      break;
    }
  }

  if (std::find(this->stack.begin(), this->stack.end(), ast) !=
      this->stack.end()) {
    Error(ast,
          "recursive type '" + std::string(ast->name) + "' have infinity size")
      .single_line()
      .emit();

    Error(*this->stack.rbegin(), "recursive without indirection")
      .emit(EL_Note)
      .exit();
  }

  this->stack.emplace_back(ast);

  switch (ast->kind) {
    case AST_Enum: {
      auto x = (AST::Enum*)ast;

      for (auto&& e : x->enumerators) {
        if (e.value_type)
          this->walk(e.value_type);
      }

      break;
    }

    case AST_Struct: {
      auto x = (AST::Struct*)ast;

      for (auto&& m : x->members)
        this->walk(m.type);

      break;
    }

    case AST_Type: {
      auto x = (AST::Type*)ast;

      if (auto find = this->S.find_usertype(x->name); find) {
        this->walk(find);
      }

      break;
    }
  }

  this->stack.pop_back();
}

//
// 名前から型を探す
std::optional<TypeInfo> Sema::get_type_from_name(std::string_view name)
{
  if (auto builtin = TypeInfo::get_kind_from_name(name); builtin)
    return builtin.value();

  if (auto usertype = this->find_usertype(name); usertype) {
    TypeInfo ret{TYPE_UserDef};

    ret.userdef_type = usertype;

    if (usertype->kind == AST_Struct) {
      for (auto&& member : ((AST::Struct*)usertype)->members)
        ret.members.emplace_back(member.name, this->check(member.type));
    }

    return ret;
  }

  return std::nullopt;
}

//
// 名前からユーザー定義型を探す
AST::Typeable* Sema::find_usertype(std::string_view name)
{
  for (auto&& item : this->root->list) {
    switch (item->kind) {
      case AST_Enum:
      case AST_Struct:
        if (((AST::Typeable*)item)->name == name)
          return (AST::Typeable*)item;
    }
  }

  return nullptr;
}

// ----------
//  Expect value to ast
// ----------------
TypeInfo Sema::expect(TypeInfo const& expected, AST::Base* ast)
{
  auto type = this->check(ast);

  // 同じならそのまま返す
  if (expected.equals(type))
    return expected;

  switch (expected.kind) {
    case TYPE_USize:
      if (type.equals(TYPE_Int))
        goto matched;

      break;

    case TYPE_Vector: {
      if (ast->is_empty_vector())
        goto matched;

      break;
    }

    case TYPE_Dict: {
      auto x = (AST::Dict*)ast;

      if (ast->is_empty_scope())
        goto matched;

      if (ast->kind == AST_Dict && !x->key_type && x->elements.empty())
        goto matched;

      break;
    }

    case TYPE_UserDef: {
      if (expected.userdef_type->kind == AST_Enum) {
        if (type.kind == TYPE_Enumerator &&
            type.userdef_type == expected.userdef_type)
          goto matched;
      }

      break;
    }
  }

  //
  // ast がスコープ
  if (ast->kind == AST_Scope) {
    // 最後の要素を評価結果とする場合、エラー指摘場所をそれに変更する
    if (auto x = (AST::Scope*)ast; x->return_last_expr)
      ast = *x->list.rbegin();
  }

  Error(ast, "expected '" + expected.to_string() + "' but found '" +
               type.to_string() + "'")
    .emit()
    .exit();

matched:
  ast->use_default = true;
  return this->value_type_cache[ast] = expected;
}

TypeInfo Sema::as_lvalue(AST::Base* ast)
{
  this->expect_lvalue(ast);
  return this->check(ast);
}
