#include "pumptop_helmet_detector_impl.hpp"
#include "pumptop_helmet_info_impl.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#include <GenPipeline/GenPipeline.hpp>
#include <YoloFamily/Yolo_wrapper.hpp>
#include "poly.hpp"
#include <thread>
#include <chrono>

// #define RECTANGLE
namespace glasssix::pumptop_helmet
{
	class pumptop_helmet_detector_impl::impl
	{
	public:
		impl() = delete;

		impl(std::string_view model_directory, int device)
			: model_directory_{std::string(model_directory)}, device_{device}
		{
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			std::string model_ext{ ".rknn" };
#elif defined(USE_BMNN)
			std::string model_ext{ ".bmodel" };
#else
			std::string model_ext{ ".onnx" };
#endif
			// 算法传过来的模型名:泵检测模型 1280-v1_ori_TAL
			net_detect_1 = std::make_shared<GenPipeline>(model_directory_ + "/pumptop_helmet_pump" + model_ext, device_);
			yolov8_instance_1 = std::make_shared<Yolov8<GenPipeline>>(1280, 1280, net_detect_1);

			// 算法传过来的模型名:人检测模型 1280T320-0108_Person_best_detection
			net_detect_2 = std::make_shared<GenPipeline>(model_directory_ + "/pumptop_helmet_person" + model_ext, device_);
			yolov8_instance_2 = std::make_shared<Yolov8<GenPipeline>>(1280, 736, net_detect_2);

			// 算法传过来的模型名:人头检测模型 640T320-200epft-baoshinegtivev2-atss-nwd-wop
			net_detect_3 = std::make_shared<GenPipeline>(model_directory_ + "/pumptop_helmet_head" + model_ext, device_);
			//yolov8_instance_3 = std::make_shared<Yolov8<GenPipeline>>(128, 128, net_detect_3);
			// 算法传过来的模型名:人头分类检测模型 helmetclassify-v2-96-labelsmooth-0.05
			net_detect_4 = std::make_shared<GenPipeline>(model_directory_ + "/pumptop_helmet_helmet" + model_ext, device_);

			net_detect_1->manual_possible_normalization(0, 1.f / 255);
			net_detect_2->manual_possible_normalization(0, 1.f / 255);
			net_detect_3->manual_possible_normalization(0, 1.f / 255);
			net_detect_4->manual_possible_normalization(0, 1.f / 255);
		}

		~impl()
		{
		}

		static inline float sigmoid_x(float x)
		{
			return static_cast<float>(1.f / (1.f + exp(-x)));
		}

		void tranpose(const float *sou, float *dest, int sourows, int soucols)
		{
			for (int i = 0; i < sourows; i++)
				for (int j = 0; j < soucols; j++)
					dest[j * sourows + i] = sou[i * soucols + j];
		}

		void Softmax(float *data, int num)
		{
			double L2_Sum = 0.f;
			for (size_t i = 0; i < num; i++)
			{
				data[i] = (exp(data[i]));
				L2_Sum += data[i];
			}
			for (size_t i = 0; i < num; i++)
				data[i] = data[i] / L2_Sum;
		}

		inline float de_sigmoid(float x)
		{
			if (x >= 1 || x < 0)
				return NAN;
			return static_cast<float>(log(x / (1 - x)));
		}

		int num = 0;
		exposing::param_vector<pumptop_helmet_info> detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::map<std::string, float> &param_map)
		{
			std::vector<int> categorys;
			std::vector<float> scores;
			std::vector<float> helmet_scores;
			// CHECK_EQ(channels, 24);
			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}
			auto results = exposing::make_param_vector<pumptop_helmet::pumptop_helmet_info>();

			return results;
		}


		exposing::param_string version() const
		{
			return "1.0.6";
		}

	private:
		std::shared_ptr<GenPipeline> net_detect_1;
		std::shared_ptr<GenPipeline> net_detect_2;
		std::shared_ptr<GenPipeline> net_detect_3;
		std::shared_ptr<GenPipeline> net_detect_4;
		std::shared_ptr<Yolov8<GenPipeline, false>> yolov8_instance_1;
		std::shared_ptr<Yolov8<GenPipeline, false>> yolov8_instance_2;

		std::vector<std::string> phais;

		std::string model_directory_;
		int device_;

		std::vector<int> posture_add_weight_1280;
		std::vector<int> posture_mul_weight_1280;
		std::vector<int> data;
	};

	pumptop_helmet_detector_impl::pumptop_helmet_detector_impl()
	{
	}

	pumptop_helmet_detector_impl::~pumptop_helmet_detector_impl()
	{
	}
	void pumptop_helmet_detector_impl::init(const exposing::param_string &models_directory, std::int32_t device)
	{
		impl_ = std::make_unique<impl>(models_directory, device);
	}

	exposing::param_string pumptop_helmet_detector_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_vector<pumptop_helmet_info> pumptop_helmet_detector_impl::detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, const exposing::param_hash_map<exposing::param_string, float> &param_map_abi) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"pumptop_helmet_detector_internal object not initialized");
		std::map<std::string, float> param_map;
		for (auto it : param_map_abi)
		{
			param_map.insert(std::make_pair(it.key(), it.value()));
		}
		return impl_->detect(bitmap, channels, height, width, param_map);
	}
}
