#pragma once

class FileDescriptor {
public:
  explicit FileDescriptor(int fd);
  FileDescriptor(FileDescriptor const &value) = delete;
  FileDescriptor(FileDescriptor &&other) noexcept;
  FileDescriptor &operator=(FileDescriptor const &value) = delete;
  FileDescriptor &operator=(FileDescriptor &&other) noexcept;
  virtual ~FileDescriptor();
  void close();
  bool is_valid() const;
  int get_fd() const;

private:
  int fd;
};