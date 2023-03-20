#pragma once

#include <cstdint>

struct Token;
struct Object;
struct BuiltinFunc;

enum ASTKind : uint8_t;

namespace AST {

enum CmpKind : uint8_t;
enum ExprKind : uint8_t;

struct Base;

template <class K, ASTKind _self_kind>
struct ExprBase;

struct Type;

//
// ConstKeyword:
//  Only have one token of value-keyword
struct ConstKeyword;

struct Value;
struct Variable;
struct CallFunc;

struct Cast;
struct Vector;
struct Dict;

struct UnaryOp;
struct IndexRef;
struct Range;
struct Assign;

struct If;
struct Return;

struct For;
struct While;
struct DoWhile;
struct Loop;
struct LoopController;

struct Scope;

struct Struct;
struct Function;

}  // namespace AST