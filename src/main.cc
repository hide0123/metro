#include <iostream>
#include "Application.h"

int main(int argc, char** argv)
{
#if !METRO_DEBUG
  try {
#endif
    return Application().main(argc, argv);
#if !METRO_DEBUG
  }
  catch (std::exception e) {
    std::cout << "unhandled exception has been occurred:"
              << std::endl
              << "  " << e.what() << std::endl
              << std::endl
              << "please report this bug" << std::endl;
  }

  return -1;
#endif
}