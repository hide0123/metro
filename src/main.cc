#include <iostream>
#include "Application.h"

int main(int argc, char** argv)
{
  try {
    return Application().main(argc, argv);
  }
  catch (std::exception e) {
    std::cout << "unhandled exception has been occurred:"
              << std::endl
              << "  " << e.what() << std::endl
              << std::endl
              << "please report this bug" << std::endl;
  }

  return -1;
}