#include "common.h"

#include "AST.h"
#include "Object.h"
#include "BuiltinFunc.h"
#include "ASMOperand.h"

#include "Error.h"
#include "Compiler.h"

#define CC(e...) alertmsg("  " << e)

Compiler::Compiler() {

}

void Compiler::compile(AST::Base* _ast) {

  switch( _ast->kind ) {
    case AST_Value: {
      auto ast = (AST::Value*)_ast;

      CC(ast->token.str);
      break;
    }

    case AST_Return: {

      break;
    }

    case AST_Scope: {
      auto ast = (AST::Scope*)_ast;

      for( auto&& x : ast->list ) {
        this->compile(x);
      }

      break;
    }

    case AST_Function: {
      auto ast = (AST::Function*)_ast;

      const auto Epipuro=ast->var_count!=0;

      alertmsg(ast->name.str << ":");

     if(Epipuro){
      CC("push bp");
      CC("mov bp, sp");
      CC("add sp, " << ast->var_count);
     }

      this->compile()

    if(Epipuro){
      CC("pop bp");
      CC("ret");}

      break;
    }
  }

}

