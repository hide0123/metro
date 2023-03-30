// ---------------------------------------------
//  AST
// ---------------------------------------------
#pragma once

#define astdef(T) auto ast = (AST::T*)_ast

#include <map>
#include <vector>
#include <concepts>

#include "Token.h"
#include "TypeInfo.h"
#include "ASTfwd.h"

#include "Kind.h"
#include "Base.h"

#include "Expr/Expr.h"
#include "Stmt/Stmt.h"

#include "AST/Typeable.h"

#include "AST/Enum.h"
#include "AST/Struct.h"
#include "AST/Function.h"

#include "AST/Impl.h"
