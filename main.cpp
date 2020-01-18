#include "Server.hpp"
#include <csignal>
#include <iostream>
#include <string.h>
#include <memory>
#include <functional>

int main(int argc, char** argv)
{
	int port = 33333;
	if (argc > 2)
	{
		std::cerr << "A lot of arguments" << std::endl;
	}
	else if (argc == 2)
	{
		if (std::isdigit(argv[1][0]))
		{
			port = atoi(argv[1]);
		}
	}
	
	Server server(port);
	server.start();
	
	return 0;
}