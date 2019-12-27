
#include <sys/epoll.h>
#include <stdint.h>
#include <cstddef>
#include <unistd.h>
#include <unordered_map>
#include <queue>

class Server {
public:

	struct ClientS {
		void set_time(long long time) { last_update = time; }
		void set_response(std::string_view const& sv) { response.assign(sv); };
		long long last_update { 0 };
		std::string response { "" };
	};

	explicit Server(int port);
	~Server();
	void start();

private:
	bool epoll_ctl_wrap(int poll, int op, int fd, unsigned events);
	bool add_to_poll(int fd, unsigned mode);
	bool delete_from_poll(int fd);
	bool modify_in_pool(int fd, int mode);

	int create_socket();
	int set_non_block(int fd);
	bool bind_socket(int fd);
	int process_socket(int fd);
	bool close_socket(int fd);

	bool is_active(long long last_time, long long cur_time) const;
	
private:

	enum {
		POLL_SIZE = 4096,
		BACKLOG = 4096,
		TIMEOUT_WAIT = 3000,
		TIMEOUT_CLIENT = 120,
		ERROR_CODE = -1
	};
	
	struct epoll_event event;
	struct epoll_event events[POLL_SIZE];
	std::unordered_map<int, ClientS> clients;
	std::priority_queue<std::pair<long long, int>> active_clients;

	int m_port;
	int m_listener;
	int m_epoll;
};