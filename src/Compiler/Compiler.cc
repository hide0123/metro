#include "common.h"

#include "AST.h"
#include "Object.h"
#include "BuiltinFunc.h"
#include "ASMOperand.h"

#include "Error.h"
#include "Compiler.h"

#define CC(e...) alertmsg("  " << e)

#define astdef(T) auto ast=(AST::T*)_ast

Compiler::Compiler() {

}

void Compiler::compile(AST::Base* _ast) {

  switch( _ast->kind ) {
    case AST_Value: {
      astdef(Value);

      CC(ast->token.str);
      break;
    }

    case AST_Variable: {
      astdef(Variable);
      
      CC("UOO "<< ast);

      break;
    }

    case AST_Return: {

      break;
    }

    case AST_Scope: {
      astdef(Scope);

      for( auto&& x : ast->list ) {
        this->compile(x);
      }

      break;
    }

    case AST_Function: {
      astdef(Function);

      const auto Epipuro=ast->var_count!=0;

      alertmsg(ast->name.str << ":");

     if(Epipuro){
      CC("push bp");
      CC("mov bp, sp");
      CC("add sp, " << ast->var_count);
     }

      this->compile(ast->code);

    if(Epipuro){
      CC("pop bp");
      CC("ret");}

      break;
    }
  }

}

