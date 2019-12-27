#include "Server.hpp"
#include <csignal>
#include <iostream>
#include <string.h>
#include <memory>

void handler(int code, siginfo_t *siginfo, void *context_of_program)
{
	if (siginfo->si_signo != SIGINT) {
		std::cout << "Must be SIGINT, but error" << std::endl;
	}
	std::cout << "Interupt server by SIGINT!" << std::endl;
}

int main(int argc, char** argv)
{
	struct sigaction action {};
	action.sa_sigaction = handler;
	action.sa_flags = SA_SIGINFO;
	if (sigaction(SIGINT, &action, nullptr) == -1)
	{
		std::cerr << "Can't change a SIGINT handler" << std::endl;
		exit(EXIT_FAILURE);
	}
	int port = 33333;
	std::cout << "Set port: ";
	std::cin >> port;
	std::unique_ptr<Server> server { new Server(port) };
	server->start();
	return 0;
}