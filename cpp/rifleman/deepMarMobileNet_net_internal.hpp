#ifndef __deepMarMobileNet_net_INTERNAL_HPP__
#define __deepMarMobileNet_net_INTERNAL_HPP__

#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <abi/consumer.hpp>

namespace glasssix::rifleman
{
    class deepMarMobileNet_net_internal
    {
    public:
        class impl;

        deepMarMobileNet_net_internal(std::string_view deepMarMobileNet_racy_path, int device);
        deepMarMobileNet_net_internal(std::string_view deepMarMobileNet_phai, std::string_view deepMarMobileNet_racy_path, int device);
        deepMarMobileNet_net_internal(const std::vector<std::string>& deepMarMobileNet_phai, std::string_view deepMarMobileNet_racy_path, int device);

        deepMarMobileNet_net_internal();
        virtual ~deepMarMobileNet_net_internal();

        deepMarMobileNet_net_internal(const deepMarMobileNet_net_internal&) = delete;
        deepMarMobileNet_net_internal& operator=(const deepMarMobileNet_net_internal&) = delete;
        
        static std::string version();

        exposing::param_vector<exposing::param_vector<exposing::param_pair<float, exposing::param_string>>> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const;

    private:
        std::unique_ptr<impl> impl_;
    };
}
#endif