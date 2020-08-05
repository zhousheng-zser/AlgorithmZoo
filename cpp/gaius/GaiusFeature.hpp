#pragma once

#ifndef _GAIUS_FEATURE_HPP_
#define _GAIUS_FEATURE_HPP_

#ifdef EXPORT_GAIUS
#undef EXPORT_GAIUS
#ifdef _MSC_VER // For Windows
#ifdef _WINDLL // Dynamic lib
#define EXPORT_GAIUS __declspec(dllexport)
#else // Static lib
#define EXPORT_GAIUS
#endif // !_WINDLL
#elif defined(__linux__) // For Linux
#define EXPORT_GAIUS
#endif
#else
#ifdef _MSC_VER
#define EXPORT_GAIUS __declspec(dllimport)
#elif defined(__linux__)
#define EXPORT_GAIUS
#endif
#endif

#include <vector>
#include <cstdint>

namespace glasssix
{
	namespace gaius
	{
		/// <summary>
		/// A common component supporting feature extraction. 
		/// </summary>
		class GaiusFeature
		{
		public:
			class impl;

			/// <summary>
			/// Creates an instance with the default CPU.
			/// </summary>
			GaiusFeature();

			/// <summary>
			/// Creates an instance with a specified GPU core or the default CPU.
			/// </summary>
			/// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
			GaiusFeature(int device);

			/// <summary>
			/// The copy constructor must be disabled in PImpl pattern.
			/// </summary>
			GaiusFeature(const GaiusFeature&) = delete;

			/// <summary>
			/// Destroys the instance.
			/// </summary>
			virtual ~GaiusFeature();

			/// <summary>
			/// The copy assignment operator must be disabled in PImpl pattern.
			/// </summary>
			GaiusFeature& operator=(const GaiusFeature&) = delete;

			/// <summary>
			/// Forwards the input data and gets the result.
			/// </summary>
			/// <param name="input_data">The input data arranged in specified order</param>
			/// <param name="num">The number of bitmaps within the input data</param>
			/// <param name="order">The order that the input data are arranged in</param>
			/// <returns>The feature vectors</returns>
			std::vector<std::vector<float>> Forward(const std::uint8_t* input_data, unsigned num, int order = 0, bool mask = false) const;

			/// <summary>
			/// Gets the version of the component.
			/// </summary>
			/// <returns>The version</returns>
			static const char* getVersion();
		private:
			impl* impl_;
		};
	}
}

#endif // !_GAIUS_FEATURE_HPP_