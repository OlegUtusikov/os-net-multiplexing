#pragma once

class FileDescriptor
{
public:
	explicit FileDescriptor(int fd);
	FileDescriptor(FileDescriptor const& value) = delete;
	FileDescriptor(FileDescriptor&& other);
	FileDescriptor& operator=(FileDescriptor const& value) = delete;
	FileDescriptor& operator=(FileDescriptor&& other);
	virtual ~FileDescriptor();
	void close();
	bool is_valid() const;
	int get_fd() const;

private:
	int fd;
};