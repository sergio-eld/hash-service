
#include "hash-service/server.h"
#include "hash-service/logging.h"

#include <asio.hpp>

#include <thread>
#include <string>

int main(int argc, char **argv) {
	try {
		const uint16_t port = argc < 2 ? 23 :
			std::stoi(argv[1]);

		// TODO: (?) separate non-io task handling to asio::thread_pool
		asio::io_context ioContext{int(std::thread::hardware_concurrency())};
		hs::server hashServer{ioContext, hs::server::config{port,
															std::chrono::seconds(10),

															// TODO: log level from CLI
															hs::std_ostream_logger()}};

		asio::signal_set signals{ioContext, SIGINT};
		signals.async_wait([&hashServer, &ioContext](asio::error_code /*errorCode*/, int sig){
			std::stringstream ss{};
			ss << "[thread:" << std::this_thread::get_id() << "] handling a signal: " << sig << '\n';

			std::cout << ss.str();
			if (sig == SIGINT)
			{
				std::cout << "SIGINT\n";
				asio::post(ioContext, [&hashServer]{hashServer.stop();});
			}
		});

		ioContext.run();
	}
	catch (const std::invalid_argument &)
	{
		std::cerr << "invalid port.\nsignature: server [port = 23]\n";
		return 0;
	}
	catch (const std::exception &e)
	{
		std::cerr << "unexpected error: " << e.what() << '\n';
		return 0;
	}

	std::cout << "THE END!\n";
	return 0;
}