#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <iostream>
#include <stdlib.h>
#include "Server.hpp"
#include "Utils.hpp"

bool Server::close_socket(int fd)
{
	if (close(fd) < 0)
	{
		Utils::print_error("Coldn't close socket");
		return false;
	}
	return true;
}

bool Server::is_active(long long last_time, long long cur_time) const
{
	return cur_time - last_time < TIMEOUT_CLIENT;
}

int Server::set_non_block(int fd)
{
	int flags;
	if ((flags = fcntl(fd, F_GETFL, 0) == -1)) {
		flags = 0;
	}
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int Server::create_socket()
{
	int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (fd < 0) {
		Utils::print_error("Couldn't create socket");
		return ERROR_CODE;
	}
	int opt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
		close_socket(fd);
		Utils::print_error("Couldn't set options for socket!");
		return ERROR_CODE;
	}
	return fd;
}

bool Server::bind_socket(int fd)
{
	sockaddr_in addr{};
	addr.sin_port = htons(m_port);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
	{
		close_socket(fd);
		Utils::print_error("Couldn't bind a listen socket");
		return false;
	}
	return true;
}

bool Server::epoll_ctl_wrap(int poll, int op, int fd, unsigned events)
{
	struct epoll_event event{};
	event.data.fd = fd;
	event.events = events;
	return epoll_ctl(poll, op, fd, &event) >= 0;
}

bool Server::add_to_poll(int fd, unsigned ev)
{
	return epoll_ctl_wrap(m_epoll, EPOLL_CTL_ADD, fd, ev);
}

bool Server::delete_from_poll(int fd)
{
	return epoll_ctl_wrap(m_epoll, EPOLL_CTL_DEL, fd, 0);
}

bool Server::modify_in_pool(int fd, int mode)
{
	return epoll_ctl_wrap(m_epoll, EPOLL_CTL_MOD, fd, mode);
}

Server::Server(int port) : m_port(port)
{
	m_listener = create_socket();
	m_epoll = epoll_create(POLL_SIZE);
	bind_socket(m_listener);
	if (listen(m_listener, BACKLOG) == -1)
	{
		close_socket(m_listener);
		close_socket(m_epoll);
		Utils::print_error("Listen failed!");
	}

	if (!add_to_poll(m_listener, EPOLLIN | EPOLLET))
	{
		close_socket(m_listener);
		close_socket(m_epoll);
		Utils::print_error("Couldn't add listener to epool!");
	}

	if (!add_to_poll(0, EPOLLIN))
	{
		close_socket(m_listener);
		close_socket(m_epoll);
		Utils::print_error("Couldn't add stdin to epool!");
	}
	Utils::log("Prepared!");
}

void Server::start()
{
	Utils::log("Started!");
	while (true)
	{
		int ready = epoll_wait(m_epoll, events, POLL_SIZE, TIMEOUT_WAIT);

		if (ready == -1)
		{
			if (errno != EINTR)
			{
				Utils::print_error("Waiting error!");
			}
			break;
		}
		Utils::log("Clients: " + std::to_string(ready));
		for (std::size_t i = 0; i < ready; ++i)
		{
			if (events[i].data.fd == m_listener)
			{
				struct sockaddr_in addr{};
				socklen_t addr_len = sizeof(addr);
				int client = accept(m_listener, reinterpret_cast<struct sockaddr*>(&addr), &addr_len);

				if (client < 0)
				{
					Utils::print_error("Couldn't connect!");
					continue;
				}

				if (set_non_block(client) == -1)
				{
					Utils::print_error("Failed to set non block client!");
					close_socket(client);
					continue;
				}

				if (!add_to_poll(client, EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET))
				{
					Utils::print_error("Failed to register client!");
					close_socket(client);
				}
				else
				{
					Utils::log("Connection successfully!");
					long long cur_time = Utils::get_current_time();
					clients[client] = { cur_time };
					active_clients.push({ -cur_time, client });
				}
			}
			else if (events[i].data.fd == 0)
			{
				std::string line;
				std::getline(std::cin, line);
				if (line == "exit")
				{
					return;
				}
			}
			else
			{

				int cur_fd = events[i].data.fd;
				if (events[i].events & (EPOLLERR | EPOLLHUP))
				{
					Utils::log("Client disconnected. Fd: " + std::to_string(cur_fd));
					delete_from_poll(cur_fd);
					close_socket(cur_fd);
					clients.erase(cur_fd);
					continue;
				}

				if (events[i].events & EPOLLIN)
				{
					//Utils::log("In IN");
					int st = process_socket(cur_fd);
					if (st > 0)
					{
						long long cur_time = Utils::get_current_time();
						clients[cur_fd].set_time(cur_time);
						active_clients.push({ -cur_time, cur_fd });
						if (!modify_in_pool(cur_fd, EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLET))
						{
							Utils::print_error("Critical 1");
							return;
						}
					}
					continue;
				}

				if (events[i].events & EPOLLOUT)
				{
					if (clients.count(cur_fd) > 0)
					{
						Utils::write(cur_fd, clients[cur_fd].response);
						long long cur_time = Utils::get_current_time();
						clients[cur_fd].set_time(cur_time);
						active_clients.push({ -cur_time, cur_fd });
						if (!modify_in_pool(cur_fd, EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET))
						{
							Utils::print_error("Critical 2");
							return;
						}
					}
					continue;
				}
				//Utils::print_error("Other operation");
				//Utils::print_error(std::to_string(events[i].events));
			}
		}

		if (ready == 0)
		{
			Utils::log("Timeout wait");
		}

		// try clear clients
		while (!active_clients.empty())
		{
			auto [time, fd] = active_clients.top();
			time = -time;
			long long cur_time = Utils::get_current_time();
			if (clients.count(fd) > 0 && clients[fd].last_update == time)
			{
				if (is_active(time, cur_time))
				{
					break;
				}
				else
				{
					Utils::log("Client disconnected. Fd: " + std::to_string(fd));
					delete_from_poll(fd);
					close_socket(fd);
					clients.erase(fd);
					active_clients.pop();
				}
			}
			else
			{
				active_clients.pop();
			}
		}
	}
}

int Server::process_socket(int fd)
{
	std::string request;
	int st = Utils::read(fd, request);
	//Utils::log("Read: " + std::to_string(st));
	if (st == -1 || st == 0)
	{
		if (st == -1)
		{
			Utils::print_error("Client disconnected. Fd: " + std::to_string(fd));
		}
		else
		{
			Utils::log("Client disconnected. Fd: " + std::to_string(fd));
		}	
		delete_from_poll(fd);
		clients.erase(fd);
		close_socket(fd);
	}
	else
	{
		Utils::prepare(request);
		auto result = Utils::lookup_host(request);
		clients[fd].set_response(Utils::vector_to_str(result));
	}
	return st;
}

Server::~Server() {
	Utils::log("Bye, bye, my dear admin!!!");
	for (auto const& [fd, cl] : clients)
	{
		delete_from_poll(fd);
		close_socket(fd);
		active_clients.pop();
	}
	clients.clear();
	delete_from_poll(m_listener);
	close_socket(m_listener);
	close_socket(m_epoll);
}