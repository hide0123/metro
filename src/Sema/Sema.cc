#include "Utils.h"
#include "debug/alert.h"

#include "Sema.h"

#include "Error.h"
#include "Object.h"
#include "BuiltinFunc.h"

#define astdef(T) auto ast = (AST::T*)_ast

std::map<AST::Base*, TypeInfo> Sema::value_type_cache;

Sema::Sema(AST::Scope* root)
    : root(root)
{
}

Sema::~Sema()
{
}

void Sema::do_check()
{
  TypeRecursionDetector tr{*this};

  for (auto&& x : this->root->list) {
    switch (x->kind) {
      case AST_Struct:
        tr.walk((AST::Typeable*)x);
        break;
    }
  }

  this->check(this->root);
}