#include <iostream>

#include "Utils.h"
#include "debug/alert.h"

#include "Application.h"

int main(int argc, char** argv)
{
  try {
    return Application().main(argc, argv);
  }
  catch (std::exception e) {
    alertmsg(e.what());
  }
  catch (...) {
    alert;
  }

  return -1;
}