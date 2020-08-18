#include "CppUnitTest.h"
#include "../plugin_demo/plugin_demo.hpp"

#include <abi/consumer.hpp>

#include <cmath>
#include <random>
#include <algorithm>

using namespace glasssix;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace unittest
{
	TEST_CLASS(unittest)
	{
	public:
		TEST_METHOD(test_plugin_demo)
		{
			// 例1：创建一个空向量，并向其中添加元素。
			{
				auto data = exposing::make_param_vector<int>();

				for (std::size_t i = 0; i < 10; i++)
				{
					data.push_back(static_cast<int>(i));
				}

				Assert::AreEqual(10ULL, data.size());
				Assert::AreEqual(5, data[5]);
			}

			// 例2：创建一个向量，并提供初始化列表。
			{
				auto data = exposing::make_param_vector<int>(1, 2, 3, 4, 5);

				Assert::AreEqual(5ULL, data.size());
				Assert::AreEqual(3, data[2]);
			}

			// 例3：创建一个哈希映射，并向其中添加元素。
			{
				auto data = exposing::make_param_hash_map<int, int>();

				data.add_or_update(1, 100);
				data.add_or_update(2, 200);
				data.add_or_update(3, 300);

				Assert::AreEqual(3ULL, data.size());
				Assert::AreEqual(200, data.get_value(2));
			}

			// 例4：创建一个哈希映射，并提供初始化列表。
			{
				auto data = exposing::make_param_hash_map<int, int>(
					{
						{ 1, 100 },
						{ 2, 200 },
						{ 3, 300 },
						{ 4, 400 },
						{ 5, 500 }
					});

				Assert::AreEqual(5ULL, data.size());
				Assert::AreEqual(300, data.get_value(3));
			}

			// 例5：从 STL 容器和指针创建映射类型。
			{
				std::vector<int> data1{ 1, 2, 3, 4, 5 };
				exposing::param_span<int> span1{ data1 };

				Assert::AreEqual(5ULL, span1.size());
				Assert::AreEqual(3, span1.data()[2]);

				std::array data2{ 1, 2, 3, 4, 5 };
				exposing::param_span<int> span2{ data2 };

				Assert::AreEqual(5ULL, span2.size());
				Assert::AreEqual(3, span2.data()[2]);

				int data3[] = { 1, 2, 3, 4, 5 };
				exposing::param_span<int> span3{ data3, sizeof(data3) / sizeof(int) };

				Assert::AreEqual(5ULL, span3.size());
				Assert::AreEqual(3, span3.data()[2]);
			}

			// 例6：利用映射创建向量，提高性能。
			{
				std::vector<int> native_data{ 1, 2, 3, 4, 5 };
				auto data = exposing::make_param_vector(exposing::param_span<int>{ native_data });

				Assert::AreEqual(5ULL, data.size());
				Assert::AreEqual(3, data[2]);
			}

			// 例7：基本字符串操作。
			{
				// 字符串一律采用 UTF-8 编码，对于字符串字面值应采用 u8 前缀。
				exposing::param_string str1{ u8"Hello World" };
				exposing::param_string str2{ u8"Bjarne Stroustrup" };
				exposing::param_string str3 = str1 + u8" " + str2;

				Assert::AreEqual("Hello World Bjarne Stroustrup", exposing::to_narrow_string(str3).c_str());

				std::string narrow_str4 = "我是一个由平台相关窄字符串转换而来。";
				auto str4 = exposing::to_param_string(narrow_str4);

				Assert::AreEqual(narrow_str4.c_str(), exposing::to_narrow_string(str4).c_str());

				auto str5 = exposing::to_param_string(123456);
				auto str6 = exposing::to_param_string(3.1416);
				auto str7 = exposing::to_param_string(1.414f);
				auto str8 = exposing::to_param_string(1024ULL);
				
				Assert::AreEqual(std::to_string(123456), exposing::to_narrow_string(str5));
				Assert::AreEqual(std::to_string(3.1416), exposing::to_narrow_string(str6));
				Assert::AreEqual(std::to_string(1.414f), exposing::to_narrow_string(str7));
				Assert::AreEqual(std::to_string(1024ULL), exposing::to_narrow_string(str8));
			}

			// 例8：字符串格式化操作。
			{
				for (std::size_t i = 0; i < 10; i++)
				{
					auto str = exposing::format(u8"我的编号是 {}。", i);

					Logger::WriteMessage(exposing::to_narrow_string(str).c_str());
				}

				std::default_random_engine random_engine{ std::random_device{}() };
				std::uniform_int_distribution<std::size_t> student_id_dist{ 10000, 9999999999 };
				std::uniform_int_distribution<std::size_t> comment_dist{ 0, 4 };

				constexpr std::array comments
				{ 
					u8"该同学思想很有高度，值得嘉奖。",
					u8"该同学进步很大，再接再厉。",
					u8"该同学有所退步，请自我反省。",
					u8"该同学考试不合格，请重修。",
					u8"该同学不适合学习，请退学。"
				};

				constexpr exposing::utf8_string_view format_str{ u8"{:>15}      {:<60}      {:<10}" };
				auto title = exposing::format(format_str, u8"学号", u8"评价", u8"备注");

				Logger::WriteMessage(to_narrow_string(title).c_str());

				for (std::size_t i = 0; i < 50; i++)
				{
					auto str = exposing::format(format_str, student_id_dist(random_engine), comments[comment_dist(random_engine)], u8"本校生");

					Logger::WriteMessage(exposing::to_narrow_string(str).c_str());
				}

				auto str1 = exposing::format(u8"这是 {2}、{1} 和 {0}。", u8"张三", u8"李四", u8"王五");
				auto str2 = exposing::format(u8"{0} 的二进制形式是 {0:#b}，八进制形式是 {0:#o}，十六进制形式是 {0:#x}。", 123456789ULL);
				auto str3 = exposing::format(u8"{0} 保留两位小数是 {0:.2f}，保留四位小数是 {0:.4f}。", std::atan(1) * 4);

				Logger::WriteMessage(exposing::to_narrow_string(str1).c_str());
				Logger::WriteMessage(exposing::to_narrow_string(str2).c_str());
				Logger::WriteMessage(exposing::to_narrow_string(str3).c_str());

				// 更多格式表达式请参阅：https://fmt.dev/latest/syntax.html
			}

			// 例9：非接口类型的装箱和拆箱。
			{
				std::array data{ 1, 2, 3, 4, 5 };
				exposing::unknown_object obj_int = exposing::box(123);
				exposing::unknown_object obj_str = exposing::box(u8"Hello World!");
				exposing::unknown_object obj_span = exposing::box(exposing::param_span<int>{ data });

				int val_int = exposing::unbox<int>(obj_int);
				auto val_str = exposing::unbox<exposing::param_string>(obj_str);
				auto val_span = exposing::unbox<exposing::param_span<int>>(obj_span);

				Assert::AreEqual(123, val_int);
				Assert::AreEqual("Hello World!", exposing::to_narrow_string(val_str).c_str());
				Assert::AreEqual(5ULL, val_span.size());
				Assert::AreEqual(3, val_span.data()[2]);
			}

			// 例10：标准异常处理。
			{
				try
				{
					auto data = exposing::make_param_hash_map<exposing::param_string, exposing::param_string>(
						{
							{ u8"CRH1A-200", u8"CRH1A-200型动车组" },
							{ u8"CRH2A", u8"CRH2A型动车组" },
							{ u8"CRH3A", u8"CRH3A型动车组" },
							{ u8"CRH380A", u8"CRH380A型动车组" }
						});

					auto value = data.get_value(u8"CR400AF");
				}
				catch (const exposing::abi_key_not_found& ex)
				{
					Logger::WriteMessage(ex.what_to_narrow().c_str());
				}

				try
				{
					// 错误的格式字符串。
					auto str = exposing::format(u8"{0*:0:0<}", 123);
				}
				catch (const exposing::abi_invalid_argument& ex)
				{
					Logger::WriteMessage(ex.what_to_narrow().c_str());
				}

				try
				{
					// 错误的接口查询。
					exposing::unknown_object obj = exposing::make_param_vector<int>(1, 2, 3);
					auto map = obj.as<exposing::param_hash_map<int, int>>();
				}
				catch (const exposing::abi_no_interface& ex)
				{
					Logger::WriteMessage(ex.what_to_narrow().c_str());
				}

				try
				{
					// 错误的拆箱。
					exposing::unknown_object obj = exposing::box(3.14);
					auto value = exposing::unbox<int>(obj);
				}
				catch (const exposing::abi_no_interface& ex)
				{
					Logger::WriteMessage(ex.what_to_narrow().c_str());
				}
			}

			// 例11：加载外部动态组件。
			{
				if (auto factory = exposing::component_loader::instance().add_module_with_factory(u8"libplugin_demo.dll"))
				{
					Logger::WriteMessage(exposing::to_narrow_string(exposing::format(u8"组件名：{}", factory.library_name())).c_str());

					auto interfaces = factory.qualified_names();

					for (const auto& item : interfaces)
					{
						Logger::WriteMessage(exposing::to_narrow_string(exposing::format(u8"组件实现接口：{}", item)).c_str());
					}

					if (!interfaces.empty())
					{
						auto demo = factory.create_instance(interfaces[0]).as<excalibur::plugin_demo>();

						Logger::WriteMessage(exposing::to_narrow_string(demo.name()).c_str());
						demo.print(exposing::make_param_hash_map<exposing::param_string, exposing::param_string>(
							{
								{ u8"天真的歌谣啊", u8"唱给谁啊" },
								{ u8"天真的你们啊", u8"在哪啊" },	
								{ u8"就随着浪花吧", u8"就随它" },
								{ u8"反正也没有彼岸", u8"可到达" }
							}));
						
						for (auto item : demo.get_values(1000, 1010))
						{
							Logger::WriteMessage(std::to_string(item).c_str());
						}

						for (auto item : demo)
						{
							Logger::WriteMessage(exposing::to_narrow_string(item).c_str());
						}
					}
				}
			}
		}
	};
}
