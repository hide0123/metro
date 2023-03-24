// ---------------------------------------------
//  AST
// ---------------------------------------------
#pragma once

#include <map>
#include <vector>
#include <concepts>

#include "Token.h"
#include "TypeInfo.h"
#include "ASTfwd.h"

#include "AST/Kind.h"
#include "AST/Base.h"

#include "AST/Expr.h"
#include "AST/Stmt.h"

#include "AST/Typeable.h"

#include "AST/Enum.h"
#include "AST/Struct.h"
#include "AST/Function.h"

#include "AST/Impl.h"