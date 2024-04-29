#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"
#include "find_cluster_num.hpp"

#include <abi/param_vector.hpp>
#include <utility>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <iomanip>
#include <GenPipeline/GenPipeline.hpp>

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
    #include <RKNN2Wrapper/rknn2_wrapper.hpp>
#endif


namespace glasssix::crowd
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{ exposing::to_narrow_string(model_directory), device}
        {
        }

        impl( std::string model_directory, int device)
        {
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            std::vector<std::string> phai;
            net_segment0_ =  std::make_unique<rknnwrapper::rknn_wrapper> (phai,  std::string(model_directory) + std::string("/crowdcount_sim0.rknn"), device),
            net_segment1_ =  std::make_unique<rknnwrapper::rknn_wrapper> (phai,   std::string(model_directory) + std::string("/crowdcount_sim1.rknn"), device);
#elif defined(USE_BMNN)
            net_crowd_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/crowdcount_sim.bmodel", device);
            net_crowd_->manual_possible_normalization(std::array<float,3>{113.7f,104.4f,100.7f},std::array<float,3>{0.01360544, 0.014084507, 0.01383125864});
#endif
            
        }

        exposing::param_vector<crowd::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, int min_cluster_size, std::map<std::string, float>& param_map)
        {
            if (min_cluster_size <= 0)
                throw exposing::abi_invalid_argument("min_cluster_size < = 0");

            if (bitmap.empty())
                throw exposing::abi_invalid_argument("current frame is empty");
            
            int min_area_threshold = std::round(param_map.count("area_threshold") ? param_map["area_threshold"] : 30.f);

            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3);

            std::memcpy(image.data, bitmap.data(), sizeof (uint8_t) * channels * height * width);

            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
                  throw exposing::abi_invalid_argument("incorrect roi in crowd");

            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width));
            float pic_scale= cropped_image.cols>cropped_image.rows ?static_cast<float>(cropped_image.rows) /640.f :static_cast<float>(cropped_image.cols) /640.f   ; 

            cv::resize(cropped_image, cropped_image, cv::Size((int)(cropped_image.cols / pic_scale), (int)(cropped_image.rows / pic_scale)), cv::INTER_LINEAR);
            
            int cropwidth =  cropped_image.cols;
            int cropheight = cropped_image.rows;
            int xslice= (cropwidth+639)/640;
            int yslice= (cropheight+639)/640;
            int pad_h = yslice*640 - cropheight;
            int pad_w = xslice*640 - cropwidth;

            if(pad_h>0 || pad_w>0 )
                cv::copyMakeBorder(cropped_image, cropped_image, 0, pad_h, 0, pad_w, cv::BORDER_CONSTANT, cv::Scalar{ 0,0,0 });
            
            cropwidth=cropped_image.cols;
            cropheight = cropped_image.rows;

            std::vector<cluster_info> detection_points;
            for(int i=0;i< yslice;i++)
            {
                for (size_t j = 0; j < xslice; j++)
                {
                    int cropx1= j*640; 
                    int cropx2= (j+1)*640 > cropwidth ? cropwidth :  (j+1)*640 ;
                    int cropy1= i*640; 
                    int cropy2= (i+1)*640 > cropheight ? cropheight :(i+1)*640 ;

                    auto result = run_segment(cropped_image,cropx1,cropx2,cropy1,cropy2, min_area_threshold);
                    for( auto& it:result) 
                    {
                        it.x1*=pic_scale;
                        it.x2*=pic_scale;
                        it.y1*=pic_scale;
                        it.y2*=pic_scale;

                        it.x1+=roi_x;
                        it.x2+=roi_x;
                        it.y1+=roi_y;
                        it.y2+=roi_y;
                        detection_points.push_back(cluster_info{ .x1 = it.x1 ,.y1 = it.y1 ,.x2 = it.x2 ,
                        .y2 = it.y2 ,.x=(it.x1+ it.x2)*0.5,.y= (it.y1 + it.y2) * 0.5 });
                    }

                }
            }
            
            return find_cluster_num(detection_points, min_cluster_size);
        }

        std::string version()
        {
			const std::string algo_module_version = "2.1.4";

			std::string nn_frame_version = "dsd";

			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);

        }

    private:


        /**
         * @fun preprocess
         * @param src, new_shape
         * @return tensor(preprocess(image))
         * @details image preprocess and make tensor from images
         */
        struct detect_list
        {
            int x1;
            int y1;
            int x2;
            int y2;
            int category;
        };

        exposing::param_vector<crowd::box_info> find_cluster_num(const std::vector<cluster_info>& detection_points, int min_cluster_size)
        {
            auto results = exposing::make_param_vector<crowd::box_info>();
            if (detection_points.size() < 4)
                return results;
            cluster_num scaler;
            std::vector<int> cluster_num_ans = scaler.find_cluster_num(detection_points, min_cluster_size);

            //crowd::box_info_internal
            //results.push_back(glasssix::exposing::make_as_first<box_info_impl>(it));
            for ( int i = 0 ;i< detection_points.size();++i)
            {
                if (cluster_num_ans[i] == 0)
                    continue;
                crowd::box_info_internal temp;
                temp.x1= detection_points[i].x1;     
                temp.y1= detection_points[i].y1;     
                temp.x2= detection_points[i].x2;     
                temp.y2= detection_points[i].y2;     
                temp.category= cluster_num_ans[i]-1;
                results.push_back(glasssix::exposing::make_as_first<box_info_impl>(temp));
            }
            return results;
        }

        std::vector<detect_list> get_boxInfo_from_Binar_map(std::vector<int>& binar_map, int min_area=35)
        {
            std::vector<detect_list> result_part;
            int width = 640;
            int height = 640;
            cv::Mat grayImage(height, width, CV_8UC1);
            for (int i = 0; i < height; i++) 
                for (int j = 0; j < width; j++) 
                {
                    int index = i * width + j;
                    grayImage.at<uchar>(i, j) = static_cast<uchar>(binar_map[index]*255);
                }

            cv::Mat labeledImage;
            cv::Mat stats;
            cv::Mat centroids;

            int numLabels = cv::connectedComponentsWithStats(grayImage, labeledImage, stats, centroids,4);
            
            for (int i = 1; i < numLabels; ++i)  // 忽略背景标签 0
            {
                if( stats.at<int>(i, cv::CC_STAT_AREA) > min_area )
                {
                    detect_list box;
                    box.x1 = stats.at<int>(i, cv::CC_STAT_LEFT);
                    box.y1 = stats.at<int>(i, cv::CC_STAT_TOP);
                    box.x2 = stats.at<int>(i, cv::CC_STAT_LEFT) + stats.at<int>(i, cv::CC_STAT_WIDTH);
                    box.y2 = stats.at<int>(i, cv::CC_STAT_TOP) + stats.at<int>(i, cv::CC_STAT_HEIGHT);

                    result_part.push_back(box);
                }
            }
            return result_part;
        }


        std::vector<detect_list> post_process( const float* pred_map, const float* predict,int area_threshold, int size=640)
        {
            std::vector<int> binar_map (640*640);
            for (size_t i = 0; i < size*size; i++)
            {
                if(pred_map[i]>=predict[i]  )
                    binar_map[i]=1;
                else
                    binar_map[i]=0;
            }
            return get_boxInfo_from_Binar_map(binar_map, area_threshold);
        }

        
        std::vector<crowd::box_info_internal>  run_segment(cv::Mat& images, int cropx1,int cropx2,int cropy1,int cropy2, int area_threshold )
        {   
            std::vector<box_info_internal> output;
            cv::Rect rect {cropx1, cropy1, cropx2 - cropx1, cropy2 - cropy1};
            cv::Mat blobs = images(rect).clone();
            if(blobs.cols!=640||blobs.rows!=640 )
                throw exposing::abi_invalid_argument("img size error in crowd");

            cv::cvtColor(blobs, blobs, cv::COLOR_BGR2RGB);
            
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forward_result;
            auto  network_results = net_segment0_->forward(blobs.data, { 1, blobs.rows, blobs.cols,blobs.channels() }, RKNN_TENSOR_NHWC);
            forward_result.push_back(network_results["input.2292"]);
            forward_result.push_back(network_results["pred_map"]);
            std::vector<int> shape1 = {1, 720, 80, 80};
            auto Mul_268 = std::make_shared<glasssix::memory::tensor<float>>(shape1, -1, glasssix::memory::NCHW);
            std::vector<int> shape2 = {1, 1, 80, 80};
            auto Mul_263 = std::make_shared<glasssix::memory::tensor<float>>(shape2, -1, glasssix::memory::NCHW);
            auto input_188= std::make_shared<glasssix::memory::tensor<float>>(shape1, -1, glasssix::memory::NCHW);

            resize_nearst(forward_result[0]->cpu_data(),Mul_268->mutable_cpu_data(), 160, 160, 80, 80, 720   );
            resize_nearst(forward_result[1]->cpu_data(),Mul_263->mutable_cpu_data(), 640, 640, 80, 80, 1   );
            Mul_77(Mul_268->cpu_data(), Mul_263->cpu_data(), input_188->mutable_cpu_data(), 80*80,720   );

            std::vector<float> input_data( 720*80*80);

            nchw2Nhwc(input_188->mutable_cpu_data(),input_data.data(),1,720,80,80 );
            auto  network_result1 = net_segment1_->forward(input_data.data(), { 1, 80, 80,720 }, RKNN_TENSOR_NHWC);

            /**
            * @fun forward part end
            * @param  none
            * @return tensor(preprocess(image))
            */

            const float *predict = network_result1["predict"]->cpu_data();
            const float* pred_map = forward_result[1]->cpu_data();


#elif defined(USE_BMNN)
            auto  net_crowd__result = net_crowd_->forward(blobs);
            const float *predict = net_crowd__result["predict_Resize_f32"]->cpu_data();
            const float* pred_map = net_crowd__result["pred_map_Sigmoid"]->cpu_data();
#endif
            auto result_part = post_process( pred_map, predict,area_threshold);

            for (auto &iter: result_part)
            {       
                box_info_internal  headp;
                headp.x1 = iter.x1+cropx1;
                headp.x2 = iter.x2+cropx1;
                headp.y1 =iter.y1+cropy1;
                headp.y2 =iter.y2+cropy1;
                output.push_back(headp);
            }
            return output;
        }


    private:
        std::string model_directory_;
        int device_;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            std::unique_ptr<glasssix::rknnwrapper::rknn_wrapper> net_segment0_;
            std::unique_ptr<glasssix::rknnwrapper::rknn_wrapper> net_segment1_;

#elif defined(USE_BMNN)
            std::shared_ptr<GenPipeline> net_crowd_;
#endif
        

    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;

    std::string detect_code_internal::version()
    {
        return impl_->version();
    }

    exposing::param_vector<crowd::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, int min_cluster_size, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, min_cluster_size, param_map);
    }
}
