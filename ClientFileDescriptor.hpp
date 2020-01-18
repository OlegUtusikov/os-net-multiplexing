#pragma once
#include <string>
#include "FileDescriptor.hpp"

class ClientFileDescriptor : public FileDescriptor
{
public:
	ClientFileDescriptor(int fd);
	~ClientFileDescriptor();
	void read();
	bool has_response() const;
	void write();
	uint32_t get_events() const { return flags; };
private:
	void parse_read_buffer();
private:
	enum{
		BUFFER_SIZE = 1024 * 1024
	};
	uint32_t flags;
	char buffer[BUFFER_SIZE];
	std::string read_buffer;
	std::string write_buffer;
};