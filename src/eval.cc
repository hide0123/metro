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
  }

  return Object::obj_none;
}