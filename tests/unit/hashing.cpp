
#include "hash-service/hash.h"

#include <gtest/gtest.h>

#include <random>
#include <vector>
#include <string>
#include <string_view>
#include <thread>

namespace {
	struct test_input {
		std::string line;
		std::string expected;
	};

	std::string get_lorem();
	std::string lorem_hex();

	const auto oceanic = test_input{
		"oceanic 815",
		"ae6a9df8bdf4545392e6b1354252af8546282b49033a9118b12e9511892197c6"
	};

	const auto lorem = test_input{
		get_lorem(),
		lorem_hex()
	};

	/**
	 * Testing hashing and hexing for a single line using single invocation
	 * @param testInput
	 */
	void test_case(const test_input &testInput);

	/**
	 * Testing hashing and hexing for a single line using several invocations,
	 * splitting to random-sized chunks.
	 * @param testInput
	 */
	void test_case_chunks(const test_input &testInput);

	/**
	 * Hashing several inputs using a single hash object.
	 * @tparam Inputs
	 * @param hash
	 */
	template <typename ... Inputs>
	void test_case(hs::sha256_hash &hash, const Inputs &...);

	TEST(Hashing, Line) {
		ASSERT_NO_FATAL_FAILURE(test_case(oceanic));
	}

	TEST(Hashing, LoremIpsum) {
		ASSERT_NO_FATAL_FAILURE(test_case(lorem));
	}

	TEST(Hashing, ReusingHashForSeveralLines){
		auto optHash = hs::sha256_hash::create();
		ASSERT_TRUE(optHash);
		ASSERT_NO_FATAL_FAILURE(test_case(*optHash, oceanic, lorem));
	}

	TEST(Hashing, LoremIpsumInChunks){
		ASSERT_NO_FATAL_FAILURE(test_case_chunks(lorem));
	}

	// not sure if gtest supports such cases
	TEST(Hashing, Multithreaded) {
		const size_t threadsCount = std::thread::hardware_concurrency();
		std::vector<std::thread> threads{};
		threads.reserve(threadsCount);
		for (size_t i = 0; i < threadsCount; ++i)
			threads.emplace_back([lorem = lorem]{
			  ASSERT_NO_FATAL_FAILURE(test_case(lorem));
			  ASSERT_NO_FATAL_FAILURE(test_case_chunks(lorem));
			});

		for (auto &t : threads)
			t.join();
	}
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

namespace {
	void test_case(const test_input &testInput){
		auto optHash = hs::sha256_hash::create();
		ASSERT_TRUE(optHash);
		ASSERT_TRUE(optHash->update(testInput.line));
		auto optRes = optHash->finalize();
		ASSERT_TRUE(optRes);
		const auto hexLine = hs::to_hex(*optRes);
		ASSERT_EQ(std::string_view((const char*)hexLine.data(), hexLine.size()), testInput.expected);
	}

	void test_case_chunks(const test_input &testInput){
		auto optHash = hs::sha256_hash::create();
		ASSERT_TRUE(optHash);

		std::random_device dev;
		std::mt19937_64 rng(dev());
		std::uniform_int_distribution<decltype(rng)::result_type> distribution{1, 256};
		const auto increment = [&distribution,
						   &rng,
						   iEnd = testInput.line.cend()](auto iter) noexcept {
			const size_t toAdvance = std::min<size_t>(std::distance(iter, iEnd), distribution(rng));
			return std::next(iter, toAdvance);
		};

		for (auto iBegin = testInput.line.cbegin(), iEnd = increment(iBegin);
			 iBegin < testInput.line.cend();
			 iEnd = increment(iEnd))
		{
			ASSERT_TRUE(optHash->update(std::string_view(&*iBegin, std::distance(iBegin, iEnd))));
			iBegin = iEnd;
		}

		auto optRes = optHash->finalize();
		ASSERT_TRUE(optRes);
		const auto hexLine = hs::to_hex(*optRes);
		ASSERT_EQ(std::string_view((const char*)hexLine.data(), hexLine.size()), testInput.expected);
	}

	template<typename... Inputs>
	void test_case(hs::sha256_hash& hash, const Inputs& ... inputs)
	{
		const auto testLine = [&hash](const test_input &input) {
		  ASSERT_TRUE(hash.update(input.line));
		  auto optRes = hash.finalize();
		  ASSERT_TRUE(optRes);
		  const auto hexLine = hs::to_hex(*optRes);
		  ASSERT_EQ(std::string_view((const char*)hexLine.data(), hexLine.size()), input.expected);
		};
		(testLine(inputs), ...);
	}

	std::string get_lorem() {
		return R"(Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer volutpat commodo urna, a scelerisque enim bibendum vitae. Curabitur semper lobortis dolor, at mattis ex luctus et. Aenean odio libero, finibus nec nisi commodo, dictum porta sapien. Fusce vel lectus eu augue vulputate hendrerit sit amet vitae arcu. Nam a lectus nec augue dapibus feugiat. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Duis ut leo vulputate mi pellentesque blandit nec at ante. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Praesent fermentum ultrices ullamcorper.

Nulla fermentum posuere turpis, elementum rutrum arcu lacinia vitae. Phasellus vulputate quis nisi in sodales. Nulla facilisi. Donec turpis sapien, lacinia id nisl vel, accumsan ullamcorper nisi. Fusce placerat eu urna quis pellentesque. Suspendisse pellentesque, ipsum vitae viverra fermentum, arcu justo feugiat tortor, et interdum ex felis non magna. Ut ut est vel lectus varius mattis sit amet vel elit. Donec ac felis ac risus rhoncus vestibulum. Vivamus dapibus imperdiet magna, vehicula porttitor dui semper eget. Suspendisse et ipsum est. Mauris nec nisi elit.

Etiam eros purus, volutpat ut mi a, consequat eleifend libero. Fusce eu tempus purus. Interdum et malesuada fames ac ante ipsum primis in faucibus. Duis sed justo neque. Maecenas pharetra leo nibh, eu tempor justo facilisis ut. Sed finibus, velit in rhoncus pharetra, dui ipsum mattis ligula, elementum interdum diam est at leo. Fusce faucibus enim ipsum, et vehicula tortor ornare sit amet.

Aliquam laoreet nisi at est scelerisque tincidunt. Phasellus lobortis sem tempus lobortis iaculis. Donec viverra mauris tincidunt, facilisis erat id, euismod leo. Praesent urna arcu, pulvinar et auctor ac, sagittis non nisi. Aenean facilisis facilisis accumsan. Praesent feugiat dignissim tempus. Fusce condimentum dictum mauris, sed auctor velit laoreet ac. Donec ultricies odio fringilla tellus semper, ac ornare ante blandit. Suspendisse laoreet laoreet pulvinar.

Nunc maximus metus nec scelerisque accumsan. Pellentesque vitae nibh sed odio venenatis maximus. Nulla ut sem ac lectus ultrices mollis sit amet vel erat. Ut et sem a sem vestibulum scelerisque eu sit amet turpis. Duis vulputate mollis diam a eleifend. Curabitur imperdiet nunc vel urna tincidunt, quis vehicula tortor euismod. Maecenas et sem sit amet urna dictum condimentum at at diam. Suspendisse nisl arcu, rhoncus in feugiat ac, pulvinar sed urna. Maecenas at leo ac est posuere pharetra sit amet eget felis. Nunc mauris augue, auctor vel dignissim a, lobortis ac purus. Cras quis bibendum magna. Vestibulum dictum tortor id velit efficitur, sed ultricies risus laoreet. In vel neque eleifend, tristique libero quis, congue ante. Nullam eget tempus lacus. Aenean ultrices felis euismod dui porttitor finibus. Praesent suscipit volutpat felis, et faucibus mi malesuada ac.)";
	}

	std::string lorem_hex(){
		return "fa472b1346fcc923b3e3a158884990d0e67d9e123aff8d18aafde254d26b30dc";
	}
}
