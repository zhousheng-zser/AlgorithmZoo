#pragma once

#ifndef _SELENE_FEATURE_HPP_
#define _SELENE_FEATURE_HPP_

#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

#include <abi/param_span.hpp>

namespace glasssix::selene
{
	/// <summary>
	/// A common component supporting feature extraction. 
	/// </summary>
	class feature_extractor_internal
	{
	public:
		class impl;

		/// <summary>
		/// Creates an instance with a specified GPU core or the default CPU.
		/// </summary>
		/// <param name="racy_path">The model path</param>
		/// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
		feature_extractor_internal(std::string_view models_directory, int model_type, int device, bool use_int8);

		/// <summary>
		/// The copy constructor must be disabled in PImpl pattern.
		/// </summary>
		feature_extractor_internal(const feature_extractor_internal&) = delete;

		/// <summary>
		/// Destroys the instance.
		/// </summary>
		virtual ~feature_extractor_internal();

		/// <summary>
		/// The copy assignment operator must be disabled in PImpl pattern.
		/// </summary>
		feature_extractor_internal& operator=(const feature_extractor_internal&) = delete;

		/// <summary>
		/// Extracts the feature data.
		/// </summary>
		/// <param name="bitmaps">Some bitmaps (128x128x3) arranged in specified order</param>
		/// <param name="count">The count of bitmaps in the buffer</param>
		/// <param name="order">The order that the bitmaps are arranged in</param>
		/// <returns>The feature vectors</returns>
		std::vector<std::vector<float>> get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order) const;

		/// <summary>
		/// Gets the model type of the component.
		/// </summary>
		/// <returns>The model type</returns>
		std::int32_t get_model_type();

		/// <summary>
		/// Gets the version of the component.
		/// </summary>
		/// <returns>The version</returns>
		static std::string version();
	private:
		std::unique_ptr<impl> impl_;
	};
}

#endif // !_GAIUS_FEATURE_HPP_