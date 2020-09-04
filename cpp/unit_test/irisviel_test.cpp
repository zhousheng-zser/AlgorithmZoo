#include "CppUnitTest.h"
#include "../irisviel/face_service.hpp"

#include <random>
#include <algorithm>
#include <filesystem>

#include <abi/consumer.hpp>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using namespace glasssix;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace unittest
{
	namespace
	{
		std::vector<float> get_feature(std::size_t size)
		{
			thread_local std::uniform_real_distribution<float> dist{ 0.f, 1.f };
			thread_local std::default_random_engine engine{ std::random_device{}() };

			std::vector<float> result;

			for (std::size_t i = 0; i < size; i++)
			{
				result.emplace_back(dist(engine));
			}

			return result;
		}
	}

	TEST_CLASS(irisviel_test)
	{
	public:
		irisviel_test() : service_{ exposing::make_exported_interface<irisviel::face_service>(100, 512, u8".") }
		{
		}

		TEST_METHOD(single_record_test)
		{
			service_.remove_all();

			auto single_record = exposing::make_exported_interface<irisviel::record>(512);

			single_record.key(u8"123");
			single_record.feature(std::array<float, 3>{ 1, 2, 3 });

			Assert::AreEqual("123", exposing::to_narrow_string(single_record.key()).c_str());
			Assert::AreEqual(1.f, single_record.feature()[0]);
			Assert::AreEqual(2.f, single_record.feature()[1]);
			Assert::AreEqual(3.f, single_record.feature()[2]);

			service_.add_record(single_record);

			auto result = service_.search(std::array<float, 512>{ 1, 2, 3 }, 10);

			Assert::AreEqual(1ULL, result.size());
			Assert::AreEqual("123", exposing::to_narrow_string(result[0].key()).c_str());
			Assert::IsTrue(result[0].similarity() > 0.98f);
		}

		TEST_METHOD(multiple_records_test)
		{
			service_.remove_all();

			auto records = exposing::make_param_vector<irisviel::record>();

			for (std::size_t i = 0; i < 1000; i++)
			{
				auto record = exposing::make_exported_interface<irisviel::record>(512);
				
				record.key(exposing::to_param_string(i));
				record.feature(get_feature(512));

				records.push_back(record);
			}

			service_.add_records(records);
			Assert::AreEqual(1000ULL, service_.search(get_feature(512), 2000).size());
		}

		TEST_METHOD(update_record_test)
		{
			single_record_test();

			auto record = exposing::make_exported_interface<irisviel::record>(512);

			record.key(u8"123");
			record.feature(std::array<float, 5>{ 100, 200, 300, 400, 500 });

			service_.update_record(record);
			
			auto result = service_.search(std::array<float, 512>{ 100, 200, 300, 400, 500 }, 1);

			Assert::AreEqual(1ULL, result.size());
			Assert::IsTrue(result[0].similarity() > 0.98f);

			auto feature = result[0].feature();

			Assert::AreEqual(feature[0], 100.f);
			Assert::AreEqual(feature[1], 200.f);
			Assert::AreEqual(feature[2], 300.f);
			Assert::AreEqual(feature[3], 400.f);
			Assert::AreEqual(feature[4], 500.f);
		}
	private:
		irisviel::face_service service_;
	};
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		std::string buffer(32768, '\0');

		// Sets the working directory.
		buffer.resize(GetModuleFileNameA(hinstDLL, buffer.data(), buffer.size()));
		std::filesystem::current_path(std::filesystem::path{ buffer }.parent_path());

		// Adds neccessary modules.
		exposing::get_component_loader().add_modules(std::vector<exposing::param_string>{ u8"libirisviel.dll", u8"liblonginus.dll", u8"libromancia.dll", u8"libgaius.dll", u8"libcassius.dll" });
		break;
	}
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}
