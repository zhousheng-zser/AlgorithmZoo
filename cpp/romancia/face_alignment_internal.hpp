#pragma once
#ifndef _FACE_ALIGNMENT_INTERNAL_HPP_
#define _FACE_ALIGNMENT_INTERNAL_HPP_

#include "../longinus/face_info.hpp"

#include <memory>
#include <string>

#include <abi/consumer.hpp>

namespace glasssix::romancia
{
	class face_alignment_internal
	{
	public:
		class impl;

		/// <summary>
		/// Creates an instance with the default CPU.
		/// </summary>
		face_alignment_internal() = delete;

		/// <summary>
		/// Creates an instance with a specified GPU core or the default CPU.
		/// </summary>
		/// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
		face_alignment_internal(/*const exposing::param_string& mask_detector_model_path, */const exposing::param_string& antispoofing_model_path, int device);

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
		/// <param name="bitmap">gray bitmap that be detected</param>
		/// <param name="height">The height of bitmap</param>
		/// <param name="width">The width of bitmap</param>
		/// <param name="faces">The faces informations</param>
		/// <returns>The feature vectors</returns>
		exposing::param_vector<exposing::param_vector<std::uint8_t>> align(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, const exposing::param_vector<longinus::face_info>& faces, std::int32_t order = 1) const;
		exposing::param_vector<exposing::param_vector<std::uint8_t>> align256(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, const exposing::param_vector<longinus::face_info>& faces, std::int32_t order = 1) const;
		
		exposing::param_vector<double> blur_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order = 1) const;


		exposing::param_vector<bool> antispoofing(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order = 1) const;

		//exposing::param_vector<bool> mask_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order = 1) const;
		exposing::param_vector<double> mask_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order = 1) const;

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