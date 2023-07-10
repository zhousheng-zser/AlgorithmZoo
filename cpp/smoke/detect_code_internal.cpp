#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <utility>
#include <unordered_map>
#include <RKNN2Wrapper/rknn2_wrapper.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>


namespace glasssix::smoke
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{hardcode::get_model_params("flame", false),  exposing::to_narrow_string(model_directory), device} 
        {

        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device)
                :net_detect_(phai,  model_directory + std::string("/person_sim.rknn"), device), net_category_(phai, model_directory + std::string("/smoke_sim.rknn"), device), model_directory_(model_directory)
                ,weight_Gemm_87 (new glasssix::memory::tensor<float>(2, 8192, -1, glasssix::memory::NCHW, nullptr))
        {   
           
            get_weight (model_directory+std::string("/smoke_supplement.racy") ,8192*2+2,weight_Gemm_87);
        }

        exposing::param_vector<smoke::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            // std::cout<<"smoke\n";
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
                  throw exposing::abi_invalid_argument("incorrect roi in smoke");
            }

            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width));

            auto detect_result = run_detect(cropped_image, roi_x, roi_y, roi_width, roi_height, param_map);

            auto cate_result=categorys(cropped_image,detect_result);

            auto results = exposing::make_param_vector<smoke::box_info>();

            for(auto& it:cate_result) 
            {
                it.x1+=roi_x;
                it.x2+=roi_x;
                it.y1+=roi_y;
                it.y2+=roi_y;
                
                results.push_back(glasssix::exposing::make_as_first<box_info_impl>(it));
            }

            return results;
        }

       
        std::string version()
        {
        const std::string algo_module_version = "1.0.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        //#if 0
        std::string nn_frame_version = net_detect_.version();
#else
        std::string nn_frame_version = net_detect_.version();
#endif
        return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

    private:

        /**  @fun letterbox
         *   @param image scaleFill
         *   @return letterbox(image)
         *   @details Resize and pad image while meeting stride-multiple constrain
         */
        typedef struct Bbox 
        {
            int x;
            int y;
            int w;
            int h;
            float score;
            int category;
        }Bbox;

        struct location_char
        {
            int x1;
            int y1;
            int x2;
            int y2;
            int category;
        };
        static bool sort_score(Bbox box1,Bbox box2) {
            return box1.score > box2.score ? true : false;
        }

       
        /**
         * @fun preprocess
         * @param src, new_shape
         * @return tensor(preprocess(image))
         * @details image preprocess and make tensor from images
         */
        std::tuple<cv::Mat, float> preprocess_categroy(cv::Mat& src,const cv::Size& input_shape = cv::Size(224, 224))
        {
            float scale = std::min((float)input_shape.width/(float)src.cols, (float)input_shape.height/(float)src.rows);
            cv::Mat cut_image;
            cv::cvtColor(src, cut_image, cv::COLOR_BGR2RGB);
            // src.copyTo(cut_image);     
            cv::Mat mask_image;
            unsigned char *data1=src.ptr<unsigned char>();
            if (src.rows != 224 || src.cols !=224)
            {
                cv::resize(cut_image, cut_image, cv::Size(224, 224), cv::INTER_CUBIC);   //no centre crop
                mask_image=cut_image;
            }
            else 
            {
                mask_image= cut_image(cv::Range(16, 224 + 16), cv::Range(16, 224 + 16));
            }
            return {mask_image, scale};
        }

        std::tuple<cv::Mat, float> preprocess_detection(cv::Mat src,  cv::Size input_shape = cv::Size(640, 640) )
        {
            float scale = std::min((float)input_shape.width/(float)src.cols, (float)input_shape.height/(float)src.rows);
            cv::Mat cut_image;
            cv::Mat mask_image(input_shape, CV_8UC3, cv::Scalar(114, 114, 114));
            if( src.rows != input_shape.height || src.cols != input_shape.width)
            {      
                cv::resize(src, cut_image, cv::Size((int)(src.cols * scale), (int)(src.rows * scale)), cv::INTER_LINEAR);

                int pad_h = int((input_shape.height - cut_image.rows) /2 ) ; 
                int pad_w = int((input_shape.width - cut_image.cols) /2 ) ; 

                cv::copyMakeBorder(cut_image, mask_image, 0, input_shape.height-cut_image.rows, 0 , input_shape.width-cut_image.cols, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
        
            }
            else 
            {
                src.copyTo(mask_image);     
            }
            cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2RGB);
            unsigned char * data=mask_image.ptr<uchar>();
            return {mask_image,scale};
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
        std::vector<std::vector<float>> concat(std::vector<std::shared_ptr<memory::tensor<float>>>& outs, float conf_thres)
        {
            const float anchors[3][6] = { {36,75, 76,55, 72,146}, {142,110, 192,243, 459,401}, {12,16, 19,36, 40,28} };//yolov7用
        const float stride[3] = { 16.0, 32.0, 8.0 };//40 20 80->   30 15 60

        std::vector<std::vector<float>> result;
        for(int n = 0; n < 3; n++)
        {
            int num_grid_x = (int)(640 / stride[n]);
            int num_grid_y = (int)(640 / stride[n]);

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
                        // float* pdata = (float*)outs[n].data + ind *  outs[n].size[4];
                         const float* pdata = ptr_out + ind *  6;
                        float box_score = sigmoid_x(pdata[4]);
                        // if(box_score > 0.f)
                        // {
                            float cx = (sigmoid_x(pdata[0]) * 2.f - 0.5f + j) * stride[n];  //cx
                            float cy = (sigmoid_x(pdata[1]) * 2.f - 0.5f + i) * stride[n];  //cy
                            float w = powf(sigmoid_x(pdata[2]) * 2.f, 2.f) * anchor_w;      //w
                            float h = powf(sigmoid_x(pdata[3]) * 2.f, 2.f) * anchor_h;      //h

                            std::vector<float> element = {cx, cy, w, h, box_score, sigmoid_x(pdata[5])};
                            result.push_back(element);
                        // }
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

        struct label_confidence
        {   
            int x1;
            int y1;
            int x2;
            int y2;
            int label;
            float confidence;
        };

       

        static std::vector<boxes_conf> yolo2xyxy(std::vector<std::vector<float>>& src, float conf_thres=0.f)
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
                if(conf > conf_thres)
                {
                    boxes_conf temp{};
                    temp.top_x = top_x;
                    temp.top_y = top_y;
                    temp.bot_x = bot_x;
                    temp.bot_y = bot_y;
                    temp.conf = conf;
                    temp.category =  maxPosition - 5;
                    //if (temp.category == 0)
                    {
                        res.push_back(temp);
                    }

                }
            }
            return res;
        }

        static float iou(Bbox box1, Bbox box2) 
        {
            int x1 = std::max(box1.x, box2.x);
            int y1 = std::max(box1.y, box2.y);
            int x2 = std::min(box1.x + box1.w, box2.x + box2.w);
            int y2 = std::min(box1.y + box1.h, box2.y + box2.h);
            int w = std::max(0, x2 - x1);
            int h = std::max(0, y2 - y1);
            float over_area = w * h;
            return over_area / (box1.w*box1.h + box2.w*box2.h - over_area);
        }
 
        static std::vector<Bbox> nms(std::vector<Bbox>&boxes, float threshold)
        {
            std::vector<Bbox>resluts;
            std::sort(boxes.begin(), boxes.end(), sort_score);
            while (boxes.size()> 0) 
            {
                resluts.push_back(boxes[0]);
                int index = 1;
                while (index < boxes.size()) {
                    float iou_value = iou(boxes[0], boxes[index]);
                    if (iou_value > threshold) {
                        boxes.erase(boxes.begin() + index);
                    }
                    else {
                        index++;
                    }
                }
                boxes.erase(boxes.begin());
            }
            return  resluts;
        }



		/**
		 * @fun computNmsInput
		 * @param src, max_wh
		 * @return std::pair<bboxes, confidence>
		 * @details slice src into bboxes and confidence, which need by dnn::NMS
		 */
		static std::vector<Bbox> computeNmsInput(std::vector<boxes_conf>& src, int max_wh,float ratio)
        {
            std::vector<Bbox> boxes;
            std::vector<float> scores;
            std::vector<int> category;
            for(auto const &it: src)
            {
         
                int c = max_wh * it.conf;
                Bbox temp;
                temp.x      = static_cast<double>(it.top_x )*ratio;
                temp.y      = static_cast<double>(it.top_y)*ratio;
                temp.w  = static_cast<double>(it.bot_x - it.top_x)*ratio;
                temp.h  = static_cast<double>(it.bot_y - it.top_y)*ratio;
                temp.score=it.conf;
                temp.category = it.category;
                // std::cout<<it.top_x<<" "<< temp.x<<std::endl;
                //if (temp.category == 0 || temp.category == 4)
                //{
                    boxes.push_back(temp);
                //}
            }
            return boxes;
        }

		/**
		 * @fun non_max_suppression
		 * @param prediction, conf_thres, iou_thres
		 * @return std::vector(boxes, classes)
		 * @details Non-Maximum Suppression (NMS) on inference results
		 */
		static std::vector<location_char> non_max_suppression(std::vector<std::vector<float>>& prediction, float conf_thres, float iou_thres, float ratio)
        {
            // std::cout<<"nms inpu size "<<prediction.size()<<std::endl;

            auto compute_box = yolo2xyxy(prediction, conf_thres);  
            // Batched NMS
            int max_wh = 4096;
            std::vector<Bbox> boxes;
            std::vector<float> scores;
            std::vector<int> classes;

            boxes= computeNmsInput(compute_box, max_wh,ratio);

            std::vector<Bbox> class_num;
            std::vector<Bbox> class_metra;
            for (auto &box:boxes)
            {
                if (box.category == 0) 
                {
                    class_num.emplace_back(box);
                }
                else 
                {
                    class_metra.emplace_back(box);
                }
            }
            auto bboxes_num=nms(class_num, iou_thres);
            auto bboxes_metra = nms(class_metra, iou_thres);
            std::vector<location_char> output_num;
            std::vector<location_char> output_metra;

            //auto f = [](int x){if(x<0) return 0; else return x;};

            for (auto it : bboxes_num)
            {   
                location_char temp;
                temp.x1=it.x;
                temp.x2=it.x+it.w;
                temp.y1=it.y;
                temp.y2=it.y+it.h;
                temp.category = it.category;
                output_num.emplace_back(temp);
            }


            return output_num;
        }
        /**
           * @fun run_detect
           * @param image param_map
           * @return std::vector<smoke::box_info_internal>
           * @details run detect (maybe in multithreading)
         */
       static void ReduceL2_mul(  std::shared_ptr<glasssix::memory::tensor<float>> input, std::shared_ptr<glasssix::memory::tensor<float>> output,float ratio=100.f)   
    {
        float L2=0.f;
        const float *ptr=input->cpu_data();
        float *out_ptr=output->mutable_cpu_data();
        
        for(int i=0;i<input->count();i++)
        {
            L2+= (ptr[i]*ptr[i]);
        }

        L2=sqrt(L2);
        // L2= 48.41;
        // std::cout<<"L2: "<<L2<<std::endl;
        if(L2<0.00000f)
        {
            L2=0.00000000009;
        }
        for(int i=0;i<input->count();i++)
        {
            out_ptr[i]=(ptr[i]/L2)*ratio;
            // out_ptr[i]=(ptr[i]/L2)*ratio;
        }
        int m=0;

    }

        void get_weight(std::string_view model_file,int weight_size, std::shared_ptr<glasssix::memory::tensor<float>> weight_f32)
        {
            FILE *fp = fopen(model_file.data(), "rb");
            if (!fp)
            {
                LOG(FATAL) << "Cannot open " << model_file;
            }
            //LOG(INFO) << "[Pipeline weights memory cost list]=====================";
            // (new glasssix::memory::tensor<float>(weight_size, this->params_.device_, memory::NCHW, nullptr);
            // std::shared_ptr<glasssix::memory::tensor<float>> weight_f32(new glasssix::memory::tensor<float>(2, 8192, -1, glasssix::memory::NCHW, nullptr));
            fread(weight_f32->mutable_cpu_data(), 1, weight_size * sizeof(float), fp);
            CHECK_EQ(fclose(fp), 0) << "Cannot close " << model_file;
        }


        static void fully_connect(std::vector<std::shared_ptr<glasssix::memory::tensor<float>>>& bottom, 
                                        std::shared_ptr<glasssix::memory::tensor<float>> top )
    {
        int l,m,n;
        l= bottom[0]->data_shape()[1];

        n= bottom[1]->data_shape()[1];
        l=1;
        m=8192;
        n=2;
        CHECK_EQ(bottom[1]->count(),m*n);

        // top.reset
        float *in1_ptr=bottom[0]->mutable_cpu_data(); 
        // for(int i=0;i<8192;i++)
        // {
        //     in1_ptr[i]=1.f;
        // }

        float *in2_ptr=bottom[1]->mutable_cpu_data(); 
        float *out_ptr=top->mutable_cpu_data(); 
        for(int i=0;i<l;i++)
        {
             for(int j=0;j<n;j++)
             {
                for (size_t k = 0; k < m; k++)
                {
                   out_ptr[j]= out_ptr[j]+(in1_ptr[k]*in2_ptr[k]);
                }      
                in2_ptr+=m;
             }
             in1_ptr++;
             out_ptr++;
        }
        out_ptr[0]+=bottom[1]->mutable_cpu_data()[8192*2];
        out_ptr[1]+=bottom[1]->mutable_cpu_data()[8192*2+1];

    }
        struct nonzero_pair 
        {
            int xindex;
            int yindex;
            nonzero_pair(int x, int y) :xindex(x), yindex(y) { };
        };

        template <class T>
        T max_tensor(std::shared_ptr<glasssix::memory::tensor<T> > input) 
        {
            T max = input->cpu_data()[0];
            for (size_t i = 0; i < input->count(); i++)
            {
                max = max > input->cpu_data()[i] ? max:input->cpu_data()[i];
            }
            return max;
        }

        void BilinearInterMethod(int channelNum, int width, int height, float* imageDataInput, int resized_width, int resized_height, float* resizedData)
        {
            float r_scale = (float)height / resized_height;
            float c_scale = (float)width / resized_width;
        
            float r_delta = (height - resized_height * r_scale)*0.5f;
            float c_delta = (width - resized_width * c_scale)*0.5f;
        
            int r_w[2];
            int c_w[2];
        
            for (size_t r = 0; r < resized_height; r++)
            {
                float r_ori = r * r_scale + r_delta;
                int v_t = floor(r_ori);//坐标
                int v_b = ceil(r_ori);//坐标
                if (v_t > height-1 || v_b > height -1)
                {
                    v_t = height - 1;
                    v_b = height - 1;
                }
        
                r_w[0] = (v_b - r_ori)*256;
                r_w[1] = 256 - r_w[0];
        
                int ind = r * resized_width * channelNum;
        
                for (size_t c = 0; c < resized_width; c++)
                {
                    float c_ori = c * c_scale + c_delta;
                    int u_l = floor(c_ori);
                    int u_r = ceil(c_ori);
                    if (u_l > width - 1 || u_r > width - 1)
                    {
                        u_l = width - 1;
                        u_r = width - 1;
                    }
        
                    c_w[0] = (u_r - c_ori)*256;
                    c_w[1] = 256 - c_w[0];
        
                    int index = ind + c* channelNum;
        
                    for (size_t i = 0; i < channelNum; i++)
                    {
                        auto q1 = *(imageDataInput + v_t * width * channelNum + u_l * channelNum + i);
                        auto q2 = *(imageDataInput + v_t * width * channelNum + u_r * channelNum + i);
                        auto q3 = *(imageDataInput + v_b * width * channelNum + u_r * channelNum + i);
                        auto q4 = *(imageDataInput + v_b * width * channelNum + u_l * channelNum + i);
                        float value = (float)(r_w[0] * c_w[0] * q1 +
                            r_w[0] * c_w[1] * q2 +
                            r_w[1] * c_w[0] * q4 +
                            r_w[1] * c_w[1] * q3);
        
                        // *(resizedData + index + i) = value>>16;
                        resizedData[ index + i] = float(value/65536);
                    }
                }
            }
        }

        std::tuple<int, int,int,int> nonzero_indices_polar(const std::vector<nonzero_pair>& nonzero_indices)
        {
            int height_min = nonzero_indices[0].xindex;
            int height_max = nonzero_indices[0].xindex;
            int width_min = nonzero_indices[0].yindex;
            int width_max = nonzero_indices[0].yindex;
            for (size_t i = 0; i < nonzero_indices.size(); i++)
            {
                height_max = nonzero_indices[i].xindex > height_max ? nonzero_indices[i].xindex : height_max;
                height_min = nonzero_indices[i].xindex < height_min ? nonzero_indices[i].xindex : height_min;
                width_max = nonzero_indices[i].yindex > width_max ? nonzero_indices[i].yindex : width_max;
                width_min = nonzero_indices[i].yindex < width_min ? nonzero_indices[i].yindex : width_min;
            }
            return std::make_tuple(height_min, height_max, width_min, width_max);
        }
        
        void batch_augment(const cv::Mat & image,cv::Mat& output ,std::shared_ptr<glasssix::memory::tensor<float> > attention_map, int mode = 1, float theta = 0.5f, float padding_ratio = 0.1f)
        {
            int imgW = image.cols;
            int imgH = image.rows;
            float theta_c = 0.f;
            if (mode == 1) 
            {  
                theta_c= theta * max_tensor(attention_map);//DEFAULT THE theta=instance
                //将原来的值
                const  float* attention_map_ptr = attention_map->cpu_data();
                cv::Mat crop_mask(224,224,CV_32F);
                std::vector<int> shape = attention_map->data_shape();
                cv::Mat interpolation_Mat(14,14, CV_32F);;// = tensor2mat(attention_map, shape);

                memcpy(interpolation_Mat.data, attention_map_ptr, 196*sizeof(float));     
                //四维可能不能resize 需要降维 
                float* inter_ptr = interpolation_Mat.ptr<float>();
                float* crop_mask_ptr = crop_mask.ptr<float>();
                BilinearInterMethod(1,14,14,inter_ptr,224,224,crop_mask_ptr);
                std::vector<nonzero_pair> nonzero_indices;
        
                theta=0.1;
                for (size_t i = 0; i < imgW * imgH; i++)
                {
                    if (crop_mask_ptr[i] > theta) 

                    {
                        nonzero_indices.emplace_back(nonzero_pair(i / imgW, i % imgH));
                    }
                }
                //接下来分别找到nonzero_indices第一列和第二列的最大值
                int height_min,height_max,width_min,width_max;
                std::tie(height_min, height_max, width_min, width_max) = nonzero_indices_polar(nonzero_indices);
                height_min = std::max(static_cast<int>(height_min - padding_ratio * 224), 0);
                width_min = std::max(static_cast<int>(width_min - padding_ratio * 224), 0);
                height_max = std::min(static_cast<int>(height_max + padding_ratio * 224), imgH);
                width_max = std::min(static_cast<int>(width_max + padding_ratio * 224), imgW);
                if( (width_max-width_min)==224 && (height_max-height_min  )==224 )
                {
                    output=image;
                }
                else
                {
                    cv::resize(image(cv::Range(height_min, height_max), cv::Range(width_min, width_max)), output, cv::Size(224, 224), cv::INTER_CUBIC);

                }
            }    
        }
		
		void Operator_227_232( std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& inputs)
		{
			 std::shared_ptr<glasssix::memory::tensor<float>> operation_232 (new glasssix::memory::tensor<float>(1, 8192, -1, glasssix::memory::NCHW, nullptr));
			float* data=operation_232->mutable_cpu_data();
			const float* input=inputs["227"]->cpu_data();
			for(int i=0;i<inputs["227"]->count();i++ )
			{
				data[i]= sqrt(fabs(input[i])+0.000001)*input[i];
			}
			inputs["232"]=operation_232;
		}

        std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> Operator_completion(
                           std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& data)
        {

            std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> output;

            // std::shared_ptr<glasssix::memory::tensor<float>> data[1];
            std::shared_ptr<glasssix::memory::tensor<float>> feature_matrix_hat(new glasssix::memory::tensor<float>(1, 8192, -1, glasssix::memory::NCHW, nullptr));
            std::shared_ptr<glasssix::memory::tensor<float>> feature_matrix(new glasssix::memory::tensor<float>(1, 8192, -1, glasssix::memory::NCHW, nullptr));
            std::shared_ptr<glasssix::memory::tensor<float>> operation_240(new glasssix::memory::tensor<float>(1, 8192, -1, glasssix::memory::NCHW, nullptr));
            std::shared_ptr<glasssix::memory::tensor<float>> operation_267(new glasssix::memory::tensor<float>(1, 8192, -1, glasssix::memory::NCHW, nullptr));
            std::shared_ptr<glasssix::memory::tensor<float>> operation_272(new glasssix::memory::tensor<float>(1, 2, -1, glasssix::memory::NCHW, nullptr));
            std::shared_ptr<glasssix::memory::tensor<float>> operation_273(new glasssix::memory::tensor<float>(1, 2, -1, glasssix::memory::NCHW, nullptr));
            
           
            // std::string Gemm_87(model_directory_+std::string("/Gemm_87.racy") );
            // get_weight (Gemm_87,8192*2+2,weight_Gemm_87);
            const float *weight=weight_Gemm_87->cpu_data();
            
            // std::shared_ptr<glasssix::memory::tensor<float>> weight_Gemm_91(new glasssix::memory::tensor<float>(2, 8192, -1, glasssix::memory::NCHW, nullptr));
            // std::string Gemm_91("../models/Gemm_91.racy");
            // get_weight (Gemm_91,8192*2+2,weight_Gemm_91);
            // const float *weight2=weight_Gemm_91->cpu_data();

            // feature_matrix_hat实现全连接操作
            //读取Gemm_91权重
            std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> bottom;
            std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> bottom1;

            ReduceL2_mul(data["257"], feature_matrix_hat,100.f);//没问题
                    const float *ptr2=feature_matrix_hat->cpu_data();
            bottom.push_back(feature_matrix_hat);
            bottom.push_back(weight_Gemm_87);
            fully_connect(bottom, operation_272);
            
            ReduceL2_mul(data["232"], feature_matrix, 100.f ) ;//对的上
            ReduceL2_mul(data["232"], operation_240, 1.f ) ;//对的上
            bottom1.push_back(feature_matrix);
            bottom1.push_back(weight_Gemm_87);
            fully_connect(bottom1, operation_267);

            float* output273=operation_273->mutable_cpu_data();
            float* ptr272=operation_272->mutable_cpu_data();
            float* ptr267=operation_267->mutable_cpu_data();
            
            // std::cout<<"ptr267: "<<ptr267[0]<<" "<<ptr267[1]<<"\n";
            // std::cout<<"ptr272: "<<ptr272[0]<<" "<<ptr272[1]<<"\n";
            output273[0]=ptr267[0] - ptr272[0];
            output273[1]=ptr267[1] - ptr272[1];

            // ptr272[0]=ptr267[0]-ptr272[0];
            // ptr272[1]=ptr267[1]-ptr272[1];

            output["273"]=operation_273;
            output["output"]=operation_267;
            output["240"]=operation_240;

            return output;
        }

        std::tuple<int,float> post_process(std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& input)
        {
            std::shared_ptr<glasssix::memory::tensor<float>> y_pred(new glasssix::memory::tensor<float>
                    (1, 2, -1, glasssix::memory::NCHW, nullptr));
            std::shared_ptr<glasssix::memory::tensor<float>> y_pred_auxred(new glasssix::memory::tensor<float>
                    (1, 2, -1, glasssix::memory::NCHW, nullptr));

            const float* y_pred_raw=input["y_pred_raw"]->cpu_data();
            const float* y_pred_aux=input["y_pred_aux"]->cpu_data();
        
            const float* y_pred_crop3=input["y_pred_crop3"]->cpu_data();
            const float* y_pred_aux_crop3=input["y_pred_aux_crop3"]->cpu_data();

            float* y_pred_ptr=y_pred->mutable_cpu_data();
            float* y_pred_auxred_ptr=y_pred_auxred->mutable_cpu_data();

            y_pred_ptr[0]= (y_pred_raw[0]+y_pred_crop3[0])/2;
            y_pred_ptr[1]= (y_pred_raw[1]+y_pred_crop3[1])/2;

            y_pred_auxred_ptr[0]= (y_pred_crop3[0]+y_pred_aux_crop3[0])/2;
            y_pred_auxred_ptr[1]= (y_pred_crop3[1]+y_pred_aux_crop3[1])/2;

            y_pred_ptr[0]=sigmoid_x( y_pred_ptr[0]);
            y_pred_ptr[1]=sigmoid_x( y_pred_ptr[1]);

            int max_index=0;
            float max=0.f;
            float s=sigmoid_x(-0.74050f);
            if( y_pred_ptr[0]> y_pred_ptr[1])
            {
                max_index=0;
                max=y_pred_ptr[0];
            }
            else
            {
                max_index=1;
                max=y_pred_ptr[1];
            }
            return {max_index,max};

        }

        std::vector<smoke::box_info_internal> categorys(cv::Mat& image,std::vector<location_char>& cate_input)
        {
            std::vector<box_info_internal> l_c;
            for(auto x:cate_input)
            {   
                cv::Mat cate_blob;
                float ratio=1.f;
                cv::Mat cropped_image = image(cv::Range(x.y1, x.y2), cv::Range(x.x1, x.x2));
                std::tie(cate_blob, ratio) =preprocess_categroy(cropped_image);

                unsigned char *iptr=cate_blob.ptr<uchar>();
                // for (int i = 0; i < cate_blob.rows*cate_blob.cols*cate_blob.channels(); i++) 
                // {
                //     iptr[i]=1;
                // }
                //  std::cout<<int(iptr[0])<<" "<<int(iptr[1])<<std::endl;

                auto  network_result1 = net_category_.forward(cate_blob.data, 
                            { 1, cate_blob.rows, cate_blob.cols,cate_blob.channels() }, RKNN_TENSOR_NHWC);
				Operator_227_232(network_result1);
                auto net_full_result = Operator_completion(network_result1);

                cv::Mat crop_images3;
                batch_augment(cate_blob, crop_images3, network_result1["269"] ,1,0.1f);
                 
                auto  network_result2 = net_category_.forward(crop_images3.data, 
                    { 1, crop_images3.rows, crop_images3.cols,crop_images3.channels() }, RKNN_TENSOR_NHWC);
				Operator_227_232(network_result2);
				auto net_full_result2 = Operator_completion(network_result2);
				
                std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> post_input;
                post_input["y_pred_raw"]=net_full_result["output"];
                post_input["y_pred_aux"]=net_full_result["273"];
                post_input["y_pred_crop3"]=net_full_result2["output"];
                post_input["y_pred_aux_crop3"]=net_full_result2["273"];
                
                box_info_internal result;

                int label;
                float confidence;
                std::tie(label, confidence) = post_process(post_input);

                result.x1=x.x1;
                result.y1=x.y1;
                result.x2=x.x2;
                result.y2=x.y2;
                result.category=label;
                result.confidence=confidence;
                l_c.emplace_back(result);

            }
            return l_c;
        }

        std::vector<location_char> run_detect(cv::Mat& image, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            std::map<std::string, float> params = {
                    {"conf_thres", param_map.count("conf_thres") ? param_map["conf_thres"] : 0.1f},
                    {"iou_thres",  param_map.count("iou_thres") ? param_map["iou_thres"] : 0.45f}};
			
			auto old_shape = cv::Size(roi_width, roi_height);

			auto new_shape = cv::Size(640,  640);

            cv::Mat blob;
            float ratio = 0;

            std::tie(blob, ratio) = preprocess_detection( image, new_shape ) ;
            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;

            auto  network_result = net_detect_.forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);

            std::vector<std::string>  out_names={"359","379","output"};


            for (size_t i=0;i< 3; i++)
            {
                forwards.push_back(network_result[out_names[i]]);
            }

			float conf_threshold = 0.35f;
			float iou_threshold = 0.45f;

			auto result = concat(forwards, conf_threshold );

			auto nms_result = non_max_suppression(result, conf_threshold, iou_threshold, 1/ratio);
            std::vector<box_info_internal> output;
            
            return nms_result;
        }



    private:
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)

		rknnwrapper::rknn_wrapper net_detect_;
        rknnwrapper::rknn_wrapper net_category_;
#else
		std::unique_ptr<excalibur::pipeline<float>> net_detect_;
        std::unique_ptr<excalibur::pipeline<float>> net_category_;
#endif
        std::shared_ptr<glasssix::memory::tensor<float>> weight_Gemm_87;
        std::string model_directory_;
        int device_ ;

    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;


    exposing::param_vector<smoke::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }

    std::string detect_code_internal::version()
	{
		return impl_->version();
	}

}
