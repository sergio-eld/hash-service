#pragma once

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <cstdint>
#include <memory>
#include <array>
#include <string_view>
#include <optional>
#include <charconv>

namespace hs {
	class sha256_hash
	{
	 public:
		constexpr static size_t digest_length = SHA256_DIGEST_LENGTH;

		sha256_hash(const sha256_hash&) = delete;
		sha256_hash& operator=(const sha256_hash&) = delete;

		sha256_hash(sha256_hash&&) noexcept = default;
		sha256_hash& operator=(sha256_hash&&) noexcept = default;

		// TODO: expected-like error
		static std::optional<sha256_hash> create() noexcept {
			unique_md_ctx context{EVP_MD_CTX_new()};
			if (!context)
				return std::nullopt;

			if (!EVP_DigestInit(context.get(), EVP_sha256()))
				return std::nullopt;

			return sha256_hash(std::move(context));
		}

		// TODO: expected-like error
		bool update(std::string_view str) noexcept {
			return bool(_context) && EVP_DigestUpdate(_context.get(), str.data(), str.size());
		}

		auto finalize() noexcept -> std::optional<std::array<uint8_t, digest_length>> {
			if (!_context)
				return std::nullopt;

			std::array<uint8_t, digest_length> hash{};
			unsigned int written = 0;
			if (!EVP_DigestFinal(_context.get(), hash.data(), &written) || !written)
				return std::nullopt;
			if (!EVP_DigestInit(_context.get(), EVP_sha256()))
				return std::nullopt;

			return hash;
		}

	 private:
		struct evp_md_ctx_free
		{
			void operator()(EVP_MD_CTX *context) const noexcept {
				EVP_MD_CTX_free(context);
			}
		};

		using unique_md_ctx = std::unique_ptr<EVP_MD_CTX, evp_md_ctx_free>;

		sha256_hash(unique_md_ctx ctx)
			: _context(std::move(ctx))
		{}

		unique_md_ctx _context;
	};

	template <size_t N>
	constexpr static auto to_hex(const std::array<uint8_t, N> &arr) noexcept -> std::array<uint8_t, N * 2> {
		constexpr const char hexMap[] = "0123456789abcdef";
		std::array<uint8_t, N * 2> hex{};
		for (size_t iHash = 0, iHex = 0; iHash < arr.size(); ++iHash, iHex += 2)
		{
			const uint8_t ch = arr[iHash];
			hex[iHex] = hexMap[(ch & 0xF0) >> 4];
			hex[iHex + 1] = hexMap[ch & 0x0F];
		}
		return hex;
	}
}
