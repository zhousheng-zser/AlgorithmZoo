#include "ocr_net_impl.hpp"
#include "ocr_net_internal.hpp"
#include "box_info_impl.hpp"

namespace glasssix::mjollner
{
	ocr_net_impl::ocr_net_impl()
	{
	}

	ocr_net_impl::~ocr_net_impl()
	{
	}

	void ocr_net_impl::init(const exposing::param_string& det_racy_path, const exposing::param_string& rec_racy_path, const exposing::param_string &alphabet_path, std::int32_t device)
	{
		impl_ = std::make_unique<ocr_net_internal>(exposing::to_narrow_string(det_racy_path), exposing::to_narrow_string(rec_racy_path), exposing::to_narrow_string(alphabet_path), device);
	}

	void ocr_net_impl::init(exposing::param_span<const exposing::param_string> det_phai, const exposing::param_string& det_racy_path, exposing::param_span<const exposing::param_string> rec_phai, const exposing::param_string& rec_racy_path, const exposing::param_string &alphabet_path, std::int32_t device)
	{
		std::vector<std::string> det_phai_internal(det_phai.size());
		std::vector<std::string> rec_phai_internal(rec_phai.size());

		std::transform(det_phai.begin(), det_phai.end(), det_phai_internal.begin(), &exposing::to_narrow_string);
		std::transform(rec_phai.begin(), rec_phai.end(), rec_phai_internal.begin(), &exposing::to_narrow_string);
		impl_ = std::make_unique<ocr_net_internal>(det_phai_internal, exposing::to_narrow_string(det_racy_path), rec_phai_internal, exposing::to_narrow_string(rec_racy_path), exposing::to_narrow_string(alphabet_path), device);
	}

	exposing::param_string ocr_net_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_vector<box_info> ocr_net_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order, int x, int y, int roi_width, int roi_height) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"mjollner internal object not initialized");

		return impl_->detect(bitmap, channels, height, width, order, x, y, roi_width, roi_height);
	}
}
