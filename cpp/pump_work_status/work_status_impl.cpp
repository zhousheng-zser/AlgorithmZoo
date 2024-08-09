#include "work_status_impl.hpp"
// #include "work_status_internal.hpp"
#include <Primitives/tensor_conversions.hpp>

#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include "general.hpp"
namespace glasssix::pump_work_status
{
    work_status_impl::work_status_impl()
    {
    }

    work_status_impl::~work_status_impl()
    {
    }

    void work_status_impl::init(std::int32_t device)
	{
	}

    exposing::param_string work_status_impl::version() const
    {
        std::string work_status_version ="1.0.0";
        return exposing::to_param_string(work_status_version);
    }

    std::int32_t work_status_impl::status(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, 
         const exposing::param_vector<int>& rois, 
    const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const
    { 
        std::map<std::string, float> param_map;
        for (auto it : param_map_abi) 
            param_map.insert(std::make_pair(it.key(), it.value()));
        
        bool big_paint_room =static_cast<bool> (std::round(param_map.count("big_paint_room")?param_map["big_paint_room"]:0));

        if (bitmap.empty())
            throw exposing::abi_invalid_argument("current frame is empty");

        CHECK_EQ(channels, 3);
        CHECK_EQ(bitmap.size(), channels * height * width);

		cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));

        std::vector<std::vector<int>> mask_array(rois.size()/2);
        for (size_t i = 0; i < rois.size(); i+=2)
            mask_array[i/2] = std::vector<int> {rois[i],rois[i+1]} ;

        clockwise_sort_by_left_corner(mask_array);

        return( classify_lamp_status(image,  big_paint_room , mask_array) && classify_base_plate_status(image, big_paint_room , mask_array)&&classify_work_equipment_status(image, big_paint_room ,mask_array ) );

    }

}
