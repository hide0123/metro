#include "common.h"

#include "AST.h"
#include "Object.h"
#include "BuiltinFunc.h"
#include "ASMOperand.h"

#include "Error.h"
#include "Compiler.h"

#define CC(e...) alertmsg("  " << e)

#define astdef(T) auto ast=(AST::T*)_ast

using namespace std::string_literals;

Compiler::Compiler() {

}

void Compiler::compile(AST::Base* _ast) {

  switch( _ast->kind ) {
    case AST_Value: {
      astdef(Value);

      CC("mov r0, #"<< ast->token.str);
      break;
    }

    case AST_Variable: {
      astdef(Variable);
      
      if(ast->index==0)
        CC("ldr r0, [bp]");
      else
        CC("ldr r0, [bp, #"<<ast->index<<"]");

      break;
    }

    case AST_CallFunc: {
      astdef(CallFunc);

      int i=ast->args.size();

      std::all_of(
        ast->args.rbegin(), ast->args.rend(),
      [&] (AST::Base*& arg) -> bool {
        this->compile(arg);

        if(i>=4){

        } else {

        }

        i--;
      });

      break;
    }

    case AST_Expr: {
      static char const* UWAA[]{
        "add",
        "sub",
        "add",
      };

      astdef(Expr);

      auto it = ast->elements.rbegin();

      this->compile(it->ast);
      CC("push r0");

      
      this->compile(ast->first);
      CC("pop r1");


      break;
    }

    case AST_Compare: {
      astdef(Compare);

      break;
    }

    case AST_If: {
      astdef(If);

      this->compile(ast->condition);



      break;
    }

    case AST_Return: {
      astdef(Return);

      this->compile(ast->expr);

      if(this->f_flag_epipuro){
        CC("pop bp");
      }

      CC("ret");

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

      this->cur_func=ast;
      this->f_flag_epipuro=ast->var_count!=0;

      alertmsg(ast->name.str << ":");

     if(this->f_flag_epipuro){
      CC("push bp");
      CC("mov bp, sp");
      CC("add sp, " << ast->var_count);

       for(size_t i=0;auto&&arg:ast->args){
        if(i<=3){
        CC("str r"<<i<<", [bp"<<(
        i==0?"":", #"s+std::to_string(i)
        )<<"]");
        }
        else{
          
        }


       i++; }
     }

      this->compile(ast->code);

      if(this->f_flag_epipuro)
        CC("pop bp");

      CC("ret");
      break;
    }
  }

}

