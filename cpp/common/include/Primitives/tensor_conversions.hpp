#pragma once

#include "tensor.hpp"
#include "tensor_converter.hpp"
#include "tensor_or_shared.hpp"

#include <memory>

namespace glasssix::memory
{
	/// <summary>
	/// Indicate a type which is the destination type of the tensor convertion.
	/// </summary>
	template<typename Destination>
	struct tensor_convert_to_tag {};

	/// <summary>
	/// Indicate a layout type which a tensor is converted to.
	/// </summary>
	template<tensor_layout Layout>
	struct tensor_convert_layout_to_tag {};

	/// <summary>
	/// A template variable to simplify coding.
	/// </summary>
	template<typename Destination>
	tensor_convert_to_tag<Destination> tensor_convert_to;

	/// <summary>
	/// A template variable to simplify coding.
	/// </summary>
	template<tensor_layout Layout>
	tensor_convert_layout_to_tag<Layout> tensor_convert_layout_to;

	/// <summary>
	/// Allocate a tensor in a uniform form.
	/// </summary>
	/// <param name="source">The source tensor</param>
	/// <returns>The allocated tensor</returns>
	template<bool Shared, typename Destination, typename Source>
	auto allocate_tensor(const tensor<Source>& source)
	{
		auto input_vector = source.order() == memory::NHWC ?
			std::vector<int>{ source.num(), source.height(), source.width(), source.channels() } :
			std::vector<int>{ source.num(),  source.channels(),  source.height(),  source.width() };

		if constexpr (Shared)
		{
			return std::make_shared<tensor<Destination>>(input_vector, source.device(), source.order()/*, &memory::pool_allocator_default<Destination>::get()*/);
		}
		else
		{
			return tensor<Destination>{ input_vector, source.device(), source.order()/*, & memory::pool_allocator_default<Destination>::get()*/ };
		}
	}

	/// <summary>
	/// Assert the template argumant is numeric statically.
	/// </summary>
	template<typename Args>
	constexpr void assert_numeric()
	{
		static_assert(std::is_arithmetic_v<Args>, "All parameters must be numerical.");
	}

	/// <summary>
	/// An internal function to convert the underlying type of a tensor to another within a duplication.
	/// </summary>
	/// <param name="source">The source tensor</param>
	/// <param name="tag">The tag containing information about the destination underlying type</param>
	/// <returns>The destination tensor</returns>
	template<bool Shared, typename Source, typename Destination>
	auto convert_to_core(const tensor_or_shared<Source, Shared>& source, const tensor_convert_to_tag<Destination>& tag)
	{
		assert_numeric<Source>();
		assert_numeric<Destination>();

		auto destination = allocate_tensor<Shared, Destination>(source.access());
		tensor_or_shared<Destination, Shared> destination_wrapper{ destination };

		if (source->device() < 0)
		{
			tensor_converter_v<Source, Destination, tensor_cpu_tag>(source.access(), destination_wrapper.access());
		}
		else
		{
			tensor_converter_v<Source, Destination, tensor_cpu_tag>(source.access(), destination_wrapper.access());
		}

		return destination;
	}

	/// <summary>
	/// An internal function to convert the layout of a tensor to another within a duplication.
	/// </summary>
	/// <param name="source">The source tensor</param>
	/// <param name="tag">The tag containing information about the destination underlying type</param>
	/// <returns>The destination tensor</returns>
	template<typename Source, bool Shared, tensor_layout Layout>
	auto convert_layout_to_core(const tensor_or_shared<Source, Shared>& source, const tensor_convert_layout_to_tag<Layout>& tag)
	{
		assert_numeric<Source>();

		auto destination = allocate_tensor<Shared, Source>(source.access());
		tensor_or_shared<Source, Shared> destination_wrapper{ destination };

		if (source->device() < 0)
		{
			tensor_layout_converter_v<Source, Layout, tensor_cpu_tag>(source.access(), destination_wrapper.access());
		}
		else
		{
			tensor_layout_converter_v<Source, Layout, tensor_cpu_tag>(source.access(), destination_wrapper.access());
		}

		return destination;
	}

	/// <summary>
	/// Provide support for convertions of the underlying type for a tensor.
	/// </summary>
	/// <param name="source">The source tensor</param>
	/// <param name="tag">The tag containing information about the destination underlying type</param>
	/// <returns>The destination tensor</returns>
	template<typename Source, typename Destination>
	tensor<Destination> operator|(const tensor<Source>& source, const tensor_convert_to_tag<Destination>& tag)
	{
		return convert_to_core<false, Source>(source, tag);
	}

	/// <summary>
	/// Provide support for convertions of the underlying type for a tensor.
	/// </summary>
	/// <param name="source">The source tensor</param>
	/// <param name="tag">The tag containing information about the destination underlying type</param>
	/// <returns>The destination tensor</returns>
	template<typename Source, typename Destination>
	std::shared_ptr<tensor<Destination>> operator|(const std::shared_ptr<tensor<Source>>& source, const tensor_convert_to_tag<Destination>& tag)
	{
		return convert_to_core<true, Source>(source, tag);
	}

	/// <summary>
	/// Provide support for convertions of the layout for a tensor.
	/// </summary>
	/// <param name="source">The source tensor</param>
	/// <param name="tag">The tag containing information about the destination layout</param>
	/// <returns>The destination tensor</returns>
	template<typename Source, tensor_layout Layout>
	tensor<Source> operator|(const tensor<Source>& source, const tensor_convert_layout_to_tag<Layout>& tag)
	{
		return convert_layout_to_core<Source, false>(source, tag);
	}

	/// <summary>
	/// Provide support for convertions of the layout for a tensor.
	/// </summary>
	/// <param name="source">The source tensor</param>
	/// <param name="tag">The tag containing information about the destination layout</param>
	/// <returns>The destination tensor</returns>
	template<typename Source, tensor_layout Layout>
	std::shared_ptr<tensor<Source>> operator|(const std::shared_ptr<tensor<Source>>& source, const tensor_convert_layout_to_tag<Layout>& tag)
	{
		return convert_layout_to_core<Source, true>(source, tag);
	}
}
