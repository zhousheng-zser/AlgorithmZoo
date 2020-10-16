#ifndef _RETINA_NET_INTERNAL_HPP_
#define _RETINA_NET_INTERNAL_HPP_

#include <vector>
#include <abi/consumer.hpp>
#include "face_info.hpp"

namespace glasssix::longinus
{
	struct anchor_win
	{
		float x_ctr;
		float y_ctr;
		float w;
		float h;
	};

	struct anchor_box
	{
		float x;
		float y;
		float h;
		float w;
	};

	struct face_pts
	{
		float x[5];
		float y[5];
	};

	struct face_info_internal
	{
		float clarity;
		float has_mask;
		float score;
		anchor_box rect;
		face_pts pts;
		float headpose[3];
		bool is_alive;
	};

	struct anchor_cfg
	{
	public:
		int STRIDE;
		std::vector<int> SCALES;
		int BASE_SIZE;
		std::vector<float> RATIOS;
		int ALLOWED_BORDER;

		anchor_cfg()
		{
			STRIDE = 0;
			SCALES.clear();
			BASE_SIZE = 0;
			RATIOS.clear();
			ALLOWED_BORDER = 0;
		}
	};

	class retina_net_internal
	{
	public:
		class impl;
		retina_net_internal() = delete;
		retina_net_internal(exposing::param_string phai_path, exposing::param_string racy_path, 
			exposing::param_string tracker_phai_path, exposing::param_string tracker_racy_path, 
			float nms_threshold = 0.4, int device = -1);
		retina_net_internal(const retina_net_internal&) = delete;
		retina_net_internal& operator=(const retina_net_internal&) = delete;
		~retina_net_internal();

		// Batch process have some advantage in inference but can't speed up preprocess and postprocess
		// TODO: implement
		//std::vector<std::vector<face_info>> detectBatchImages(std::vector<cv::Mat> imgs, float threshold = 0.5);
		//Test in GTX1060:
		// | model | speed | input size | preprocess time | inference | postprocess time |
		//	| :------ : | : ---- : | : -------- : | : ------------ - : | : ------ - : | : -------------- : |
		//	|  caffe | ????ms | 1920x1080 | ????ms | 61ms | ????ms      |
		//	|  caffe | ????ms | 1280£ø720 | ????ms | 44ms | ????ms      |
		//	|  caffe | 17.3ms | 640£ø480 | 3.9ms | 13.4ms | 1.0ms |
		exposing::param_vector<face_info> detect(exposing::param_span<std::uint8_t> &bitmap, int channels, int height, int width, int min_size = 16, float threshold = 0.5, int order = 0);
		
		face_info single_trace(face_info face, exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int order = 0);

		static std::string version();

	private:
		impl* impl_;
	};
}


#endif // _RETINA_FACE_HPP_
