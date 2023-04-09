#pragma once

#include "hash-service/hash.h"

#include <asio.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <chrono>

#include <array>

namespace hs {

	using tcp_socket = asio::ip::tcp::socket;

	/**
	 * Session class.
	 *
	 * Implements asynchronous state-machine that receives '\n'-terminated
	 * lines of ASCII characters and responds with an '\n'-terminated line containing
	 * the calculated hash (sha256) in a hex format.
	 */
	class session
	{
	 public:

		/**
		 * Default configuration type.
		 */
		struct config
		{
			std::chrono::microseconds timeout;
		};

		class termination;

		/**
		 * @brief Asynchronously start a new session.
		 *
		 * If the socket is not connected, will not start the session.
		 * If an internal error has occurred, will not start the session, terminating the connection.
		 *
		 * @tparam Config configuration type. Must have the same traits as `session::config`.
		 * @param socket incoming tcp connection
		 * @param conf config with parameters
		 * @return Termination object to be used for graceful termination if needed.
		 */
		template <typename Config>
		static termination start(tcp_socket &&socket, Config &&conf) noexcept;

	 private:
		session() = default;

		struct context;

		/**
		 * Receiving state.
		 * Asynchronously receives data, that potentially containing a '\n' terminated line.
		 * Transitions to Encoding upon receiving.
		 *
		 * The session will be terminated in cases, if:
		 * - a timeout has occurred
		 * - the client has ended the connection
		 * - operation has been cancelled
		 * - an internal error has occurred
		 * @param ctx
		 */
		static void receiving(std::shared_ptr<context> ctx) noexcept;

		/**
		 * Encoding state.
		 * Asynchronously encodes bytes from the buffer. If the line is complete, transitions to Responding.
		 * If no complete lines are available in the buffer, transitions to Receiving.
		 *
		 * The session will be terminated in cases, if:
		 * - an internal error has occurred
		 * @param ctx
		 */
		static void encoding(std::shared_ptr<context> ctx) noexcept;

		/**
		 * Responding state.
		 * Asynchronously sends the hashed hex '\n'-terminated line.
		 * If the buffer contains pending bytes, transitions to Encoding. Otherwise transitions to receiving.
		 *
		 * The session will be terminated in cases, if:
		 * - a timeout has occurred
		 * - the client has ended the connection
		 * - operation has been cancelled
		 * - an internal error has occurred
		 * @param ctx
		 */
		static void responding(std::shared_ptr<context> ctx) noexcept;
	};

	/**
	 * Private session context. Not a part of the public API.
	 * Session owns the context exclusively.
	 *
	 * Context's lifetime starts upon receiving a tcp connection and ends upon disconnection.
	 *
	 * An object of the context may be weak-referenced in order to track the lifetime.
	 */
	struct session::context : private std::enable_shared_from_this<context>
	{
		constexpr static size_t buffer_size = 2048;

		tcp_socket socket;

		// TODO: wrapper for the string buffer
		std::array<uint8_t, buffer_size> stringBuffer{};
		size_t pendingBytes = 0;

		constexpr static size_t hex_buffer_sz = sha256_hash::digest_length * 2 + 1;
		std::array<uint8_t, hex_buffer_sz> hexBuffer{};
		sha256_hash hash;

		std::weak_ptr<context> weak_ref() {
			return weak_from_this();
		}

		template <typename Config>
		[[nodiscard]] static std::shared_ptr<context> create(tcp_socket &&socket,
																sha256_hash &&hash,
																Config &&conf) noexcept {
			return std::shared_ptr<context>(new context(std::move(socket), std::move(hash), std::forward<Config>(conf)));
		}

	 private:
		template <typename Config>
		context(tcp_socket &&socket, sha256_hash &&hash, Config &&/*conf*/)
			: socket(std::move(socket)),
			hash(std::move(hash))
		{}
	};

	/**
	 * Session termination handler.
	 * Received upon session start, can be used to observe the session's lifetime,
	 * can be invoked to terminate the session.
	 *
	 * Primarily used for graceful shutdown.
	 */
	class session::termination
	{
	 public:
		explicit termination(std::weak_ptr<session::context> &&context) noexcept
			: _context(std::move(context))
		{}

		/**
		 * @threadsafe Safe to be called from multiple threads.
		 * @return `true` if session is still alive, `false` otherwise.
		 */
		[[nodiscard]] bool is_alive() const noexcept{
			return !_context.expired();
		}

		/**
		 * @brief Terminates the session. No-op if the session has been destroyed.
		 * @threadsafe Safe to be called from multiple threads
		 */
		void operator()() noexcept {
			auto ctx = _context.lock();
			if (!ctx)
				return;

			// TODO: is this enough?
			ctx->socket.cancel();
		}

	 private:
		std::weak_ptr<session::context> _context;
	};

	void session::receiving(std::shared_ptr<context> /*ctx*/) noexcept
	{
		// TODO: implement
	}

	void session::encoding(std::shared_ptr<context> /*ctx*/) noexcept
	{
		// TODO: implement
	}

	void session::responding(std::shared_ptr<context> /*ctx*/) noexcept
	{
		// TODO: implement
	}

	template<typename Config>
	session::termination session::start(tcp_socket&& socket, Config&& conf) noexcept
	{
		auto optHash = sha256_hash::create();
		if (!optHash)
			return termination({});

		auto ctx = context::create(std::move(socket), std::move(*optHash), std::forward<Config>(conf));
		session::receiving(ctx);

		return session::termination(ctx->weak_ref());
	}
}
