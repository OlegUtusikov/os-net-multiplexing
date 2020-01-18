#include <functional>

#include "Server.hpp"
#include "Utils.hpp"

Server::Server(int port) :
  stopped{false},
  socket(::socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)),
  epoll() {
  if (!socket.is_valid()) {
    LOGE("Cannot create server's socket %d", socket.get_fd());
    throw std::runtime_error("Can't create server's socket");
  }
  LOGI("Init server's socket: %d", socket.get_fd());

  sockaddr_in addr{};
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (::bind(socket.get_fd(), reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) != 0) {
    LOGE("Cannot bind server's socket: %d", socket.get_fd());
    throw std::runtime_error("ServerS, can't bind a socket");
  }

  if (::listen(socket.get_fd(), 1024) != 0) {
    LOGE("Cannot listen socket: %d", socket.get_fd());
    throw std::runtime_error("ServerS, can't listen on socket");
  }
  LOGI("Server created: %d", socket.get_fd());
}

void Server::start() {
  LOGI("Init server: %d", socket.get_fd());
  stopped = false;
  epoll.add_to_poll(STDIN_FILENO, EPOLLIN, [this](uint32_t) {
    static char buffer[1000];
    int len = ::read(STDIN_FILENO, buffer, sizeof(buffer));
    if (len > 0)
    {
      if (std::string(buffer, len).find("exit") != std::string::npos)
      {
        LOGI("Stopeed setted");
        stopped = true;
      }
    }
  });
  epoll.add_to_poll(socket.get_fd(), EPOLLIN, [this](uint32_t) {
    struct sockaddr_in addr{};
    socklen_t addr_size = sizeof(addr);
    int client = ::accept4(socket.get_fd(), reinterpret_cast<struct sockaddr *>(&addr), &addr_size, O_NONBLOCK);
    if (client == -1) {
      LOGE("Can't accept client");
      return;
    }
    LOGI("Accept client: %d", client);
    clients.emplace(client, std::make_shared<ClientFileDescriptor>(client));
    ClientFileDescriptor &cur_client = *clients.at(client);
    epoll.add_to_poll(client, cur_client.get_events(), [this, &cur_client](uint32_t events) {
      if (EPOLLIN & events) {
        LOGI("Catch EPOLLIN [client: %d]", cur_client.get_fd());
        cur_client.read();
        epoll.modify_in_pool(cur_client.get_fd(), cur_client.get_events());
      }

      if (EPOLLOUT & events) {
        LOGI("Catch EPOLLOUT [client: %d]", cur_client.get_fd());
        cur_client.write();
        epoll.modify_in_pool(cur_client.get_fd(), cur_client.get_events());
        if (cur_client.get_events() == EPOLLHUP) {
          LOGI("End upwriting and drop client: %d", cur_client.get_fd());
          drop_client(cur_client.get_fd());
          return;
        }
      }

      if ((EPOLLHUP | EPOLLERR) & events) {
        LOGI("Catch EPOLLHUP [client: %d]", cur_client.get_fd());
        LOGI("HUP client: %d", cur_client.get_fd());
        drop_client(cur_client.get_fd());
        return;
      }
    }, CLIENT_TIMEOUT_S);
  });

  LOGI("Init server's loop: %d", socket.get_fd());
  while (!stopped) {
    auto non_active_clients = epoll.poll();
    for (int client : non_active_clients) {
      clients.erase(client);
      LOGI("Drop inactive client: %d", client);
    }
  }
  LOGI("Stop server: %d", socket.get_fd());
}

void Server::drop_client(int fd) {
  epoll.delete_from_poll(fd);
  clients.erase(fd);
}