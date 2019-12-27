#pragma once
#include <vector>
#include <string_view>
#include <cstring>
#include <ctype.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sstream>
#include <chrono>

namespace Utils
{

constexpr std::size_t SEND_SIZE = 4096;
constexpr std::size_t READ_SIZE = 4096;

void print_error(std::string_view const& msg) noexcept
{
	std::cerr << "ERROR: " << msg << " Cause: " << std::strerror(errno) << std::endl;
}

void log(std::string_view const& msg) noexcept
{
	std::cout << "INFO: " << msg << std::endl;
}

int read(int fd, std::string& s)
{
	s.clear();
	char buf[READ_SIZE + 1];
	std::memset(buf, 0, READ_SIZE + 1);
	int count = 0;
	while(true)
	{
		count = recv(fd, buf, READ_SIZE, MSG_NOSIGNAL);
		if (count == -1)
		{
			//print_error("read: -1");
			if (errno == EINTR || errno == EAGAIN)
			{
				return s.size();
			}
			print_error("Error read process!");
			return -1;
		}

		if (count == 0 && errno == EAGAIN)
		{
			//print_error("read: 0 and EAGAIN");
			return 0;
		}

		if (count == 0)
		{
			//print_error("size: 0");
			return s.size();
		}

		for(std::size_t i = 0; i < static_cast<std::size_t>(count); ++i)
		{
			s.push_back(buf[i]);
		}
	}
	return s.size();
}

int sendall(int s, char *buf, int len, int flags) {
	int total = 0;
	int size = 0;

	while(total < len) {
		size = send(s, buf + total, len - total, flags);
		if(size == -1) {
			break;
		}
		total += size;
	}
	return size == -1 ? -1 : total;
}

int write(int fd, std::string_view const& s) {
	char buf[SEND_SIZE + 1];
	int len = std::min(SEND_SIZE, s.size());
	std::strncpy(buf, s.data(), len);
	return sendall(fd, buf, len, 0);
}

std::vector<std::pair<std::string, int>> lookup_host(std::string_view const& host)
{
	struct addrinfo hints, *res;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags |= AI_CANONNAME;

	if (getaddrinfo(host.data(), NULL, &hints, &res) != 0)
	{
		print_error("Can't take ip list.");
		return {};
	}

	log("Ok");
	log(host);

	std::vector<std::pair<std::string, int>> result;

	void *ptr;
	char addrstr[100];
	while (res)
	{
		inet_ntop (res->ai_family, res->ai_addr->sa_data, addrstr, 100);

		switch (res->ai_family)
		{
		case AF_INET:
			ptr = &(reinterpret_cast<struct sockaddr_in*>(res->ai_addr))->sin_addr;
			break;
		case AF_INET6:
			ptr = &(reinterpret_cast<struct sockaddr_in6*>(res->ai_addr))->sin6_addr;
			break;
		}

		inet_ntop (res->ai_family, ptr, addrstr, 100);
		result.push_back({ addrstr, res->ai_family == PF_INET6 ? 6 : 4 });

		res = res->ai_next;
	}

	return result;
}

std::string vector_to_str(std::vector<std::pair<std::string, int>> const& v)
{
	std::stringstream ss;
	ss << "-----------------------------------------------\n";
	for (auto const& [ip, version] : v)
	{
		ss << "IPv" << version << " : " << ip << "\n";
	}
	ss << "===============================================\n";
	return ss.str();
}

void prepare(std::string& request)
{
	std::stringstream ss;
	for (char c : request)
	{
		if (!std::isspace(c))
		{
			ss << c;
		}
	}
	request = ss.str();
}

long long get_current_time()
{
	using namespace std::chrono;
	return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

} // Utils