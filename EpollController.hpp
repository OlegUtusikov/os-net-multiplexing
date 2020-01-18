
#pragma once
#include <sys/epoll.h>
#include <unistd.h>
#include <unordered_map>
#include <functional>
#include <memory>
#include "FileDescriptor.hpp"

using Callback = std::function<void(uint32_t)>;
class EpollController
{
public:

	EpollController();
	~EpollController();

	std::vector<int> poll();

	bool epoll_ctl_wrap(int op, int fd, unsigned events);
	bool add_to_poll(int fd, unsigned mode, Callback const& function);
	bool delete_from_poll(int fd);
	bool modify_in_pool(int fd, int mode);

private:
	static bool is_active(std::size_t client);
	std::vector<int> get_active_clients() const;
private:

	enum {
		POLL_SIZE = 4096,
		TIMEOUT_WAIT = 10000,
		TIMEOUT_CLIENT = 60,
	};

	FileDescriptor m_epoll;
	bool stoped { false };
	epoll_event event;
	epoll_event events[POLL_SIZE];
	std::unordered_map<int, Callback> functions;
	std::unordered_map<int, std::size_t> clients;
};