#ifndef __YOLOV5S_NET_INTERNAL_HPP__
#define __YOLOV5S_NET_INTERNAL_HPP__

#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

#include "result_info.hpp"
#include "vp_info.hpp"

namespace glasssix::valklyrs
{
    struct anchor_box
    {
        float x;
        float y;
        float height;
        float width;
    };

    struct obj_info_internal
    {
        anchor_box rect;
        int label;
        float prob;
    };

    struct vp_info_internal
    {
        exposing::param_vector<float> coordinates;
        exposing::param_vector<exposing::param_string> attributes;
    };

    struct result_info_internal
    {
        exposing::param_vector<vp_info> vehicle_list;
        exposing::param_vector<vp_info> person_list;
    };

    struct person_attribute
    {
        static constexpr std::array<const char *, 2> gender{"female", "male"};
        static constexpr std::array<const char *, 3> age{"AgeOver60", "Age18-60", "AgeLess18"};
        static constexpr std::array<const char *, 3> ori{"Front", "Side", "Back"};
        static constexpr std::array<const char *, 2> hat{"No_Hat", "Hat"};
        static constexpr std::array<const char *, 2> glass{"No_Glasses", "Glasses"};
        static constexpr std::array<const char *, 2> handbag{"No_HandBag", "HandBag"};
        static constexpr std::array<const char *, 2> shoulderbag{"No_ShoulderBag", "ShoulderBag"};
        static constexpr std::array<const char *, 2> backpack{"No_Backpack", "Backpack"};
        static constexpr std::array<const char *, 2> sleeve{"ShortSleeve", "LongSleeve"};
        static constexpr std::array<const char *, 3> texture{"plaid", "splice", "pure"};
        static constexpr std::array<const char *, 3> lower_type{"Trousers", "Shorts", "Skirt&Dress"};
    };

    /// <summary>
    /// A common component supporting anti-spoofing.
    /// </summary>
    class yolov5s_net_internal
    {
    public:
        class impl;

        /// <summary>
        /// Creates an instance with a specified GPU core or the default CPU.
        /// </summary>
        /// <param name="racy_path">The model path</param>
        /// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
        yolov5s_net_internal(std::string_view yolov5s_racy_path, std::string_view vehicle_racy_path, std::string_view person_racy_path, int device);

        /// <summary>
        /// Creates an instance with a specified GPU core or the default CPU.
        /// </summary>
        /// <param name="phai_path">The phai</param>
        /// <param name="racy_path">The model path</param>
        /// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
        yolov5s_net_internal(const std::vector<std::string> &yolov5s_phai, std::string_view yolov5s_racy_path, const std::vector<std::string> &vehicle_phai, std::string_view vehicle_racy_path, const std::vector<std::string> &person_phai, std::string_view person_racy_path, int device);

        /// <summary>
        /// The copy constructor must be disabled in PImpl pattern.
        /// </summary>
        yolov5s_net_internal(const yolov5s_net_internal &) = delete;

        /// <summary>
        /// Destroys the instance.
        /// </summary>
        virtual ~yolov5s_net_internal();

        /// <summary>
        /// The copy assignment operator must be disabled in PImpl pattern.
        /// </summary>
        yolov5s_net_internal &operator=(const yolov5s_net_internal &) = delete;

        /// <summary>
        /// Extracts the feature data.
        /// </summary>
        /// <param name="bitmaps">Some bitmaps (128x128x3) arranged in specified order</param>
        /// <param name="count">The count of bitmaps in the buffer</param>
        /// <param name="order">The order that the bitmaps are arranged in</param>
        /// <returns>The feature vectors</returns>
        exposing::param_vector<valklyrs::result_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const;

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