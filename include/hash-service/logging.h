#pragma once

#include <string>
#include <thread>

#include <iostream>
#include <sstream>

namespace hs {
	/**
	 * Logging level flag
	 */
	enum class log_level {
		errors = 1,
		warnings = 2,
		messages = 4
	};

	log_level operator|(log_level lhs, log_level rhs) noexcept {
		return log_level(static_cast<std::underlying_type_t<log_level>>(lhs) |
						 static_cast<std::underlying_type_t<log_level>>(rhs));
	}

	log_level operator&(log_level lhs, log_level rhs) noexcept {
		return log_level(static_cast<std::underlying_type_t<log_level>>(lhs) &
						 static_cast<std::underlying_type_t<log_level>>(rhs));
	}

	/**
	 * ad-hoc logger using std::cout and std::cerr.
	 * Need to be replaced later
	 */
	class std_ostream_logger
	{
	 public:
		explicit std_ostream_logger(log_level level)
			: _level(level)
		{}

		std_ostream_logger()
			: std_ostream_logger(log_level::errors | log_level::warnings | log_level::messages)
		{}

		void message(std::string s) const noexcept {
			std::cout << "s";
			std::cout << "level: " << int(_level) << '\n';
			if (!is_enabled(log_level::messages))
				return;

			std::stringstream ss;
			ss << "[thread:" << std::this_thread::get_id() << "] " << s << '\n';
			std::cout << ss.str();
		}

		void warning(std::string s) const noexcept {
			if (!is_enabled(log_level::warnings))
				return;

			std::stringstream ss;
			ss << "WARNING [thread:" << std::this_thread::get_id() << "] " << s << '\n';
			std::cout << ss.str();
		}

		void error(std::string s) const noexcept {
			if (!is_enabled(log_level::errors))
				return;

			std::stringstream ss;
			ss << "ERROR [thread:" << std::this_thread::get_id() << "] " << s << '\n';
			std::cerr << ss.str();
		}

	 private:
		[[nodiscard]] bool is_enabled(log_level flag) const noexcept {
			return bool(_level & flag);
		}

		log_level _level;
	};

}
