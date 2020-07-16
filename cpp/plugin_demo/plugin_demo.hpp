#pragma once

// 本文件定义了一个 ABI 接口示例。

#include <abi/consumer.hpp>

namespace glasssix::excalibur
{
	struct plugin_demo;
}

namespace glasssix::exposing::impl
{
	/// <summary>
	/// 第一步：创建 ABI 类型特化，该特化描述了接口的抽象。
	/// </summary>
	template<> struct abi<excalibur::plugin_demo>
	{
		/// <summary>
		/// 类型标识：接口类型
		/// </summary>
		using identity_type = type_identity_interface;

		/// <summary>
		/// 类型的唯一标识符，可使用 工具-创建GUID 命令生成。
		/// </summary>
		static constexpr guid id{ "84002B53-6090-49EF-A75D-5419865EA7A8" };

		/// <summary>
		/// 定义 ABI 抽象
		/// </summary>
		struct type : abi_unknown_object
		{
			/// <summary>
			/// 定义一个无参函数，其返回值为字符串。
			/// </summary>
			/// <param name="result">返回值</param>
			/// <returns>错误码</returns>
			virtual std::int32_t G6_ABI_CALL name(abi_out_t<param_string> result) noexcept = 0;

			/// <summary>
			/// 定义一个无返回值函数，其有一个参数，类型为 map&lt;string, string&gt;。
			/// </summary>
			/// <param name="params">参数</param>
			/// <returns>错误码</returns>
			virtual std::int32_t G6_ABI_CALL print(abi_in_t<param_hash_map<param_string, param_string>> params) noexcept = 0;

			/// <summary>
			/// 定义一个函数，其有两个整型参数 from 和 to，返回值为 vector&lt;int&gt;。
			/// </summary>
			/// <param name="from">参数1</param>
			/// <param name="to">参数2</param>
			/// <param name="result">返回值</param>
			/// <returns>错误码</returns>
			virtual std::int32_t G6_ABI_CALL get_values(std::int32_t from, std::int32_t to, abi_out_t<param_vector<std::int32_t>> result) noexcept = 0;
		};
	};

	/// <summary>
	/// 第二步：创建静态虚函数表。
	/// </summary>
	template<typename Derived>
	struct interface_vtable<Derived, excalibur::plugin_demo> : interface_vtable_base<Derived, excalibur::plugin_demo>
	{
		virtual std::int32_t G6_ABI_CALL name(abi_out_t<param_string> result) noexcept override
		{
			// abi_safe_call 的作用是把 C++ 异常翻译为错误码 + 错误描述信息，在客户端重构。
			return abi_safe_call([&] { *result = detach_abi(this->self().name()); });
		}

		virtual std::int32_t G6_ABI_CALL print(abi_in_t<param_hash_map<param_string, param_string>> params) noexcept override
		{
			return abi_safe_call([&] { this->self().print(create_from_abi<param_hash_map<param_string, param_string>>(params)); });
		}

		virtual std::int32_t G6_ABI_CALL get_values(std::int32_t from, std::int32_t to, abi_out_t<param_vector<std::int32_t>> result) noexcept override
		{
			// 通过 create_from_abi 函数将 ABI 指针转换为具体类型，从而在客户端还原。
			// 通过 detach_abi 函数将具体类型和 ABI 指针解绑并返回 ABI 类型。
			return abi_safe_call([&] { *result = detach_abi(this->self().get_values(create_from_abi<std::int32_t>(from), create_from_abi<std::int32_t>(to))); });
		}
	};

	/// <summary>
	/// 第三步：创建现代 C++ ABI 适配层。
	/// </summary>
	template<> struct abi_adapter<excalibur::plugin_demo>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, excalibur::plugin_demo>
		{
			param_string name() const
			{
				param_string result{ nullptr };

				return (check_abi_result(this->self_abi().name(put_abi(result))), result);
			}

			void print(const param_hash_map<param_string, param_string>& params) const
			{
				check_abi_result(this->self_abi().print(get_abi(params)));
			}

			param_vector<std::int32_t> get_values(std::int32_t from, std::int32_t to) const
			{
				param_vector<std::int32_t> result{ nullptr };

				return (check_abi_result(this->self_abi().get_values(get_abi(from), get_abi(to), put_abi(result))), result);
			}
		};
	};
}

namespace glasssix::excalibur
{
	/// <summary>
	/// 第四步：声明接口，其继承自自身和 exposing::iterable_object&lt;exposing::param_string&gt; 接口，支持迭代器。
	/// </summary>
	struct plugin_demo : exposing::inherits<plugin_demo, exposing::iterable_object<exposing::param_string>>
	{
		using inherits<plugin_demo, exposing::iterable_object<exposing::param_string>>::inherits;
	};
}
