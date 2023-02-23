#ifndef DEC_PRECLUDE_H
#define DEC_PRECLUDE_H

#include <string>
#include <iostream>

void dec_check(bool con, std::string message) {
  if (!con) {
    std::cerr << "Failed: " << message << "\n";
    exit(1);
  }
}
#endif