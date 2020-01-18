#include <stdexcept>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "Utils.hpp"
#include "ClientFileDescriptor.hpp"

ClientFileDescriptor::ClientFileDescriptor(int fd) : FileDescriptor(fd), flags(EPOLLIN | EPOLLHUP)
{
	LOGI("Create CFD: %d", fd);
}

ClientFileDescriptor::~ClientFileDescriptor() {
	LOGI("Destroy CFD: %d", get_fd());
}

void ClientFileDescriptor::read()
{
	int len = ::read(get_fd(), buffer, BUFFER_SIZE);
	if (len == 0)
	{
		flags &= ~EPOLLIN;
		return;
	}
	if (len > 0)
	{
		read_buffer += std::string(buffer, len);
		parse_read_buffer();
		if (!write_buffer.empty())
		{
			flags |= EPOLLOUT;
		}
	}
}

void ClientFileDescriptor::parse_read_buffer()
{
	size_t pos = read_buffer.find("\n");
	while (pos != std::string::npos)
	{
		std::string domain = read_buffer.substr(0, pos);
		LOGI("Parsed domain '%s' for client %d", domain.c_str(), get_fd());
		read_buffer = read_buffer.substr(pos + 1);
		pos = read_buffer.find("\n");
		Utils::prepare(domain);
		write_buffer += Utils::vector_to_str(Utils::lookup_host(domain));
	}
}

void ClientFileDescriptor::write()
{
	int len = ::write(get_fd(), write_buffer.c_str(), write_buffer.size());
	if (len > 0)
	{
		write_buffer = write_buffer.substr(len);
	}
	if (write_buffer.empty())
	{
		flags |= ~EPOLLOUT;
	}
}