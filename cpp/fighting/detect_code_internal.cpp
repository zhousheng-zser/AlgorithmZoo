#include "detect_code_internal.hpp"
#include <opencv2/opencv.hpp>
#include "box_info_impl.hpp"
#include <GenPipeline/PrePostProcessGenPipeline.hpp>
#include "../genpipeline/market/yolov8_GEN.hpp"
#include "combine_related_box.hpp"
#include <unistd.h>  
#ifdef BUILD_DEBUG_INFO
//#define GetShowRatio(visual_img) std::min(float(1920.f / visual_img.cols), float(1080.f / visual_img.rows)) * 0.75
//#define ShowResize(visual_img, showRatio) cv::resize(visual_img, visual_img, cv::Size(), showRatio, showRatio)f
//#define ImgShow(visual_img) cv::imshow("visual_img", visual_img);cv::waitKey(0)
//#define AdpShow(img) {auto visual_img=img.clone();ShowResize(visual_img,GetShowRatio(visual_img));ImgShow(visual_img);}
#endif

namespace glasssix::fighting
{
	class detect_code_internal::impl
	{
	public:
		impl(std::string_view model_directory, int device, int batch) :BATCH_(batch)
		{
			std::string model_directory_ = exposing::to_narrow_string(model_directory);
			if (*model_directory_.rbegin() != '/') model_directory_ += '/';
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			std::string model_ext(".rknn");
#elif defined(USE_BMNN)
			std::string model_ext(".bmodel");
#else
			std::string model_ext(".onnx");
#endif
			person_det_= PrePostProcessGenPipeline::mkSharePipeline(model_directory_ + "fight_pedestrian" + model_ext, 0);
			person_det_->manual_possible_normalization(0, 1.f / 255);
			person_det_->set_postprocessing(yolov8_GEN<1, 1>);

			if (BATCH_ == 10) {
				nonm_instance_ = PrePostProcessGenPipeline::mkSharePipeline(model_directory_ + "fight_10b.nnm" + model_ext, 0); // not normalization if rknn
				/// Version 3.0.0 and before
				FIGHT_INFER_H_ = 256;
				FIGHT_INFER_W_ = 460;
			}
			else if (BATCH_ == 8) {
				nonm_instance_ = PrePostProcessGenPipeline::mkSharePipeline(model_directory_ + "fight_8b.nnm" + model_ext, 0);// not normalization if rknn
				FIGHT_INFER_H_ = 256;
				FIGHT_INFER_W_ = 460;
			}
			else
				throw exposing::abi_invalid_argument("fighting incorrect BATCH_ param");

			std::array<float, 3> means_v{ 123.675, 116.28, 103.53 };
			std::array<float, 3> stand_v{ 58.395, 57.12, 57.375 };

			std::vector<cv::Mat> std_channels;
			std::vector<cv::Mat> mean_channels;
			for (int i = 0; i < 3; i++) {
				/* Extract an individual channel. */
				cv::Mat std_channel(FIGHT_INFER_H_, FIGHT_INFER_W_, CV_32FC1, cv::Scalar(1.f / stand_v[i]));
				std_channels.push_back(std_channel);
				cv::Mat mean_channel(FIGHT_INFER_H_, FIGHT_INFER_W_, CV_32FC1, cv::Scalar(-means_v[i]));
				mean_channels.push_back(mean_channel);
			}
			cv::merge(std_channels, m_std);
			cv::merge(mean_channels, m_mean);

			f32ImgsArr.resize(BATCH_ * 3 * FIGHT_INFER_H_ * FIGHT_INFER_W_);
		}

		exposing::param_vector<fighting::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map_std)
		{
			size_t bitmap_size = bitmap.size();
			CHECK_EQ(bitmap.size(), BATCH_ * height * width * 3);
			if (roi_x<0 || roi_x>width || roi_y > height || roi_y < 0 || roi_height<0 || (roi_height + roi_y) >height || roi_width<0 || (roi_width + roi_x) > width)
			{
				throw exposing::abi_invalid_argument("incorrect roi in fighting");
			}

			const float fight_thres = param_map_std.count("fight_thres") ? param_map_std["fight_thres"] : 0.5f;
			const float person_conf_thres = param_map_std.count("person_conf_thres") ? param_map_std["person_conf_thres"] : 0.3f;
			const float person_nms_thres = 0.6f;


			std::vector<cv::Mat> batchImages;
			for (int i = 0; i < BATCH_; i++) {
				cv::Mat InteImage(cv::Size(width, height), CV_8UC3, bitmap.data() + i * 3 * height * width);
				/// person det will letterbox (resize), clone cropped unnecessary
				/// fight_detect will resize, clone cropped unnecessary
				cv::Mat cropped_image = InteImage(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width)).clone();
				batchImages.push_back(cropped_image);
			}

			// Pedestrian detect
			std::vector<cv::Rect> person_box_list;
if(BATCH_==8)
{
			 person_box_list = get_person_box_by_detect_result_list(batchImages, height, width, {0,4,7}, person_conf_thres, person_nms_thres);
}
else
{	
			person_box_list = get_person_box_by_detect_result_list(batchImages, height, width, {0,4,9}, person_conf_thres, person_nms_thres);
}
			// Fighting detect
			auto result = exposing::make_param_vector<fighting::box_info>();
			for (auto gang_rect : person_box_list) {
				std::vector<cv::Mat> may_ft_region_batch_images;
				int index=0;
				for (auto& frame : batchImages) {
					auto sub_region = GenPipeTools::safty_cut(frame, gang_rect);

					cv::resize(sub_region, sub_region, cv::Size2i{ FIGHT_INFER_W_, FIGHT_INFER_H_ });
					// Can cvtColor to split operation?
					// std::cout<<sub_region.cols<<" "<<sub_region.rows<<std::endl;
					// sleep(1);
					// cv::imwrite(std::to_string(index) + "sub_region.jpg", sub_region );
					index++;
					cv::cvtColor(sub_region, sub_region, cv::COLOR_BGR2RGB);
					// cv::imwrite(std::to_string(index) + "ssub_region.jpg", sub_region );
					may_ft_region_batch_images.emplace_back(sub_region);
				}

				// for (size_t i = 0; i < 10; i++)
				// {
				// 	cv::Mat testimg = cv::imread(std::to_string(i)+".jpg");
				// 	cv::resize(testimg, testimg, cv::Size2i{ FIGHT_INFER_W_, FIGHT_INFER_H_ });
				// 	cv::cvtColor(testimg, testimg, cv::COLOR_BGR2RGB);
				// 	// may_ft_region_batch_images.emplace_back(testimg);
				// }

				float score = fight_detect_10B_handnormalization(may_ft_region_batch_images);
				// std::cout<<"dsdsd2\n";
				BoxInfoInternal fightdet_box(gang_rect, score, fight_thres);
				fightdet_box.add(roi_x, roi_y);

				result.push_back(exposing::make_as_first<box_info_impl>(fightdet_box));
			}
			return result;
		}

		std::vector<cv::Rect> get_person_box_by_detect_result_list(
			std::vector<cv::Mat> batchImages,
			int height,
			int width,
			std::vector<int> sample_frame_idset,
			const float person_conf_thres,
			const float person_nms_thres)
		{
			std::vector<cv::Rect> person_box_list;
			// person_box_list.reserve(2 * BATCH_);

#ifdef BUILD_DEBUG_INFO
			//auto Vis = batchImages[0].clone();
#endif // BUILD_DEBUG_INFO

			const float infer_ratio = FIGHT_INFER_W_ * 1.f / FIGHT_INFER_H_;
			for (int f_id : sample_frame_idset) {
				cv::Mat InteImageStp = batchImages[f_id];

#ifdef BUILD_DEBUG_INFO
				auto VisFrame = InteImageStp.clone();
#endif // BUILD_DEBUG_INFO
				auto Vis = batchImages[f_id].clone();
				auto frame_persons = person_detect(f_id, InteImageStp, person_conf_thres, person_nms_thres);

				float model_input_scale_rate =460.f/256.f;
				std::vector<cv::Rect> combined_box_list;
				for (auto& frame_person : frame_persons) {
					if (frame_person.xmax > frame_person.xmin && frame_person.ymax > frame_person.ymin) 
					{
						float w = frame_person.xmax - frame_person.xmin;
						float h = frame_person.ymax - frame_person.ymin;

						if( std::max(w,h)< 120.0  )
							continue;
						w = int(w * 5.0);
						h = int(h * 1.3);

						frame_person.reset_w_h_kepCenter(w, h);
						frame_person.constraintRectBoundary(width, height);  //修改问题
						combined_box_list.emplace_back(frame_person.get_rect());

#ifdef BUILD_DEBUG_INFO
						cv::rectangle(VisFrame, frame_person.get_rect(), { 0,255,0 }, 2);
#endif // BUILD_DEBUG_INFO
					}
				}
						combine_related_box(combined_box_list, 0.00001f);
						// for (auto  x :combined_box_list)
								// cv::rectangle(Vis, x, { 0,100,255 }, 2);
						
						if(person_box_list.size()==0) 						
							person_box_list = combined_box_list;
						else
						{
							std::vector<cv::Rect> new_person_box_list;
							for (auto person_box : person_box_list)
							{
								for (auto it = combined_box_list.begin(); it != combined_box_list.end();)
								{
									// std::cout<<"inner"<<
									if (count_iou_(person_box, *it) > 0.001)
									{
										auto outer_box = get_outer_box_(person_box, *it);
										int cx = (outer_box.br().x + outer_box.tl().x) / 2;
										int cy = (outer_box.br().y + outer_box.tl().y) / 2;
										int cw = outer_box.br().x - outer_box.tl().x;
										int ch = outer_box.br().y - outer_box.tl().y;
										PersonBBox temp_frame_person;
										temp_frame_person.xmin = outer_box.tl().x;
										temp_frame_person.ymin = outer_box.tl().y;
										temp_frame_person.xmax = outer_box.br().x;
										temp_frame_person.ymax = outer_box.br().y;
										if (float(cw) / float(ch) < model_input_scale_rate)
											cw = int(ch * model_input_scale_rate);
										else
											ch = int(cw / model_input_scale_rate);

										temp_frame_person.reset_w_h_kepCenter(cw, ch);
										temp_frame_person.constraintRectBoundary(width, height); 
										new_person_box_list.push_back(temp_frame_person.get_rect());

										// 擦除当前的 combined_box
										it = combined_box_list.erase(it);
										break;
									}
									else
									{
										++it;
									}
								}
							}
							// person_box_list = new_person_box_list;
							if(new_person_box_list.size())
								return new_person_box_list;
							person_box_list.resize(0);
							// std::cout<<"person_box_list: "<<person_box_list.size()<<std::endl;
						}
				// for (auto  x :person_box_list)
				// 	cv::rectangle(Vis, x, { 255,0, 0 }, 2);

				// sleep(1);
				// cv::imwrite("Vis.jpg", Vis );
#ifdef BUILD_DEBUG_INFO
				//AdpShow(VisFrame);
#endif // BUILD_DEBUG_INFO
			}

			// combine_related_box(person_box_list, 0.1);

#ifdef BUILD_DEBUG_INFO
			////std::cout << "# combine_related_box size = " << person_box_list.size() << std::endl;			
			//for (auto& combineBox : person_box_list) {
			//	cv::rectangle(Vis, combineBox, { 255,255,0 }, 2);			
			//}
			//AdpShow(Vis);
#endif // BUILD_DEBUG_INFO
			person_box_list.resize(0);
			return person_box_list;
		}

		std::vector<PersonBBox> person_detect(int frame_id, cv::Mat& image,float con_thres, float iou_thres) {
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			const int letter_h = 384;
			const int letter_w = 640;
#elif defined(USE_BMNN)
			const int letter_h = 480;
			const int letter_w = 800;
#endif

			GenPipTools::LetterInfo letter_op;
			auto letter_img = GenPipTools::letter_image(image, letter_w, letter_h, letter_op, true);
			//person_det_->profiler_tic();
			auto tensor_out = person_det_->forward(letter_img).begin()->second;
			//person_det_->profiler_toc();
			const int vf_nums = tensor_out->height(); //vf, visual field
			const int per_vf_len = tensor_out->width();
			std::vector<PersonBBox> box_list;
			for (size_t idx = 0; idx < vf_nums; idx++) {
				float* pdata = tensor_out->mutable_cpu_data() + idx * per_vf_len;
				float conf = pdata[4];
				if (conf > con_thres) {
					PersonBBox obj_box(pdata[0] * letter_w, pdata[1] * letter_h, pdata[2] * letter_w, pdata[3] * letter_h, conf, 0);
					obj_box.frame_id = frame_id;
					box_list.push_back(obj_box);
				}
			}
			GenPipTools::nms_cpu(box_list, iou_thres);
			GenPipTools::letter_map_origin_location(box_list, letter_op);

			return box_list;
		}

		float fight_detect_10B_handnormalization(const std::vector<cv::Mat>& BatchImgs) {
			static constexpr int channel_ = 3; //RGB
			const int HWStep = FIGHT_INFER_H_ * FIGHT_INFER_W_;

			/// Subsequent operations will not allocate new memory 
			/// but implicitly manipulate the memory space of f32ImgsArr.data 
			/// by using cv::Mat in an referencing external memory mode."
			for (int imgIdx = 0; imgIdx < BATCH_; imgIdx++) {
				cv::Mat sample_float(FIGHT_INFER_H_, FIGHT_INFER_W_, CV_32FC3);
				BatchImgs[imgIdx].convertTo(sample_float, CV_32FC3);
				cv::add(sample_float, m_mean, sample_float);	 // sample_float += m_mean
				cv::multiply(sample_float, m_std, sample_float); // sample_float *= m_std

				auto f32ImgsArr_img = f32ImgsArr.data() + imgIdx * 3 * HWStep;
				cv::Mat input_c0(FIGHT_INFER_H_, FIGHT_INFER_W_, CV_32FC1, f32ImgsArr_img + 0 * HWStep);
				cv::Mat input_c1(FIGHT_INFER_H_, FIGHT_INFER_W_, CV_32FC1, f32ImgsArr_img + 1 * HWStep);
				cv::Mat input_c2(FIGHT_INFER_H_, FIGHT_INFER_W_, CV_32FC1, f32ImgsArr_img + 2 * HWStep);
				std::array<cv::Mat, 3> input_channels{ input_c0, input_c1, input_c2 };
				cv::split(sample_float, input_channels); //eq split to f32ImgsArr
			}

			//nonm_instance_->profiler_tic();
#ifdef USE_RKNN2API // rknn net only accept HWC, a trouble maker
			/* NOTE: Transposing will allocate new memory for the result. No changed f32ImgsArr.data()`s internal values */
			cv::Mat f32ImgsArr_TS_HW_C(BATCH_ * 3, HWStep, CV_32FC1, f32ImgsArr.data());
			cv::transpose(f32ImgsArr_TS_HW_C, f32ImgsArr_TS_HW_C); //{30, 256, 460} -> {256, 460, 30} 30:10*3
			auto ts_f32_rst_map = nonm_instance_->forward((float*)f32ImgsArr_TS_HW_C.data, { 1, FIGHT_INFER_H_, FIGHT_INFER_W_, BATCH_ * 3 }, 1); //RKNN_TENSOR_NHWC is 1
#else
			auto ts_f32_rst_map = nonm_instance_->forward(f32ImgsArr.data(), { 1, BATCH_ * 3, FIGHT_INFER_H_, FIGHT_INFER_W_ }, 0);
#endif
			//nonm_instance_->profiler_toc();
			auto det_scores = ts_f32_rst_map.begin()->second->cpu_data();
			return det_scores[0];
		}

		std::string version()
		{
			const std::string algo_module_version = "3.2.1";
			std::string nn_frame_version = nonm_instance_->version();
			return fmt::format(R"({ {"nn_frame_version":"{}", "algo_module_version" : "{}"} })", nn_frame_version, algo_module_version);
		}

	private:
		std::shared_ptr<PrePostProcessGenPipeline> person_det_;
		std::shared_ptr<PrePostProcessGenPipeline> nonm_instance_;
		int BATCH_;
		int FIGHT_INFER_H_;
		int FIGHT_INFER_W_;
		cv::Mat m_mean;
		cv::Mat m_std;
		std::vector<float> f32ImgsArr;
	};

	detect_code_internal::detect_code_internal(std::string_view model_directory, int device, int BATCH_)
		: impl_{ std::make_unique<impl>(model_directory, device, BATCH_) }
	{
	}

	detect_code_internal::~detect_code_internal()
	{
	}

	exposing::param_vector<fighting::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map_std)
	{
		return impl_->detect(bitmap, height, width, roi_x, roi_y, roi_width, roi_height, param_map_std);
	}

	std::string detect_code_internal::version()
	{
		return impl_->version();
	}
}
