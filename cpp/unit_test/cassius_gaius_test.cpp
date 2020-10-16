#include "CppUnitTest.h"
#include "../gaius/feature_extractor.hpp"
#include "../cassius/feature_extractor.hpp"
#include "../common/include/Primitives/tensor.hpp"
#include "../common/include/Primitives/pool_allocator.hpp"

#include <random>
#include <algorithm>
#include <filesystem>
#include <thread>

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
				auto cassius_extractor = exposing::make_exported_interface<cassius::feature_extractor>(u8"models/unicorn.phai", u8"models/unicorn.racy", -1);
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
				auto gaius_extractor = exposing::make_exported_interface<gaius::feature_extractor>(u8"models/mobile_unicorn.phai", u8"models/mobile_unicorn.racy", u8"models/mobile_unicorn.phai", u8"models/mobile_unicorn.racy", -1);
				auto result = gaius_extractor.get(input_bitmap, 10, 0, false);

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

		TEST_METHOD(multithread_test)
		{
			std::vector<std::uint8_t> input_bitmap(128 * 128 * 3);
			auto cassius_extractor1 = exposing::make_exported_interface<cassius::feature_extractor>(u8"models/unicorn.phai", u8"models/unicorn.racy", -1);
			auto cassius_extractor2 = exposing::make_exported_interface<cassius::feature_extractor>(u8"models/unicorn.phai", u8"models/unicorn.racy", -1);
			auto cassius_extractor3 = exposing::make_exported_interface<cassius::feature_extractor>(u8"models/unicorn.phai", u8"models/unicorn.racy", -1);
			auto cassius_extractor4 = exposing::make_exported_interface<cassius::feature_extractor>(u8"models/unicorn.phai", u8"models/unicorn.racy", -1);

			std::thread t1([&]() {
				for (size_t i = 0; i < 100000; i++)
				{
					cassius_extractor1.get(input_bitmap, 1, 0);
				}
			});

			std::thread t2([&]() {
				for (size_t i = 0; i < 100000; i++)
				{
					cassius_extractor2.get(input_bitmap, 1, 0);
				}
			});

			std::thread t3([&]() {
				for (size_t i = 0; i < 100000; i++)
				{
					cassius_extractor3.get(input_bitmap, 1, 0);
				}
			});

			std::thread t4([&]() {
				for (size_t i = 0; i < 100000; i++)
				{
					cassius_extractor4.get(input_bitmap, 1, 0);
				}
			});


			t4.join();
			t3.join();
			t2.join();
			t1.join();
		}
	};
}
