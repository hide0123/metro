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


      int addsp=0;

    if(ast->args.size()>4){
      addsp = ast->args.size()-4;

      CC("push bp");
      CC("mov bp, sp");
      CC("add sp, #" << addsp);
    }

      for(
        int i=((signed)ast->args.size())-1,j=0;;i--,j++){

        if(i>=4){
          this->compile(ast->args[i]);

          if(j)
            CC("str r0, [sp, #"<<j<<"]");
          else
            CC("str r0, [sp]");
          continue;
        }

        j=i;
        while(i>0){
          this->compile(ast->args[i]);
          CC("push r0");
          i--;
        }
        i=1;
        this->compile(ast->args[0]);
        while(i<=j){
        CC("pop r" << i);
        i++;
        }
        break;
      }

      CC("call " << ast->name);

      if(addsp) {
        CC("pop bp");
      }

      break;
    }

    case AST_Expr: {
      static char const* UWAA[]{
        "add",
        "sub",
        "mul",
        "div",
      };

      #define funcmacroxx(k) ({ \
        assert(static_cast<int>(k)<(int)std::size(UWAA)); \
        UWAA[static_cast<int>(k)];})

      astdef(Expr);

      auto it = ast->elements.rbegin();

      this->compile(it->ast);
      CC("push r0");

      while(it!=ast->elements.rend()-1){
        this->compile(it->ast);
        CC("pop r1");
        CC(funcmacroxx(it->kind) << " r0, r1");
        CC("push r0");
      }
      
      this->compile(ast->first);
      CC("pop r1");
        CC(funcmacroxx(it->kind) << " r0, r1");

      break;
    }

    case AST_Compare: {



      break;
    }

    case AST_If: {



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
      CC("add sp, #" << ast->var_count);
        
        for(int i=ast->args.size();i>=0;i--){
          if(i>3) i=3;

          if(i>0)
            CC("str r"<<i<<", [bp, #"<<i<<"]");
          else
            CC("str r"<<i<<", [bp]");
        }
     }

      this->compile(ast->code);

      if(ast->code->list.empty()
      ||(*ast->code->list.rbegin())->kind!=AST_Return){
        CC("pop bp");
        CC("ret");
      }

      break;
    }
  }

}

