#ifndef __YOLO_NET_INTERNAL_HPP__
#define __YOLO_NET_INTERNAL_HPP__

#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

#include "hat_info.hpp"

namespace glasssix::gungnir
{
    struct anchor_box
	{
		float x;
		float y;
		float height;
		float width;
	};

    struct hat_info_internal
    {
       anchor_box rect;
       int label;
       float prob;
    };

	/// <summary>
	/// A common component supporting anti-spoofing. 
	/// </summary>
	class yolo_net_internal
	{
	public:
		class impl;

		/// <summary>
		/// Creates an instance with a specified GPU core or the default CPU.
		/// </summary>
		/// <param name="racy_path">The model path</param>
		/// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
		yolo_net_internal(std::string_view racy_path, int device);

		/// <summary>
		/// Creates an instance with a specified GPU core or the default CPU.
		/// </summary>
		/// <param name="phai_path">The phai</param>
		/// <param name="racy_path">The model path</param>
		/// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
		yolo_net_internal(const std::vector<std::string>& phai, std::string_view racy_path, int device);

		/// <summary>
		/// The copy constructor must be disabled in PImpl pattern.
		/// </summary>
		yolo_net_internal(const yolo_net_internal&) = delete;

		/// <summary>
		/// Destroys the instance.
		/// </summary>
		virtual ~yolo_net_internal();

		/// <summary>
		/// The copy assignment operator must be disabled in PImpl pattern.
		/// </summary>
		yolo_net_internal& operator=(const yolo_net_internal&) = delete;

		/// <summary>
		/// Extracts the feature data.
		/// </summary>
		/// <param name="bitmaps">Some bitmaps (128x128x3) arranged in specified order</param>
		/// <param name="count">The count of bitmaps in the buffer</param>
		/// <param name="order">The order that the bitmaps are arranged in</param>
		/// <returns>The feature vectors</returns>
		exposing::param_vector<hat_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const;

		/// <summary>
		/// Gets the version of the component.
		/// </summary>
		/// <returns>The version</returns>
		static std::string version();
	private:
		std::unique_ptr<impl> impl_;
	};
}
#endif