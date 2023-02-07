#include <cassert>
#include "lcc.h"

Evaluator::Evaluator() {
  
}

Evaluator::~Evaluator() {
  
}

Object* Evaluator::create_object(AST::Value* ast) {
  auto type = TypeChecker::value_type_cache[ast];

  auto& obj = this->immediate_objects[ast];

  if( obj )
    return obj;

  switch( type.kind ) {
    case TYPE_Int:
      obj = new ObjLong(std::stoi(ast->token.str.data()));
      break;

    default:
      todo_impl;
  }

  assert(obj != nullptr);

  return obj;
}

Object* Evaluator::evaluate(AST::Base* ast) {
  if( !ast )
    return Object::obj_none;

  switch( ast->kind ) {
    case AST_Value: {
      return Evaluator::create_object((AST::Value*)ast);
    }

    case AST_Expr: {

      auto x = (AST::Expr*)ast;

      auto ret = this->evaluate(x->first);

      for( auto&& elem : x->elements ) {
        auto item = this->evaluate(elem.ast);

        switch( elem.kind ) {
          case AST::Expr::EX_Add:
            ret = Evaluator::add_object(ret, item);
            break;
        }
      }

      return ret;
    }
  }

  return Object::obj_none;
}

Object* Evaluator::add_object(Object* left, Object* right) {
  // todo: if left is vector

  auto ret = left->clone();

  switch( left->type.kind ) {
    case TYPE_Int:
      ((ObjLong*)ret)->value += ((ObjLong*)right)->value;
      break;
  }

  return ret;
}
