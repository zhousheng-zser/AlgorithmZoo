#ifndef __YOLO_NET_INTERNAL_HPP__
#define __YOLO_NET_INTERNAL_HPP__

#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

#include "box_info.hpp"

namespace glasssix::leavepost
{
    struct anchor_box
	{
		float x;
		float y;
		float height;
		float width;
	};

    struct box_info_internal
    {
       anchor_box rect;
       int label;
       float confidence;
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
		yolo_net_internal(std::string_view model_directory, int device);

		yolo_net_internal(const yolo_net_internal&) = delete;
		virtual ~yolo_net_internal();
		yolo_net_internal& operator=(const yolo_net_internal&) = delete;

		exposing::param_vector<box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height) const;

		/// <summary>
		/// Gets the version of the component.
		/// </summary>
		/// <returns>The version</returns>
		std::string version();
	private:
		std::unique_ptr<impl> impl_;
	};
}
#endif