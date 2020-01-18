#include "EpollController.hpp"

#include "Utils.hpp"

// static
bool EpollController::is_active(size_t client_last_access_time_s, size_t timeout_s) {
  std::size_t cur_time = Utils::get_current_time();
  return (cur_time - client_last_access_time_s) <= timeout_s;
}

bool EpollController::epoll_ctl_wrap(int op, int fd, unsigned events) {
  epoll_event event{};
  event.data.fd = fd;
  event.events = events;
  return epoll_ctl(epoll_fd.get_fd(), op, fd, &event) >= 0;
}

bool EpollController::add_to_poll(int fd, unsigned ev, Callback const &function, size_t timeout_s) {
  bool res = epoll_ctl_wrap(EPOLL_CTL_ADD, fd, ev);
  if (res) {
    std::size_t cur_time = Utils::get_current_time();
    handlers[fd] = function;
    clients_last_access_time_s[fd] = cur_time;
    client_timeouts_s[fd] = timeout_s;
  }
  return res;
}

bool EpollController::delete_from_poll(int fd) {
  bool res = epoll_ctl_wrap(EPOLL_CTL_DEL, fd, 0);
  if (res) {
    handlers.erase(fd);
    clients_last_access_time_s.erase(fd);
    client_timeouts_s.erase(fd);
  }
  return res;
}

bool EpollController::modify_in_pool(int fd, int mode) {
  return epoll_ctl_wrap(EPOLL_CTL_MOD, fd, mode);
}

EpollController::EpollController() :
  epoll_fd(epoll_create(POLL_SIZE)),
  raised_events{} {
  if (!epoll_fd.is_valid()) {
    LOGE("Cannot create epoll: %d", epoll_fd.get_fd());
    throw std::runtime_error("Can't create a epoll");
  }
  LOGI("Epoll created: %d", epoll_fd.get_fd());
}

std::vector<int> EpollController::poll() {
  LOGI("Start polling %d", epoll_fd.get_fd());
  std::vector<int> timeoutClients;
  int ready = epoll_wait(epoll_fd.get_fd(), raised_events, POLL_SIZE, TIMEOUT_WAIT);
  if (ready == -1) {
    if (errno != EINTR) {
      LOGE("Waiting error!");
    }
  }
  for (std::size_t i = 0; i < ready; ++i) {
    int cur_fd = raised_events[i].data.fd;
    std::size_t cur_time = Utils::get_current_time();
    clients_last_access_time_s[cur_fd] = cur_time;
    handlers[cur_fd](raised_events[i].events);
  }

  if (ready == 0) {
    LOGI("Timeout wait");
  }

  for (int client : get_active_clients()) {
    if (!is_active(clients_last_access_time_s[client], client_timeouts_s[client])) {
      delete_from_poll(client);
      timeoutClients.push_back(client);
    }
  }
  return timeoutClients;
}

std::vector<int> EpollController::get_active_clients() const {
  std::vector<int> result;
  for (auto const&[fd, time] : clients_last_access_time_s) {
    result.push_back(fd);
  }
  return result;
}

EpollController::~EpollController() {
  LOGI("Epoll is closing");
  for (int client : get_active_clients()) {
    delete_from_poll(client);
  }
}