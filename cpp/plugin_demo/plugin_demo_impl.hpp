#pragma once

// 本文件定义了一个接口实现的示例。

#include "plugin_demo.hpp"

#include <abi/consumer.hpp>

namespace glasssix::excalibur
{
	inline constexpr utf8_string_view plugin_demo_impl_qualified_name{ u8"glasssix.excalibur.plugin_demo" };

	/// <summary>
	/// 第一步：声明实现类，继承 exposing::implements 模板类用于实现指定的接口。这里我们实现了 plugin_demo 自定义接口。
	/// </summary>
	struct plugin_demo_impl : exposing::implements<plugin_demo_impl, plugin_demo>, exposing::make_external_qualified_name<plugin_demo_impl_qualified_name>
	{
		exposing::param_string name() const;
		void print(const exposing::param_hash_map<exposing::param_string, exposing::param_string>& params) const;
		exposing::param_vector<std::int32_t> get_values(std::int32_t from, std::int32_t to) const;

		/// <summary>
		/// 实现 iterable_object 接口（继承自 plugin_demo 接口）
		/// </summary>
		exposing::object_iterator<exposing::param_string> get_iterator() const;
	};
}
