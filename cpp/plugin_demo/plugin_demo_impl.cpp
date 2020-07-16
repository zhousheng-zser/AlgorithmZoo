#pragma once

// 本文件定义了一个接口实现的示例。

#include "plugin_demo_impl.hpp"

#include <iostream>

namespace glasssix::excalibur
{
	using namespace exposing;

	param_string plugin_demo_impl::name() const
	{
		return format(u8"我的名字是：{}，行号：{}", to_param_string(__FILE__), __LINE__);
	}

	void plugin_demo_impl::print(const param_hash_map<param_string, param_string>& params) const
	{
		// param_hash_map 支持迭代。
		// 使用 exposing::begin 和 exposing::end 可获取迭代器。
		for (const auto& item : params)
		{
			std::cout << to_narrow_string(format(u8"[{}, {}]", item.key(), item.value())) << std::endl;
		}
	}

	param_vector<std::int32_t> plugin_demo_impl::get_values(std::int32_t from, std::int32_t to) const
	{
		auto result = make_param_vector<std::int32_t>();

		for (std::int32_t i = from; i < to; i++)
		{
			result.push_back(i);
		}

		return result;
	}

	object_iterator<param_string> plugin_demo_impl::get_iterator() const
	{
		static const std::initializer_list<int> numbers{ 1, 2, 3, 4, 5 };

		struct impl : implements<impl, object_iterator<param_string>>
		{
			impl(const std::initializer_list<int>& list) : iter_{ list.begin() }, iter_end_{ list.end() }
			{
			}

			param_string current() const
			{
				return format(u8"我是 {}", *iter_);
			}

			bool valid() const
			{
				return iter_ != iter_end_;
			}

			bool move_to_next()
			{
				return ++iter_ != iter_end_;
			}
		private:
			std::initializer_list<int>::const_iterator iter_;
			std::initializer_list<int>::const_iterator iter_end_;
		};

		return make_as_first<impl>(numbers);
	}
}
