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
