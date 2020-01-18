#include <iostream>
#include <memory>

#include "Server.hpp"
#include "Utils.hpp"

int main(int argc, char **argv) {
  int port;
  if (argc > 2) {
    LOGE("Too many arguments: %d", argc);
    return 0;
  } else if (argc == 2) {
    if (std::isdigit(argv[1][0])) {
      port = std::stoi(argv[1]);
    } else {
      LOGE("Bad arg: %s", argv[1]);
      return 0;
    }
  } else {
    LOGE("Too few arguments: %d", argc);
    return 0;
  }

  Server server(port);
  server.start();

  return 0;
}