#include "yolov5s_net_impl.hpp"
#include "yolov5s_net_internal.hpp"
#include "result_info_impl.hpp"

namespace glasssix::valklyrs
{
	yolov5s_net_impl::yolov5s_net_impl()
	{
	}

	yolov5s_net_impl::~yolov5s_net_impl()
	{
	}

	void yolov5s_net_impl::init(const exposing::param_string &yolov5s_racy_path, const exposing::param_string &vehicle_racy_path, const exposing::param_string &person_racy_path, std::int32_t device)
	{
		impl_ = std::make_unique<yolov5s_net_internal>(exposing::to_narrow_string(yolov5s_racy_path), exposing::to_narrow_string(vehicle_racy_path), exposing::to_narrow_string(person_racy_path), device);
	}

	void yolov5s_net_impl::init(exposing::param_span<const exposing::param_string> yolov5s_phai, const exposing::param_string &yolov5s_racy_path, exposing::param_span<const exposing::param_string> vehicle_phai, const exposing::param_string &vehicle_racy_path, exposing::param_span<const exposing::param_string> person_phai, const exposing::param_string &person_racy_path, std::int32_t device)
	{
		std::vector<std::string> yolov5s_phai_internal(yolov5s_phai.size());
		std::vector<std::string> vehicle_phai_internal(vehicle_phai.size());
		std::vector<std::string> person_phai_internal(person_phai.size());

		std::transform(yolov5s_phai.begin(), yolov5s_phai.end(), yolov5s_phai_internal.begin(), &exposing::to_narrow_string);
		std::transform(vehicle_phai.begin(), vehicle_phai.end(), vehicle_phai_internal.begin(), &exposing::to_narrow_string);
		std::transform(person_phai.begin(), person_phai.end(), person_phai_internal.begin(), &exposing::to_narrow_string);
		impl_ = std::make_unique<yolov5s_net_internal>(yolov5s_phai_internal, exposing::to_narrow_string(yolov5s_racy_path), vehicle_phai_internal, exposing::to_narrow_string(vehicle_racy_path), person_phai_internal, exposing::to_narrow_string(person_racy_path), device);
	}

	exposing::param_string yolov5s_net_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_vector<result_info> yolov5s_net_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"valklyrs internal object not initialized");

		return impl_->detect(bitmap, channels, height, width, order);
	}
}
