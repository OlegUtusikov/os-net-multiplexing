#include "ClientFileDescriptor.hpp"

#include <algorithm>
#include <stdexcept>

#include <sys/epoll.h>
#include <unistd.h>

#include "Utils.hpp"

ClientFileDescriptor::ClientFileDescriptor(int fd) :
  FileDescriptor(fd),
  flags(EPOLLIN | EPOLLHUP | EPOLLERR),
  buffer{} {
  LOGI("Create CFD: %d", fd);
}

ClientFileDescriptor::~ClientFileDescriptor() {
  LOGI("Destroy CFD: %d", get_fd());
}

void ClientFileDescriptor::read() {
  int len = ::read(get_fd(), buffer, BUFFER_SIZE);
  if (len == 0) {
    flags &= ~EPOLLIN;
    return;
  }
  if (len > 0) {
    read_buffer += std::string(buffer, len);
    parse_read_buffer();
    if (!write_buffer.empty()) {
      flags |= EPOLLOUT;
    }
  }
}

void ClientFileDescriptor::parse_read_buffer() {
  size_t pos = std::min(read_buffer.find('\n'), read_buffer.find('\r'));
  while (pos != std::string::npos) {
    std::string domain = read_buffer.substr(0, pos);
    read_buffer = read_buffer.substr(pos + 1);
    pos = std::min(read_buffer.find('\n'), read_buffer.find('\r'));
    if (!std::all_of(domain.begin(), domain.end(), isspace)) {
      LOGI("Parsed domain '%s' for client %d", domain.c_str(), get_fd());
      Utils::prepare(domain);
      write_buffer += Utils::vector_to_str(Utils::lookup_host(domain));
    }
  }
}

void ClientFileDescriptor::write() {
  int len = ::write(get_fd(), write_buffer.c_str(), write_buffer.size());
  if (len > 0) {
    write_buffer = write_buffer.substr(len);
  }
  if (write_buffer.empty()) {
    flags &= ~EPOLLOUT;
  }
}