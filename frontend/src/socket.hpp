#ifndef VICTUS_SOCKET_HPP
#define VICTUS_SOCKET_HPP

#include <string>
#include <future>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <thread>
#include <atomic>

enum ServerCommands
{
	GET_FAN_SPEED,
    SET_FAN_SPEED,
	SET_FAN_MODE,
	GET_FAN_MODE,
	SET_FAN_PROFILE,
	GET_CPU_TEMP,
	GET_ALL_TEMPS,
	GET_KEYBOARD_COLOR,
	SET_KEYBOARD_COLOR,
	GET_KBD_BRIGHTNESS,
	SET_KBD_BRIGHTNESS
};

struct PendingCommand
{
	ServerCommands type;
	std::string command;
	std::promise<std::string> result;
};

class VictusSocketClient
{
public:
	VictusSocketClient(const std::string &socket_path);
	~VictusSocketClient();

	std::future<std::string> send_command_async(ServerCommands type, const std::string &command = "");

private:
	std::string send_command(const std::string &command);
	std::string socket_path;

	bool connect_to_server();
	void close_socket();

	int sockfd;
    std::mutex socket_mutex;

	std::unordered_map<ServerCommands, std::string> command_prefix_map;

	// Request queue system (max 3 concurrent requests)
	static constexpr int MAX_CONCURRENT_REQUESTS = 3;
	std::queue<std::unique_ptr<PendingCommand>> command_queue;
	std::mutex queue_mutex;
	std::condition_variable queue_cv;
	std::thread queue_worker_thread;
	std::atomic<bool> shutdown_queue{false};
	std::atomic<int> active_requests{0};

	void queue_worker();
	void process_queued_command(std::unique_ptr<PendingCommand> pending);
};

#endif // VICTUS_SOCKET_HPP
