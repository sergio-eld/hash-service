#pragma once

#include "hash-service/session.h"

#include <asio.hpp>

#include <type_traits>
#include <chrono>
#include <algorithm>

#include <vector>
#include <thread>

namespace hs {
	using asio::ip::tcp;

	namespace detail {
		template <typename Config, typename = void>
		struct _get_time_interval
		{
			constexpr std::chrono::milliseconds operator()(const Config&) const noexcept {
				return std::chrono::seconds(2);
			}
		};

		template <typename Config>
		struct _get_time_interval<Config, std::void_t<decltype(std::declval<Config>().time_interval)>>
		{
			constexpr std::chrono::milliseconds operator()(const Config& c) const noexcept {
				return c.time_interval;
			}
		};
	}

	template <typename Config>
	constexpr static std::chrono::milliseconds get_time_interval(const Config &c) noexcept {
		return detail::_get_time_interval<std::decay_t<Config>>{}(c);
	}

	/**
	 * @brief TCP hashing server.
	 *
	 * Upon instantiation begins asynchronously accepting new tcp connections and
	 * monitoring their lifetime.
	 * Stores termination handlers for accepted sessions for graceful termination
	 * when server::stop() is called.
	 *
	 * Periodically disposes of dead termination references.
	 */
	class server
	{
	 public:
		struct config
		{
			uint16_t port;
			std::chrono::milliseconds connection_timeout;
		};

		/**
		 * Constructor.
		 * Upon instantiation begins asynchronously accepting new tcp connections.
		 * @tparam Config
		 * @param executor
		 * @param config
		 */
		template <typename Config>
		constexpr server(asio::io_context &executor, Config &&config)
			: _acceptor(executor, tcp::endpoint(tcp::v4(), config.port)),
			  _connectionTimeout(config.connection_timeout),
			  _monitoringStrand(executor.get_executor()),
			  _monitoringInterval(get_time_interval(config)),
			  _monitoringTimer(executor)
		{
			if(_monitoringInterval < std::chrono::milliseconds(200))
				_monitoringInterval = std::chrono::milliseconds(200);

			// TODO: configure?
			_sessionTerminators.reserve(256);
			start_monitoring();
			accepting();
		}

		server(const server&) = delete;
		server(server &&) = delete;
		server& operator=(const server&) = delete;
		server& operator=(server&&) = delete;

		/**
		 * Stops all operations.
		 * All the outstanding tcp connections are gracefully shutdown.
		 * Removes all the termination handlers.
		 */
		void stop() {
			asio::error_code errorCode{};
			_acceptor.cancel(errorCode);

			if (!errorCode)
				asio::post(_monitoringStrand, [this]{terminate_all_sessions();});
			// TODO: (?) handle errorCode
		}

	 private:
		void accepting() noexcept {
			_acceptor.async_accept(
				[this](asio::error_code err, tcp::socket socket) mutable {
				  if (!err){
					  using config = hs::session::config;
					  auto &&term = hs::session::start(std::move(socket), config{_connectionTimeout});
					  asio::post(_monitoringStrand, [this, term = std::move(term)] () mutable {
						register_session(std::move(term));
					  });

					  accepting();
					  return;
				  }

				  if (err == asio::error::operation_aborted)
				  {
					  _monitoringTimer.cancel();
					  return;
				  }

				  // TODO: (?) something terrible has happened.
				});
		}

		void start_monitoring() {
			asio::error_code errorCode{};
			_monitoringTimer.expires_at(std::chrono::steady_clock::now() + _monitoringInterval, errorCode);
			// TODO: what if we fail with errorCode here?

			_monitoringTimer.async_wait([&](asio::error_code err){
			  if (!err) {
				  asio::post(_monitoringStrand, [this] {remove_dead_sessions(); });
				  return;
			  }

			  if (err == asio::error::operation_aborted)
			  {
				  return;
			  }

			  // TODO: (?) something terrible has happened.
			});
		}

		void remove_dead_sessions() {
			auto iRemove = std::remove_if(std::begin(_sessionTerminators), std::end(_sessionTerminators),
				[](const auto &s) noexcept -> bool {
				  return !s.is_alive();
				});
			_sessionTerminators.erase(iRemove, std::end(_sessionTerminators));
			asio::post(_monitoringStrand, [this]{start_monitoring();});
		}

		void register_session(hs::session::termination &&session) {
			_sessionTerminators.push_back(std::move(session));
		}

		void terminate_all_sessions() {
			for (auto &term : _sessionTerminators)
				term();
		}

	 private:
		tcp::acceptor _acceptor;

		std::chrono::milliseconds _connectionTimeout;

		// strand to serialize actions on adding new and removing dead sessions
		asio::strand<asio::io_service::executor_type> _monitoringStrand;
		std::chrono::milliseconds _monitoringInterval;
		asio::steady_timer _monitoringTimer;
		std::vector<hs::session::termination> _sessionTerminators;
	};
}