#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>

#include "EpollController.hpp"
#include "Utils.hpp"

// static
bool EpollController::is_active(std::size_t client)
{
	std::size_t cur_time = Utils::get_current_time();
	return (cur_time - client) <= TIMEOUT_CLIENT;
}

bool EpollController::epoll_ctl_wrap(int op, int fd, unsigned events)
{
	epoll_event event{};
	event.data.fd = fd;
	event.events = events;
	return epoll_ctl(m_epoll.get_fd(), op, fd, &event) >= 0;
}

bool EpollController::add_to_poll(int fd, unsigned ev, Callback const& function)
{
	bool res = epoll_ctl_wrap(EPOLL_CTL_ADD, fd, ev);
	if (res)
	{
		std::size_t cur_time = Utils::get_current_time();
		functions[fd] = function;
		clients[fd] = cur_time;
	}
	return res;
}

bool EpollController::delete_from_poll(int fd)
{
	bool res = epoll_ctl_wrap(EPOLL_CTL_DEL, fd, 0);
	if (res)
	{
		functions.erase(fd);
		clients.erase(fd);
	}
	return res;
}

bool EpollController::modify_in_pool(int fd, int mode)
{
	return epoll_ctl_wrap(EPOLL_CTL_MOD, fd, mode);
}

EpollController::EpollController() : m_epoll(epoll_create(POLL_SIZE))
{
	if (!m_epoll.is_valid())
	{
		LOGE("Cannot create epoll: %d", m_epoll.get_fd());
		throw std::runtime_error("Can't create a epoll");
	}
	LOGI("Epoll created: %d", m_epoll.get_fd());
}

std::vector<int> EpollController::poll()
{
	LOGI("Started!");
	std::vector<int> timeoutClients;
	int ready = epoll_wait(m_epoll.get_fd(), events, POLL_SIZE, TIMEOUT_WAIT);
	if (ready == -1)
	{
		if (errno != EINTR)
		{
			LOGE("Waiting error!");
		}
	}
	for (std::size_t i = 0; i < ready; ++i)
	{
		int cur_fd = events[i].data.fd;
		std::size_t cur_time = Utils::get_current_time();
		clients[cur_fd] = cur_time;
		functions[cur_fd](events[i].events);
	}

	if (ready == 0)
	{
		LOGI("Timeout wait");
	}

	for (int client : get_active_clients())
	{
		if (!is_active(clients[client]))
		{
			delete_from_poll(client);
			timeoutClients.push_back(client);
		}
	}
	return timeoutClients;
}

std::vector<int> EpollController::get_active_clients() const
{
	std::vector<int> result;
	for (auto const& [fd, time] : clients)
	{
		result.push_back(fd);
	}
	return result;
}

EpollController::~EpollController() {
	LOGI("Epoll is closing!");
	for (int client : get_active_clients())
	{
		delete_from_poll(client);
	}
}