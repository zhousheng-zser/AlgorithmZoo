#include "CppUnitTest.h"
#include "../gaius/feature_extractor.hpp"
#include "../cassius/feature_extractor.hpp"
#include "../common/include/Primitives/tensor.hpp"
#include "../common/include/Primitives/pool_allocator.hpp"

#include <random>
#include <algorithm>
#include <filesystem>

#include <abi/consumer.hpp>

using namespace glasssix;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace unittest
{
	TEST_CLASS(cassius_gaius_test)
	{
	public:
		TEST_METHOD(cassius_extractor_test)
		{
			std::vector<std::uint8_t> input_bitmap(128 * 128 * 10);

			try
			{
				auto cassius_extractor = exposing::make_exported_interface<cassius::feature_extractor>(u8"models/unicorn.phai", -1);
				auto result = cassius_extractor.get(input_bitmap, 10, 0);

				Assert::AreEqual(10ULL, result.size());

				for (std::size_t i = 0; i < 10; i++)
				{
					Assert::AreEqual(512ULL, result[i].size());
				}
			}
			catch (const exposing::abi_error& ex)
			{
				Logger::WriteMessage(ex.what_to_narrow().c_str());
				Assert::Fail();
			}
		}

		TEST_METHOD(gaius_extractor_test)
		{
			std::vector<std::uint8_t> input_bitmap(128 * 128 * 10);

			try
			{
				auto gaius_extractor = exposing::make_exported_interface<gaius::feature_extractor>(u8"models/mobile_unicorn.phai", -1);
				auto result = gaius_extractor.get(input_bitmap, 10, 0);

				Assert::AreEqual(10ULL, result.size());

				for (std::size_t i = 0; i < 10; i++)
				{
					Assert::AreEqual(128ULL, result[i].size());
				}
			}
			catch (const exposing::abi_error& ex)
			{
				Logger::WriteMessage(ex.what_to_narrow().c_str());
				Assert::Fail();
			}
		}

		TEST_METHOD(pool_allocator_test)
		{
			memory::tensor<float> test{ { 1, 3, 100, 100 }, -1, memory::NCHW, &memory::pool_allocator_default<float>::get() };

			test.mutable_cpu_data();
			test.mutable_gpu_data();
		}
	};
}
