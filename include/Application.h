#pragma once

#include <string>

class Application {


public:
  Application();
  ~Application();

  int main(int argc, char** argv);

  static void initialize(); // これいるのか？

private:


  std::string source_code;

};

