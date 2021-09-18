#include "kcf_tracker_internal.hpp"
#include "hardcode.hpp"
#include "kcf_tracker_impl.hpp"
#include "track_info_impl.hpp"
#include "kcftracker.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
// #include <opencv2/highgui/highgui.hpp>

#include <fstream>
#include <algorithm>
#include <sstream>
#include "dirent.h"
#include <time.h>
#include <cfloat>

namespace glasssix::banshee
{
    class kcf_tracker_internal::impl
    {
    public:
        impl(exposing::param_span<std::uint8_t> bitmap, std::int32_t width, std::int32_t height, std::int32_t x, std::int32_t y, std::int32_t roi_width, std::int32_t roi_height)
        {
            // KCF tracker config parameters
            bool HOG = true;
            bool FIXEDWINDOW = false;
            bool MULTISCALE = true;
            tracker = new KCFTracker(HOG, FIXEDWINDOW, MULTISCALE);
            cv::Rect roi_bbox(x, y, width, height);
            cv::Mat frame(height, width, CV_8UC3);
            std::copy(bitmap.begin(), bitmap.end(), frame.data);
            tracker->init(roi_bbox, frame);
        }

        track_info update(exposing::param_span<std::uint8_t> bitmap, std::int32_t width, std::int32_t height)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            cv::Mat frame(height, width, CV_8UC3);
            std::copy(bitmap.begin(), bitmap.end(), frame.data);
            trackerResult tracker_result = tracker->update(frame);
            cv::Rect track_bbox = tracker_result.track_roi;
            float track_prob = tracker_result.track_prob;
            track_info_internal track_internal;
            track_internal.x = track_bbox.x;
            track_internal.y = track_bbox.y;
            track_internal.width = track_bbox.width;
            track_internal.height = track_bbox.height;
            track_internal.prob = track_prob;
            return exposing::make_as_first<track_info_impl>(track_internal);
        }

        static std::string version()
        {
            return "1.0.0";
        }

    private:
        KCFTracker *tracker;
    };

    kcf_tracker_internal::kcf_tracker_internal(exposing::param_span<std::uint8_t> bitmap, std::int32_t width, std::int32_t height, std::int32_t x, std::int32_t y, std::int32_t roi_width, std::int32_t roi_height) : impl_{std::make_unique<impl>(bitmap, width, height, x, y, roi_width, roi_height)}
    {
    }

    kcf_tracker_internal::~kcf_tracker_internal()
    {
    }

    track_info kcf_tracker_internal::update(exposing::param_span<std::uint8_t> bitmap, std::int32_t width, std::int32_t height) const
    {
        return impl_->update(bitmap, width, height);
    }

    std::string kcf_tracker_internal::version()
    {
        return impl::version();
    }
}