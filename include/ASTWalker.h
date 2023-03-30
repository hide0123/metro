#pragma once

#include <functional>
#include "AST/ASTfwd.h"

namespace ASTWalker {

bool walk(AST::Base* ast, std::function<bool(AST::Base*)> func);

}