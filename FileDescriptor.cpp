#include <stdexcept>
#include <unistd.h>
#include "FileDescriptor.hpp"
#include "Utils.hpp"

FileDescriptor::FileDescriptor(int fd) : fd(fd) {
	LOGI("Create FD: %d", fd);
}

FileDescriptor::FileDescriptor(FileDescriptor&& other) : fd(other.fd)
{
	other.fd = -1;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other)
{
	if (&other == this)
	{
		return *this;
	}
	fd = other.fd;
	other.fd = -1;
	return *this;
}

FileDescriptor::~FileDescriptor()
{
	if (is_valid())
	{
		LOGI("Destroy FD: %d", fd);
		close();
	}
}

void FileDescriptor::close()
{
	if (::close(fd) < 0)
	{
		LOGE("Can't close socket");
	}
}

bool FileDescriptor::is_valid() const
{
	return fd > 0;
}

int FileDescriptor::get_fd() const
{
	return fd;
}