#pragma once

#include "tensor.hpp"
#include "tensor_helper.hpp"
#include "tensor_layout.hpp"

#include <cassert>

namespace glasssix::memory
{
	/// <summary>
	/// Indicate a CPU-based convertion.
	/// </summary>
	struct tensor_cpu_tag {};

	/// <summary>
	/// Indicate a GPU-based convertion.
	/// </summary>
	struct tensor_gpu_tag {};

	/// <summary>
	/// The default converter for underlying types.
	/// </summary>
	template<typename Source, typename Destination>
	struct tensor_underlying_type_converter
	{
		Destination operator()(const Source& source) const
		{
			return (Destination)source;
		}
	};

	template<typename Source, typename Destination>
	tensor_underlying_type_converter<Source, Destination> tensor_underlying_type_converter_v;

	/// <summary>
	/// The default converter.
	/// </summary>
	template<typename Source, typename Destination, typename TTag>
	struct tensor_converter {};

	/// <summary>
	/// The default CPU-based converter.
	/// </summary>
	template<typename Source, typename Destination>
	struct tensor_converter<Source, Destination, tensor_cpu_tag>
	{
		void operator()(const tensor<Source>& source, tensor<Destination>& destination) const
		{
			auto source_ptr = source.cpu_data();
			auto destination_ptr = destination.mutable_cpu_data();
			auto count = std::min(source.count(), destination.count());

			// Assign the data directly.
			for (size_t i = 0; i < count; i++)
			{
				destination_ptr[i] = tensor_underlying_type_converter_v<Source, Destination>(source_ptr[i]);
			}
		}
	};

	template<typename Source, typename Destination, typename TTag>
	tensor_converter<Source, Destination, TTag> tensor_converter_v;

	/// <summary>
	/// The default layout converter.
	/// </summary>
	template<typename UnderlyingType, tensor_layout Layout, typename TTag>
	struct tensor_layout_converter {};

	/// <summary>
	/// The default CPU-based layout converter.
	/// Layout type: grayscale 8-bit.
	/// </summary>
	template<typename UnderlyingType>
	struct tensor_layout_converter<UnderlyingType, tensor_layout::grayscale, tensor_cpu_tag>
	{
		void operator()(const tensor<UnderlyingType>& source, tensor<UnderlyingType>& destination) const
		{
			// We only support triple-channel bitmaps and single-channel bitmaps.
			assert(tensor_helper::has_single_channel(source) || tensor_helper::has_triple_channel(source) || tensor_helper::has_quadruple_channel(source));

			switch (source.channels())
			{
			case 1:
				destination = std::move(source.clone());
				break;
			case 3:
				tensor_helper::rgb_or_rgba_to_gray(source, destination, 1);
				break;
			case 4:
				tensor_helper::rgb_or_rgba_to_gray(source, destination, 1);
				break;
			default:
				break;
			}
		}
	};

	/// <summary>
	/// The default CPU-based layout converter.
	/// Layout type: grayscale 32-bit.
	/// </summary>
	template<typename UnderlyingType>
	struct tensor_layout_converter<UnderlyingType, tensor_layout::grayscale_3, tensor_cpu_tag>
	{
		void operator()(const tensor<UnderlyingType>& source, tensor<UnderlyingType>& destination) const
		{
			// We only support triple-channel bitmaps and single-channel bitmaps.
			assert(tensor_helper::has_single_channel(source) || tensor_helper::has_triple_channel(source) || tensor_helper::has_quadruple_channel(source));

			switch (source.channels())
			{
			case 1:
				destination = std::move(source.clone());
				break;
			case 3:
				tensor_helper::rgb_or_rgba_to_gray(source, destination, 3);
				break;
			case 4:
				tensor_helper::rgb_or_rgba_to_gray(source, destination, 3);
				break;
			default:
				break;
			}
		}
	};

	/// <summary>
	/// The default CPU-based layout converter.
	/// Layout type: RGB.
	/// </summary>
	template<typename UnderlyingType>
	struct tensor_layout_converter<UnderlyingType, tensor_layout::rgb, tensor_cpu_tag>
	{
		void operator()(const tensor<UnderlyingType>& source, tensor<UnderlyingType>& destination) const
		{
			// We only support triple-channel bitmaps and single-channel bitmaps.
			assert(tensor_helper::has_triple_channel(source) || tensor_helper::has_quadruple_channel(source));

			switch (source.channels())
			{
			case 3:
				destination = std::move(source.clone());
				break;
			case 4:
				tensor_helper::rgba_to_rgb(source, destination);
				break;
			default:
				break;
			}
		}
	};

	template<typename UnderlyingType, tensor_layout Layout, typename TTag>
	tensor_layout_converter<UnderlyingType, Layout, TTag> tensor_layout_converter_v;
}
