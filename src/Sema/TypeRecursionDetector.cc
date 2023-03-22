#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Object.h"
#include "BuiltinFunc.h"

#include "Error.h"
#include "Sema.h"

#define astdef(T) auto ast = (AST::T*)_ast

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

  if (std::find(this->stack.begin(), this->stack.end(),
                ast) != this->stack.end()) {
    Error(ast, "recursive type '" + std::string(ast->name) +
                   "' have infinity size")
        .single_line()
        .emit();

    Error(*this->stack.rbegin(),
          "recursive without indirection")
        .emit(EL_Note)
        .exit();
  }

  this->stack.emplace_back(ast);

  switch (ast->kind) {
    case AST_Struct: {
      auto x = (AST::Struct*)ast;

      for (auto&& x : x->members) {
        this->walk(x.type);
      }

      break;
    }

    case AST_Type: {
      auto x = (AST::Type*)ast;

      if (auto find = this->S.find_struct(x->name); find) {
        this->walk(find);
      }

      break;
    }
  }

  this->stack.pop_back();
}
