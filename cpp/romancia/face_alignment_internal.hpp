#pragma once
#ifndef _FACE_ALIGNMENT_INTERNAL_HPP_
#define _FACE_ALIGNMENT_INTERNAL_HPP_
#include <string>

#include <abi/consumer.hpp>
#include "Excalibur/pipeline.hpp"

namespace glasssix::romancia
{
	class face_alignment_internal
	{
	public:
		class impl;

		/// <summary>
		/// Creates an instance with the default CPU.
		/// </summary>
		face_alignment_internal();

		/// <summary>
		/// Creates an instance with a specified GPU core or the default CPU.
		/// </summary>
		/// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
		face_alignment_internal(int device);

		/// <summary>
		/// The copy constructor must be disabled in PImpl pattern.
		/// </summary>
		face_alignment_internal(const face_alignment_internal&) = delete;

		/// <summary>
		/// Destroys the instance.
		/// </summary>
		virtual ~face_alignment_internal();

		/// <summary>
		/// The copy assignment operator must be disabled in PImpl pattern.
		/// </summary>
		face_alignment_internal& operator=(const face_alignment_internal&) = delete;

		/// <summary>
		/// Extracts the feature data.
		/// </summary>
		/// <param name="gray_bitmap">gray bitmap that be detected</param>
		/// <param name="height">The height of bitmap</param>
		/// <param name="width">The width of bitmap</param>
		/// <param name="bboxes">The bboxes of faces on bitmap</param>
		/// <param name="bboxes">The landmarks of faces on bitmap</param>
		/// <returns>The feature vectors</returns>
		exposing::param_vector<std::uint8_t> align(exposing::param_span<std::uint8_t> gray_bitmap, std::int32_t height, std::int32_t width,
			exposing::param_vector<exposing::param_vector<std::int32_t>> bboxes, exposing::param_vector<exposing::param_vector<std::int32_t>> landmarks) const;

		/// <summary>
		/// Gets the version of the component.
		/// </summary>
		/// <returns>The version</returns>
		static std::string version();
	private:

		impl* impl_;
	};
}

#endif