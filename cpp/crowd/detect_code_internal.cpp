#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"
#include "find_cluster_num.hpp"

#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <utility>

#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <tuple>

#include "Excalibur/pipeline.hpp"
#include "Excalibur/operation_make_border.hpp"
#include "Excalibur/operation_safty_cut.hpp"
#include "Primitives/tensor_conversions.hpp"
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
    #include <RKNN2Wrapper/rknn2_wrapper.hpp>
#endif
#include <iomanip>
#include <tuple>

namespace glasssix::crowd
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{get_model_params("crowd", false),  exposing::to_narrow_string(model_directory), device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device)
                :net_segment0_(phai,  model_directory + std::string("/crowdcount_sim0.rknn"), device),
                 net_segment1_(phai,  model_directory + std::string("/crowdcount_sim1.rknn"), device)
        {

        }

        exposing::param_vector<crowd::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, int min_cluster_size, std::map<std::string, float>& param_map)
        {
            if (min_cluster_size <= 0)
            {
                throw exposing::abi_invalid_argument("min_cluster_size < = 0");
            }

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
                  throw exposing::abi_invalid_argument("incorrect roi in crowd");
            }

            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width));


            float pic_scale= cropped_image.cols>cropped_image.rows ?static_cast<float>(cropped_image.rows) /640.f :static_cast<float>(cropped_image.cols) /640.f   ; 

            cv::resize(cropped_image, cropped_image, cv::Size((int)(cropped_image.cols / pic_scale), (int)(cropped_image.rows / pic_scale)), cv::INTER_LINEAR);
            


            int cropwidth =  cropped_image.cols;
            int cropheight = cropped_image.rows;
            int xslice= (cropwidth+639)/640;
            int yslice= (cropheight+639)/640;
            int pad_h = yslice*640 - cropheight;
            int pad_w = xslice*640 - cropwidth;
            // std::cout<<xslice<<" "<<yslice<<std::endl;
            if(pad_h>0 || pad_w>0 )
            {
                cv::copyMakeBorder(cropped_image, cropped_image, 0, pad_h, 0, pad_w, cv::BORDER_CONSTANT, cv::Scalar{ 0,0,0 });
            }
            
            cropwidth=cropped_image.cols;
            cropheight = cropped_image.rows;

            // int index = 0;
            std::vector<cluster_info> detection_points;
            for(int i=0;i< yslice;i++)
            {
                for (size_t j = 0; j < xslice; j++)
                {
                    int cropx1= j*640; 
                    int cropx2= (j+1)*640 > cropwidth ? cropwidth :  (j+1)*640 ;
                    int cropy1= i*640; 
                    int cropy2= (i+1)*640 > cropheight ? cropheight :(i+1)*640 ;

                    // std::cout<<cropx1<<" "<<cropx2<<std::endl;

                    auto result = run_segment(cropped_image,cropx1,cropx2,cropy1,cropy2);
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
			const std::string algo_module_version = "2.1.1";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			//#if 0
			std::string nn_frame_version = net_segment0_.version();
#else
			std::string nn_frame_version = net_segment0_.version();
#endif
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
            if (detection_points.size() == 0)
                return results;
            cluster_num scaler;
            std::vector<int> cluster_num = scaler.find_cluster_num(detection_points, min_cluster_size);

            //crowd::box_info_internal
            //results.push_back(glasssix::exposing::make_as_first<box_info_impl>(it));
            for ( int i = 0 ;i< detection_points.size();++i)
            {
                if (cluster_num[i] == 0)
                    continue;
                crowd::box_info_internal temp;
                temp.x1= detection_points[i].x1;     
                temp.y1= detection_points[i].y1;     
                temp.x2= detection_points[i].x2;     
                temp.y2= detection_points[i].y2;     
                temp.category= cluster_num[i]-1;
                results.push_back(glasssix::exposing::make_as_first<box_info_impl>(temp));
            }
            return results;
        }
        void resize_nearst(const float *source,float *dst ,int sou_height,int sou_width,int dst_height,int dst_width,int channel )
        {
            for(int c=0;c < channel;c++)
            {
                float* dsts   = dst    + c*dst_height*dst_width;
                const float *sources = source + c*sou_height*sou_width;
                for (int y = 0; y < dst_height; ++y) 
                {
                    for (int x = 0; x < dst_width; ++x) 
                    {
                        int sourceX = x * sou_height / dst_height;
                        int sourceY = y * sou_width / dst_width;
                        float ss=sources[sourceY *sou_width + sourceX];
                        dsts[  y*dst_width + x]= 0.f;
                        dsts[  y*dst_width + x] = sources[sourceY *sou_width + sourceX];
                    }
                }
            }
        }

        void Mul_77(const float* sou1,const float *sou2,float* dst, int h_w, int channel)
        {
            for (size_t i = 0; i < channel; i++)
            {
                for (size_t j = 0; j < h_w; j++)
                {
                    dst[i * h_w + j] = sou1[i * h_w + j] * sou2[j];
                }
            }

        }


        void nchw2Nhwc(float* inputNCHW, float* outputNHWC, int batchSize, int numChannels, int height, int width) {
            int nhwcSize = batchSize * height * width * numChannels;
            for (int b = 0; b < batchSize; ++b) {
                for (int c = 0; c < numChannels; ++c) {
                    for (int h = 0; h < height; ++h) {
                        for (int w = 0; w < width; ++w) {
                            int indexNCHW = b * numChannels * height * width + c * height * width + h * width + w;
                            int indexNHWC = b * height * width * numChannels + h * width * numChannels + w * numChannels + c;
                            outputNHWC[indexNHWC] = inputNCHW[indexNCHW];
                        }
                    }
                }
            }
        }

        std::vector<detect_list> get_boxInfo_from_Binar_map(std::vector<int>& binar_map, int min_area=3)
        {
            std::vector<detect_list> result_part;
            int width = 640;
            int height = 640;
            cv::Mat grayImage(height, width, CV_8UC1);
            for (int i = 0; i < height; i++) 
            {
                for (int j = 0; j < width; j++) 
                {
                    int index = i * width + j;
                    grayImage.at<uchar>(i, j) = static_cast<uchar>(binar_map[index]*255);
                }
            }

            cv::Mat labeledImage;
            cv::Mat stats;
            cv::Mat centroids;

            int numLabels = cv::connectedComponentsWithStats(grayImage, labeledImage, stats, centroids,4);
            
            for (int i = 1; i < numLabels; ++i)  // 忽略背景标签 0
            {
                detect_list box;
                box.x1 = stats.at<int>(i, cv::CC_STAT_LEFT);
                box.y1 = stats.at<int>(i, cv::CC_STAT_TOP);
                box.x2 = stats.at<int>(i, cv::CC_STAT_LEFT) + stats.at<int>(i, cv::CC_STAT_WIDTH);
                box.y2 = stats.at<int>(i, cv::CC_STAT_TOP) + stats.at<int>(i, cv::CC_STAT_HEIGHT);

                // int left   = stats.at<int>(i, cv::CC_STAT_LEFT);
                // int top    = stats.at<int>(i, cv::CC_STAT_TOP);
                // int width  = stats.at<int>(i, cv::CC_STAT_WIDTH);
                // int height = stats.at<int>(i, cv::CC_STAT_HEIGHT);
                // int area   = stats.at<int>(i, cv::CC_STAT_AREA);
                // int centre_x =static_cast<int>(centroids.at<double>(i, 0));
                // int centre_y =static_cast<int>(centroids.at<double>(i, 1));
                result_part.push_back(box);
            }
            return result_part;
        }


        std::vector<detect_list> post_process( const float* pred_map, const float* predict, int size=640)
        {
            std::vector<int> binar_map (640*640);
            for (size_t i = 0; i < size*size; i++)
            {
                if(pred_map[i]>=predict[i]  )
                {
                    binar_map[i]=1;
                }
                else
                {
                    binar_map[i]=0;
                }
            }
            return get_boxInfo_from_Binar_map(binar_map);
        }

        
        std::vector<crowd::box_info_internal>  run_segment(cv::Mat& images, int cropx1,int cropx2,int cropy1,int cropy2 )
        {   
            std::vector<box_info_internal> output;
            cv::Rect rect {cropx1, cropy1, cropx2 - cropx1, cropy2 - cropy1};

            cv::Mat blobs = images(rect).clone();

            if(blobs.cols!=640||blobs.rows!=640 )
            {
                throw exposing::abi_invalid_argument("img size error in crowd");
            }

            cv::cvtColor(blobs, blobs, cv::COLOR_BGR2RGB);

            std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forward;


            /**
            * @fun forward part begin
            * @param  none
            * @return tensor(preprocess(image))
            * @details because of model truncation, we cut th models to 2 part and complement the middle 
            */
            auto  network_results = net_segment0_.forward(blobs.data, { 1, blobs.rows, blobs.cols,blobs.channels() }, RKNN_TENSOR_NHWC);

            forward.push_back(network_results["input.2292"]);
            forward.push_back(network_results["pred_map"]);
            std::vector<int> shape1 = {1, 720, 80, 80};
            auto Mul_268 = std::make_shared<glasssix::memory::tensor<float>>(shape1, -1, glasssix::memory::NCHW);
            std::vector<int> shape2 = {1, 1, 80, 80};
            auto Mul_263 = std::make_shared<glasssix::memory::tensor<float>>(shape2, -1, glasssix::memory::NCHW);
            auto input_188= std::make_shared<glasssix::memory::tensor<float>>(shape1, -1, glasssix::memory::NCHW);

            resize_nearst(forward[0]->cpu_data(),Mul_268->mutable_cpu_data(), 160, 160, 80, 80, 720   );
            resize_nearst(forward[1]->cpu_data(),Mul_263->mutable_cpu_data(), 640, 640, 80, 80, 1   );
            Mul_77(Mul_268->cpu_data(), Mul_263->cpu_data(), input_188->mutable_cpu_data(), 80*80,720   );

            std::vector<float> input_data( 720*80*80);

            nchw2Nhwc(input_188->mutable_cpu_data(),input_data.data(),1,720,80,80 );
            auto  network_result1 = net_segment1_.forward(input_data.data(), { 1, 80, 80,720 }, RKNN_TENSOR_NHWC);

            /**
            * @fun forward part end
            * @param  none
            * @return tensor(preprocess(image))
            */

            const float *predict=network_result1["predict"]->cpu_data();
            const float* pred_map = forward[1]->cpu_data();

            auto result_part = post_process( pred_map, predict);

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

            // std::vector<crowd::box_info_internal> temp;
            // return temp;
        }


    private:
        std::string model_directory_;
        int device_;
        glasssix::rknnwrapper::rknn_wrapper net_segment0_;
        glasssix::rknnwrapper::rknn_wrapper net_segment1_;
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
