#pragma once

class Compiler {

public:

  Compiler();

  void compile(AST::Base* ast);

  AST::Function* cur_func{};
  bool f_flag_epipuro{};





private:


};

