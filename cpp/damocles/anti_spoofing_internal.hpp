#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

#include <abi/param_span.hpp>
#include "../longinus/face_info.hpp"

namespace glasssix::damocles
{
	/// <summary>
	/// A common component supporting anti-spoofing. 
	/// </summary>
	class anti_spoofing_internal
	{
	public:
		class impl;

		/// <summary>
		/// Creates an instance with a specified GPU core or the default CPU.
		/// </summary>
		/// <param name="racy_path">The model path</param>
		/// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
		anti_spoofing_internal(std::string_view models_directory, int model_type, int device);

		/// <summary>
		/// The copy constructor must be disabled in PImpl pattern.
		/// </summary>
		anti_spoofing_internal(const anti_spoofing_internal&) = delete;

		/// <summary>
		/// Destroys the instance.
		/// </summary>
		virtual ~anti_spoofing_internal();

		/// <summary>
		/// The copy assignment operator must be disabled in PImpl pattern.
		/// </summary>
		anti_spoofing_internal& operator=(const anti_spoofing_internal&) = delete;

		/// <summary>
		/// Extracts the feature data.
		/// </summary>
		/// <param name="bitmaps">Some bitmaps (128x128x3) arranged in specified order</param>
		/// <param name="count">The count of bitmaps in the buffer</param>
		/// <param name="order">The order that the bitmaps are arranged in</param>
		/// <returns>The feature vectors</returns>
		std::vector<std::vector<float>> spoofing_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const;

		/// <summary>
		/// Extracts the feature data.
		/// </summary>
		/// <param name="action_cmd">action command type</param>
		/// <param name="bitmaps">Some bitmaps (128x128x3) arranged in specified order</param>
		/// <param name="count">The count of bitmaps in the buffer</param>
		/// <param name="order">The order that the bitmaps are arranged in</param>
		/// <returns>The feature vectors</returns>
		bool presentation_attack_detect(int action_cmd, const longinus::face_info& face, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const;

		/// <summary>
		/// Gets the version of the component.
		/// </summary>
		/// <returns>The version</returns>
		static std::string version();
	private:
		std::unique_ptr<impl> impl_;
	};
}