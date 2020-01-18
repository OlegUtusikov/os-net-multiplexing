#include "Utils.hpp"

#include <cstdarg>

namespace Utils
{

void print_log(const char *type, const char *file, const char *func, int line, std::ostream &stream, const char *format, ...) {
	thread_local static char buffer[1000];

	std::string file_name(file);
	size_t pos = file_name.rfind("/");
	if (pos != std::string::npos) {
		file_name = file_name.substr(pos + 1);
	}

	va_list va;
	va_start(va, format);
	sprintf(buffer, format, va);
	va_end(va);

	stream << "{" << std::strerror(errno) << " [" << errno << "]" << "}  [" << file_name << ":" << func << ":" << line << "]  " << buffer << std::endl;
}

std::vector<std::pair<std::string, int>> lookup_host(std::string_view const& host)
{
	if (host.empty())
	{
		return {};
	}

	struct addrinfo hints, *res;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(host.data(), NULL, &hints, &res) != 0)
	{
		LOGE("Can't take ip list.");
		return {};
	}

	std::vector<std::pair<std::string, int>> result;

	void* ptr = nullptr;
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
	if (v.empty())
	{
		ss << "No IP adresses\n";
	}
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

std::size_t get_current_time()
{
	using namespace std::chrono;
	return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

} // Utils