#include <cassert>
#include <iostream>
#include "lcc.h"

Error& Error::emit() {
  std::cerr << this->msg << std::endl;

  return *this;
}

void Error::exit(int code) {
  std::exit(code);
}


std::string TypeInfo::to_string() const {
  static std::map<TypeKind, char const*> kind_name_map {
    { TYPE_Int, "int" },
    { TYPE_Float, "float" },
  };

  assert(kind_name_map.contains(this->kind));

  std::string s = kind_name_map[this->kind];

  if( this->is_mutable ) {
    s += " mut";
  }

  return s;
}

bool TypeInfo::equals(TypeInfo const& type) const {
  return
    this->kind == type.kind
    && this->is_mutable == type.is_mutable;
}