#pragma once

#include <string>
#include "FileDescriptor.hpp"

class ClientFileDescriptor : public FileDescriptor {
public:
  explicit ClientFileDescriptor(int fd);
  ~ClientFileDescriptor() override;
  void read();
  void write();
  uint32_t get_events() const { return flags; };

private:
  void parse_read_buffer();

private:
  static constexpr int BUFFER_SIZE = 1024 * 1024;
  uint32_t flags;
  char buffer[BUFFER_SIZE];
  std::string read_buffer;
  std::string write_buffer;
};