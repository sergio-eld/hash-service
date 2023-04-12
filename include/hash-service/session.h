#pragma once

#include "hash-service/hash.h"
#include "hash-service/logging.h"

#include <asio.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <chrono>

#include <array>

namespace hs {
	namespace detail {
		template <typename T, typename ... Ts, size_t ... I>
		constexpr static auto append(std::index_sequence<I...>, std::array<T, sizeof...(I)> to, Ts... vals) {
			constexpr size_t N = sizeof...(I) + sizeof...(Ts);
			return std::array<T, N>{to[I]..., T(vals)...};
		}

		template <size_t N, typename T, typename ... Ts>
		constexpr static auto append(std::array<T, N> to, Ts ... vals) noexcept {
			return append(std::make_index_sequence<N>(), to, vals...);
		}
	}

	using tcp = asio::ip::tcp;

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
			std_ostream_logger logger;
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
		static termination start(tcp::socket &&socket, Config &&conf) noexcept;

	 private:
		session() = default;

		struct context;

		/**
		 * @brief Receiving state.
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
	struct session::context : std::enable_shared_from_this<context>
	{
		constexpr static size_t buffer_size = 2048;

		tcp::socket socket;
		asio::strand<tcp::socket::executor_type> socketStrand;

		// TODO: wrapper for the string buffer
		std::array<uint8_t, buffer_size> stringBuffer{};
		size_t pendingBytes = 0;

		constexpr static size_t hex_buffer_sz = sha256_hash::digest_length * 2 + 1;
		std::array<uint8_t, hex_buffer_sz> hexBuffer{};
		sha256_hash hash;
		std_ostream_logger logger;

		std::weak_ptr<context> weak_ref() {
			return weak_from_this();
		}

		template <typename Config>
		[[nodiscard]] static std::shared_ptr<context> create(tcp::socket &&socket,
																sha256_hash &&hash,
																Config &&conf) noexcept {
			return std::shared_ptr<context>(new context(std::move(socket), std::move(hash), std::forward<Config>(conf)));
		}

	 private:
		template <typename Config>
		context(tcp::socket &&socket, sha256_hash &&hash, Config &&conf)
			: socket(std::move(socket)),
			socketStrand(socket.get_executor()),
			hash(std::move(hash)),
			logger(conf.logger)
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

			asio::post(ctx->socketStrand, [ctx]{
			  ctx->socket.cancel();
			  asio::error_code errorCode{};
			  ctx->socket.shutdown(asio::socket_base::shutdown_both, errorCode);
			  if (errorCode != asio::error_code())
				  ctx->logger.error(std::string("session::termination() error: ") + errorCode.message());
			});
		}

	 private:
		std::weak_ptr<session::context> _context;
	};

	void session::receiving(std::shared_ptr<context> ctx) noexcept
	{
		const auto func_name = std::string("session::") + __func__;

		tcp::socket &socket = ctx->socket;
		auto &buffer = ctx->stringBuffer;

		socket.async_receive(asio::buffer(buffer),
			[ctx, func_name](asio::error_code err, size_t bytesReceived) {
			if (!err)
			{
				ctx->pendingBytes = bytesReceived;
				session::encoding(ctx);
				return;
			}

			if (err == asio::error::operation_aborted)
			{
				ctx->logger.message(func_name + " cancelled");
				return;
			}

			if (err == asio::error::eof)
			{
				ctx->logger.message(func_name + ": tcp socket has disconnected");
				return;
			}

			ctx->logger.error(func_name + " error:" + err.message());
			// terminating the session
		});
	}

	void session::encoding(std::shared_ptr<context> ctx) noexcept
	{
		const auto func_name = std::string("session::") + __func__;

		// TODO: make a buffer-class
		const auto inspectBuffer = [](const auto &buffer, size_t bytes, char term) {
		  struct _res
		  {
			  size_t dataLength, // will not include '\n'
			  toErase; // will include '\n' if found
		  };

		  const auto iBegin = std::cbegin(buffer),
			  iEnd = std::next(iBegin, bytes);

		  const auto iTerm = std::find(iBegin, iEnd, term);
		  const size_t dataLength = std::distance(iBegin, iTerm);
		  const size_t toErase = dataLength +
								 (iTerm != std::cend(buffer) && *iTerm == term ? 1 : 0);

		  return _res{dataLength, toErase};
		};

		const auto shiftLeft = [](auto &buffer, size_t bytes) {
		  const auto iBegin = std::begin(buffer);
		  const auto iNewEnd = std::rotate(iBegin, std::next(iBegin, bytes), std::end(buffer));
		  std::for_each(iNewEnd, std::end(buffer), [](auto &e) { e = 0; });
		};

		const size_t pendingBytes = ctx->pendingBytes;
		if (!pendingBytes)
		{
			asio::post(ctx->socketStrand, [ctx]{ receiving(ctx); });
			return;
		}

		auto &buffer = ctx->stringBuffer;
		const auto [dataLength, toErase] = inspectBuffer(buffer, pendingBytes, '\n');
		std::string_view lineChunk{(const char*)buffer.data(), dataLength};
		if (!ctx->hash.update(lineChunk))
		{
			ctx->logger.error(func_name + " error: hash.update() failed");
			return;
		}

		shiftLeft(buffer, toErase);
		ctx->pendingBytes -= toErase;

		// happens only if '\n' has been found upon inspection
		const bool lineComplete = (toErase - dataLength) == 1;
		if (lineComplete)
		{
			asio::post(ctx->socketStrand, [ctx]{ responding(ctx); });
			return;
		}

		asio::post(ctx->socketStrand, [ctx]{ receiving(ctx); });
	}

	void session::responding(std::shared_ptr<context> ctx) noexcept
	{
		const auto func_name = std::string("session::") + __func__;

		const auto res = ctx->hash.finalize();
		if (!res)
		{
			ctx->logger.error(func_name + " error: hash.finalize() failed");
			return;
		}

		ctx->hexBuffer = detail::append(to_hex(*res), '\n');
		tcp::socket &socket = ctx->socket;
		asio::async_write(socket, asio::buffer(ctx->hexBuffer),
			[ctx, func_name](asio::error_code err, size_t /*bytesTransferred*/) noexcept{

			if (!err)
			{
				ctx->pendingBytes ?
					session::encoding(ctx) :
					asio::post(ctx->socketStrand, [ctx]{ receiving(ctx); });
				return;
			}

			if (err == asio::error::operation_aborted)
			{
				ctx->logger.message(func_name + " cancelled");
				return;
			}
			if (err == asio::error::eof)
			{
				ctx->logger.message(func_name + ": tcp socket has disconnected");
				return;
			}

			ctx->logger.error(func_name + " error:" + err.message());
			// terminating session here
		});
	}

	template<typename Config>
	session::termination session::start(tcp::socket&& socket, Config&& conf) noexcept
	{
		auto optHash = sha256_hash::create();
		if (!optHash)
			return termination({});

		auto ctx = context::create(std::move(socket), std::move(*optHash), std::forward<Config>(conf));
		asio::post(ctx->socketStrand, [ctx]{session::receiving(ctx);});

		return session::termination(ctx);
	}
}
