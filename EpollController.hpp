#pragma once

#include <functional>
#include <memory>
#include <unordered_map>

#include <sys/epoll.h>
#include <unistd.h>

#include "FileDescriptor.hpp"

using Callback = std::function<void(uint32_t)>;

class EpollController {
public:

  EpollController();
  ~EpollController();

  std::vector<int> poll();

  bool epoll_ctl_wrap(int op, int fd, unsigned events);
  bool add_to_poll(int fd, unsigned mode, Callback const &function,
                   size_t timeout_s = std::numeric_limits<size_t>::max());
  bool delete_from_poll(int fd);
  bool modify_in_pool(int fd, int mode);

private:
  static bool is_active(std::size_t client_last_access_time_s, size_t timeout_s);
  std::vector<int> get_active_clients() const;
private:

  static constexpr size_t POLL_SIZE = 4096;
  static constexpr size_t TIMEOUT_WAIT = 10000;

  FileDescriptor epoll_fd;
  epoll_event raised_events[POLL_SIZE];
  std::unordered_map<int, Callback> handlers;
  std::unordered_map<int, std::size_t> clients_last_access_time_s;
  std::unordered_map<int, size_t> client_timeouts_s;
};