#pragma once

#include <unordered_map>
#include "EpollController.hpp"
#include "ClientFileDescriptor.hpp"
#include <memory>

class Server {
public:
  explicit Server(int port);
  void start();
  ~Server() = default;
private:
  void drop_client(int fd);
private:
  static constexpr size_t CLIENT_TIMEOUT_S = 60;

  bool stopped;
  EpollController epoll;
  FileDescriptor socket;
  std::unordered_map<int, std::shared_ptr<ClientFileDescriptor>> clients;
};