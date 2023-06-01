#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <utility>

#include <RKNN2Wrapper/rknn2_wrapper.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>


namespace glasssix::flame
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{hardcode::get_model_params("flame", false),  exposing::to_narrow_string(model_directory), device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device)
                :net_instance_(phai,  model_directory + std::string("/flame_sim.rknn"), device)
        {

        }

        exposing::param_vector<flame::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            
            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof (uint8_t) * channels * height * width);
            
            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
            {
                  throw exposing::abi_invalid_argument("incorrect roi in flame");
            }

            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width));

            auto result = run_detect(cropped_image, roi_x, roi_y, roi_width, roi_height, param_map);

            auto results = exposing::make_param_vector<flame::box_info>();

            for(auto& it:result) {
                it.x1+=roi_x;
                it.x2+=roi_x;
                it.y1+=roi_y;
                it.y2+=roi_y;
                results.push_back(glasssix::exposing::make_as_first<box_info_impl>(it));
            }

            return results;
        }

        static std::string version()
        {
            return "1.0.0";
        }

    private:

        /**  @fun letterbox
         *   @param image scaleFill
         *   @return letterbox(image)
         *   @details Resize and pad image while meeting stride-multiple constrain
         */
        static std::pair<cv::Mat, float> letterbox(cv::Mat& img, cv::Size new_shape)
        {
            int H = img.rows;
            int W = img.cols;
            float ratio_w = (float)W / (float)new_shape.width;
            float ratio_h = (float)H / (float)new_shape.height;
            float ratio = ratio_w;

            cv::Mat resize_img;
            if(H==new_shape.height && W==new_shape.width)
            {
                resize_img=img;
            }
            else
            {
                if (ratio_w == ratio_h)
                {
                    cv::resize(img, resize_img, cv::Size2i{ new_shape.width, new_shape.height });}
                else if (ratio_w > ratio_h)
                {

                    int new_x = new_shape.width;
                    int new_y = (int)(H / ratio_w);
                    int pad1 = (int)((new_shape.height - new_y) / 2);
                    int pad2 = new_shape.height - new_y - pad1;
                    cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
                    cv::copyMakeBorder(resize_img, resize_img, 0, pad1 + pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
                }
                else
                {
                    ratio = ratio_h;
                    int new_y = new_shape.height;
                    int new_x = (int)(W / ratio_h);
                    int pad1 = (int)((new_shape.width - new_x) / 2);
                    int pad2 = new_shape.width - new_x - pad1;
                    cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
                    cv::copyMakeBorder(resize_img, resize_img, 0, 0, 0, pad1 + pad2, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
                }
            }

            return { resize_img, ratio };
        }

        /**
         * @fun preprocess
         * @param src, new_shape
         * @return tensor(preprocess(image))
         * @details image preprocess and make tensor from images
         */
     std::tuple<cv::Mat, float> preprocess(cv::Mat src, cv::Size& new_shape)
        {
            cv::Mat image;
            float ratio;
            std::tie(image, ratio) = letterbox(src, new_shape);
            cv::Mat image_blob;
            cv::cvtColor(image, image_blob, cv::COLOR_BGR2HLS);
            return {image_blob, ratio};

        }
       
		/**
		 * @fun sigmoid_x
		 * @param x
		 * @return sigmoid(x)
		 */
		static inline float sigmoid_x(float x)
		{
			return static_cast<float>(1.f / (1.f + exp(-x)));
		}

		/**
		 * @fun concat
		 * @param infer_out, conf_thres
		 * @return source
		 * @details concat 3 into 1
		 */
        static std::vector<std::vector<float>> concat2(std::vector<std::shared_ptr<memory::tensor<float>>>& outs, float conf_thres)
        {

            const float anchors[3][6] = { {30,61, 62,45, 59,119},{116,90, 156,198, 373,326},{10,13, 16,30, 33,23}};
            const int stride[3] = { 40, 20, 80};
            const float strides[3] = { 16.0, 32.0, 8.0};
            std::vector<std::vector<float>> result;
            for(int n = 0; n < 3; n++)
            {
                int num_grid_x = stride[n];
                int num_grid_y = stride[n];

                int ind = 0;
                const float *ptr_out=outs[n]->cpu_data();
                for(int q = 0; q < 3; q++)
                {
                    const float anchor_w = anchors[n][q * 2];
                    const float anchor_h = anchors[n][q * 2 + 1];
                    for(int i = 0; i < num_grid_x; i++)
                    {
                        for(int j = 0; j < num_grid_y; j++)
                        {

                            const float* pdata = ptr_out + ind *  7;

                            float box_score = sigmoid_x(pdata[4]);

                            if(box_score > conf_thres)
                            {

                                float cx = (sigmoid_x(pdata[0]) * 2.f - 0.5f + j) * strides[n];  //cx
                                float cy = (sigmoid_x(pdata[1]) * 2.f - 0.5f + i) * strides[n];  //cy
                                float w = powf(sigmoid_x(pdata[2]) * 2.f, 2.f) * anchor_w;      //w
                                float h = powf(sigmoid_x(pdata[3]) * 2.f, 2.f) * anchor_h;      //h
                                std::vector<float> element = {cx, cy, w, h, box_score, sigmoid_x(pdata[5]), sigmoid_x(pdata[6])};
                                result.push_back(element);
                            }
                            ind++;
                        }
                    }
                }
            }
            return result;
        }

		/**
		 * @fun computeNx6
		 * @param anchor, conf_thres
		 * @return [box,confidence,category]
		 * @details concat xywh into nx6
		 */
		 struct boxes_conf
		 {
			 float top_x;
			 float top_y;
			 float bot_x;
			 float bot_y;
			 float conf;
             int category;
		 };

		static std::vector<boxes_conf> computeNx6(std::vector<std::vector<float>>& src, float conf_thres)
		{
			std::vector<boxes_conf> res;
			for(auto it: src)
			{
				float top_x = it[0] - it[2] / 2;
				float top_y = it[1] - it[3] / 2;
				float bot_x = it[0] + it[2] / 2;
				float bot_y = it[1] + it[3] / 2;
				float conf  = it[4];
				int maxPosition = std::max_element(it.begin()+5, it.end()) - it.begin();
				if(it[maxPosition] * conf > conf_thres)
				{
					boxes_conf temp{};
					temp.top_x = top_x;
					temp.top_y = top_y;
					temp.bot_x = bot_x;
					temp.bot_y = bot_y;
					temp.conf = conf;
                    temp.category =  maxPosition - 5;
					res.push_back(temp);
				}
			}
			return res;
		}

		/**
		 * @fun computNmsInput
		 * @param src, max_wh
		 * @return std::pair<bboxes, confidence>
		 * @details slice src into bboxes and confidence, which need by dnn::NMS
		 */
		static std::tuple<std::vector<cv::Rect2d>, std::vector<float>, std::vector<int>> computeNmsInput(std::vector<boxes_conf>& src, int max_wh,float ratio)
		{
			std::vector<cv::Rect2d> boxes;
			std::vector<float> scores;
            std::vector<int> category;
			for(auto const &it: src)
			{
				int c = max_wh * it.conf;
				cv::Rect2d temp;
		        temp.x      = static_cast<double>(it.top_x )*ratio;
				temp.y      = static_cast<double>(it.top_y )*ratio;
				temp.width  = static_cast<double>(it.bot_x - it.top_x);
				temp.height = static_cast<double>(it.bot_y - it.top_y);
				boxes.push_back(temp);
				scores.push_back(it.conf);
                category.push_back(it.category);
			}
			return std::make_tuple(boxes, scores, category);
		}

		/**
		 * @fun non_max_suppression
		 * @param prediction, conf_thres, iou_thres
		 * @return std::vector(boxes, classes)
		 * @details Non-Maximum Suppression (NMS) on inference results
		 */
		static std::vector<std::tuple<cv::Point, cv::Point, int>> non_max_suppression(std::vector<std::vector<float>>& prediction, float conf_thres, float iou_thres, float ratio)
		{
			auto compute_box = computeNx6(prediction, conf_thres);

			// Batched NMS
			int max_wh = 4096;
            std::vector<cv::Rect2d> bboxes;
            std::vector<float> scores;
            std::vector<int> classes;
          
            std::tie(bboxes, scores, classes) = computeNmsInput(compute_box, max_wh,ratio);
			// Perform non-maximum suppression to eliminate redundant overlapping boxes with
			// lower confidences
			std::vector<int> indices;

            std::vector<int> indices_smoke;
            std::vector<int> indices_fire;

            std::vector<cv::Rect2d> bboxes_smoke;
            std::vector<cv::Rect2d> bboxes_fire;
            std::vector<int> mapping_smoke;
            std::vector<int> mapping_fire;
            std::vector<float> scores_smoke;
            std::vector<float> scores_fire;
            for(int i=0;i<bboxes.size(); i++)
            {
                if(classes[i]==1)
                {
                    bboxes_smoke.push_back(bboxes[i]);
                    mapping_smoke.push_back(i);
                    scores_smoke.push_back(scores[i]);
                }
                else
                {
                    bboxes_fire.push_back(bboxes[i]);
                    mapping_fire.push_back(i);
                    scores_fire.push_back(scores[i]);
                }
            }

            cv::dnn::NMSBoxes(bboxes_smoke, scores_smoke, conf_thres, iou_thres, indices_smoke, 1.f, 3);

			cv::dnn::NMSBoxes(bboxes_fire, scores_fire, conf_thres, iou_thres, indices_fire, 1.f, 3);

            for(int i=0;i<indices_smoke.size();i++)
            {
                indices.push_back(mapping_smoke[ indices_smoke[i] ]  );
            }

            for(int i=0;i<indices_fire.size();i++)
            {
                indices.push_back(mapping_fire[ indices_fire[i] ]  );
            }

			std::vector<std::tuple<cv::Point, cv::Point, int>> output;

			auto f = [](int x){if(x<0) return 0; else return x;};

			for (int idx : indices)
			{
				auto box1 = cv::Point(f(static_cast<int>(compute_box[idx].top_x * ratio)), f(static_cast<int>(compute_box[idx].top_y * ratio)));
				auto box2 = cv::Point(f(static_cast<int>(compute_box[idx].bot_x * ratio)), f(static_cast<int>(compute_box[idx].bot_y * ratio)));
				output.emplace_back(std::make_tuple(box1, box2, compute_box[idx].category));
			}
			
			return output;
		}

        /**
           * @fun run_detect
           * @param image param_map
           * @return std::vector<flame::box_info_internal>
           * @details run detect (maybe in multithreading)
        */
        std::vector<flame::box_info_internal> run_detect(cv::Mat& image, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            std::map<std::string, float> params = {
                    {"conf_thres", param_map.count("conf_thres") ? param_map["conf_thres"] : 0.1f},
                    {"iou_thres",  param_map.count("iou_thres") ? param_map["iou_thres"] : 0.45f}};
			
			auto old_shape = cv::Size(roi_width, roi_height);

			auto new_shape = cv::Size(640,  640);

            cv::Mat blob;
            float ratio = 0;

            std::tie(blob, ratio) = preprocess(image,new_shape);
           
            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;

            auto  network_result = net_instance_.forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);

            std::vector<std::string>  out_names={"417","437","output"};


            for (size_t i=0;i< 3; i++)//对输出数据做处理
            {
                forwards.push_back(network_result[out_names[i]]);
            }

			float conf_threshold = 0.1f;
			float iou_threshold = 0.45f;

			auto result = concat2(forwards, conf_threshold);

			auto nms_result = non_max_suppression(result, conf_threshold, iou_threshold, ratio);

            std::vector<box_info_internal> output;

            for(auto const &it: nms_result)
            {
                box_info_internal temp;
                temp.x1 = std::get<0>(it).x;
                temp.y1 = std::get<0>(it).y;
                temp.x2 = std::get<1>(it).x;
                temp.y2 = std::get<1>(it).y;
                temp.category = std::get<2>(it);
                output.push_back(temp);
            }

            return output;
        }


    private:
        std::string model_directory_;
        int device_;
        rknnwrapper::rknn_wrapper net_instance_;

    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;

    std::string detect_code_internal::version()
    {
        return impl::version();
    }

    exposing::param_vector<flame::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}
