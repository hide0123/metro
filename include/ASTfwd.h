#pragma once

#include <cstdint>

struct Object;
struct BuiltinFunc;

enum ASTKind : uint8_t;

namespace AST {

enum CmpKind : uint8_t;
enum ExprKind : uint8_t;

struct Type;

struct Base;

struct ConstKeyword;

struct Value;
struct Variable;
struct CallFunc;

struct Cast;
struct Array;
struct Dict;

struct UnaryOp;
struct IndexRef;
struct Range;
struct Assign;

template <class K, ASTKind _self_kind>
struct ExprBase;

struct Return;
struct LoopController;
struct If;
struct For;
struct While;
struct DoWhile;
struct Loop;
struct Scope;

struct Function;

}  // namespace AST