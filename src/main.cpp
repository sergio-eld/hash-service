
#include "hash-service/server.h"

#include <asio.hpp>

#include <iostream>

namespace {
}

int main(int argc, char **argv) {
	(void)argc;
	(void)argv;

	{
		// TODO: cli args
		uint16_t port = 23;
		(void)port;

		// TODO: (?) separate non-io task handling to asio::thread_pool
		asio::io_context ioContext{int(std::thread::hardware_concurrency())};
		hs::server hashServer{ioContext, hs::server::config{port, std::chrono::seconds(2)}};

		asio::signal_set signals{ioContext, SIGINT};
		signals.async_wait([&hashServer](asio::error_code /*errorCode*/, int sig){
		  std::cout << "handling a signal: " << sig << '\n';
		  if (sig == SIGINT)
		  {
			  std::cout << "SIGINT\n";
			  hashServer.stop();
		  }
		});

		std::cout << "Listening to port " << port << "...\n";

		ioContext.run();
	}

	std::cout << "THE END!\n";
	return 0;
}