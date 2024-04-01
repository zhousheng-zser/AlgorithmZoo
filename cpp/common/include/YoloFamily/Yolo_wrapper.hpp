#include <opencv2/opencv.hpp>
#include <vector>
#include <algorithm>
#include "Excalibur/pipeline.hpp"
#include "Primitives/tensor_conversions.hpp"

using namespace glasssix;
    void Softmax(float* data, float* dst_data, int num )
    {             
        double L2_Sum=0.f;
        for(size_t i=0; i<num; i++) 
        {
            dst_data[i] = ( exp(data[i] ) );
            L2_Sum +=  dst_data[i];
        }
        for(size_t i=0; i<num; i++) 
            dst_data[i] = dst_data[i] / L2_Sum ;
    }

    static inline float sigmoid_x(float x)
    {
        return static_cast<float>(1.f / (1.f + exp(-x)));
    }

    static void tranpose(const float* sou, float* dest, int sourows, int soucols)
    {
        for(int i=0;i< sourows;i++)
            for(int j=0;j< soucols;j++)
                dest[j*sourows+i]=sou[ i * soucols + j];    
    }

    static void  Softmax(float* data, int num )
    {             
        double L2_Sum=0.f;
        for(size_t i=0; i<num; i++) 
        {
            data[i]= ( exp(data[i] ) );
            L2_Sum +=  data[i];
        }
        for(size_t i=0; i<num; i++) 
            data[i] =  data[i] / L2_Sum ;
    }

    static inline float de_sigmoid(float x)
    {
        if(x>=1 ||x<0)
            return NAN;
        return static_cast<float> (log( x/(1-x)));
    }

    struct key_point
    {
        float x;
        float y;
        float score;
        key_point(float x_, float y_, float score_):x(x_),y(y_),score(score_)
        {}
    };

    struct ObjectInfo
    {
        int x1;
        int y1;
        int x2;
        int y2;
        int category;
        float score;
        std::vector<key_point> key_points;
        ObjectInfo(int x1_,int y1_,int x2_,int y2_,int category_,float score_ ):x1(x1_),y1(y1_),x2(x2_),y2(y2_),category(category_),score(score_)
        {}

        ObjectInfo(int x1_,int y1_,int x2_,int y2_,int category_,float score_,std::vector<key_point> &key_points_):x1(x1_),y1(y1_),x2(x2_),y2(y2_),category(category_),score(score_),key_points(key_points_)
        {}
    };


    using Box = std::vector<float>;
    struct pic_process_param
    {
        int pad_h;
        int pad_w;
        float ratio;
    };


// 基类 YoloBase
template <typename T>
class YoloBase {
protected:
	cv::Mat infer_image;
    cv::Mat image;
    int model_input_height_;
    int model_input_width_;
	T pipeline;
    pic_process_param pic_process_param_;

public:
    YoloBase(int model_input_width,int model_input_height, T pipe) : pipeline(pipe),model_input_height_(model_input_height), model_input_width_(model_input_width)  {}

	virtual void preprocess_detection(cv::Mat& src,  cv::Size input_shape = cv::Size(640, 640) ,bool BGR2RGB=true )=0;
	
	virtual std::vector<std::vector<float>> yoloconcat(std::vector<std::shared_ptr<glasssix::memory::tensor<float>>>& outs,float conf )=0;

	std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> sort_model_result(std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>>& model_results )
	{
		struct CompareSharedPtrLength {
			bool operator()(const std::shared_ptr<memory::tensor<float>>& a, const std::shared_ptr<memory::tensor<float>>& b) const {
				return a->count() < b->count();
			}
		};
		std::vector<std::shared_ptr<memory::tensor<float>>> output;
		std::transform(model_results.begin(), model_results.end(), std::back_inserter(output),
					[](const std::pair<std::string, std::shared_ptr<memory::tensor<float>>>& pair) {
						return pair.second;
					});
		std::sort(output.begin(), output.end(), CompareSharedPtrLength());
		return output;
	}
	
    std::vector<std::vector<float>> centre_xywh2WH(std::vector<std::vector<float>>& net_result, int pad_h, int pad_w, float scale )
    {
            std::vector<std::vector<float>> output;
            if(!net_result.size() )
                return output;

            for(auto& centrexywh : net_result)
            {
                std::vector<float> temp(centrexywh.size());
                temp = centrexywh;
                temp[0]    =  static_cast<double>((centrexywh[0] - centrexywh[2]/ 2) - pad_w)*scale;
                temp[1]    =  static_cast<double>((centrexywh[1] - centrexywh[3]/ 2)- pad_h)*scale;
                temp[2]    =  static_cast<double>(centrexywh[2])*scale ;
                temp[3]    =  static_cast<double>(centrexywh[3])*scale ;       
                temp[4]=centrexywh[4];
                temp[5]=centrexywh[5];
                for (size_t i = 0; i < (centrexywh.size()-6)/3; i++)
                {
                    temp[6 + i*3 + 0] =  (temp[6 + i*3 + 0] - pad_w)*scale;
                    temp[6 + i*3 + 1] =  (temp[6 + i*3 + 1] - pad_h)*scale;
                }
                
                output.push_back(temp);            
            }
            return output ;
    }

    float intersectionOverUnion(const Box& box1, const Box& box2) {
        float x1 = std::max(box1[0], box2[0]);
        float y1 = std::max(box1[1], box2[1]);
        float x2 = std::min(box1[0] + box1[2], box2[0] + box2[2]);
        float y2 = std::min(box1[1] + box1[3], box2[1] + box2[3]);

        if (x1 >= x2 || y1 >= y2) {
            return 0.0f; 
        }

        float intersection_area = (x2 - x1) * (y2 - y1);
        float area1 = box1[2] * box1[3];
        float area2 = box2[2] * box2[3];
        float union_area = area1 + area2 - intersection_area;

        return intersection_area / union_area;
    }

    std::vector<int> object_nms(const std::vector<Box>& boxes, float iou_threshold) 
    {
        std::vector<int> indices(boxes.size());
        for (size_t i = 0; i < boxes.size(); i++)
            indices[i]=i;
        
        std::vector<float> scores;
        for (size_t i = 0; i < boxes.size(); ++i) 
            scores.push_back(boxes[i][4]);

        std::sort(indices.begin(), indices.end(), [&scores](int a, int b) {
            return scores[a] > scores[b];
        });

        std::vector<int> keep;
        while (indices.size() > 0) 
        {
            int idx = indices[0];
            keep.push_back(idx);

            std::vector<int> new_indices;
            for (size_t i = 1; i < indices.size(); ++i) 
            {
                int cur_idx = indices[i];
                if (intersectionOverUnion(boxes[idx], boxes[cur_idx]) <= iou_threshold) 
                    new_indices.push_back(cur_idx);          
            }
            indices = new_indices;
        }
        return keep;
    }

	void box_result_move_to_disjoint_region(std::vector<std::vector<float>>&sou_data, int bias=100000 )
    {
        for (size_t i = 0; i < sou_data.size(); i++)
            sou_data[i][0] =  sou_data[i][0]+ sou_data[i][5]*bias;      
    }

    // 获取检测到的对象
    std::vector<ObjectInfo> get_objects(cv::Mat image, float conf=0.5,float iou_threshold=0.65  ) 
	{	
		std::vector<ObjectInfo> out;
		auto new_shape = cv::Size(model_input_width_, model_input_height_);
		preprocess_detection(image,new_shape);
		auto model_results = pipeline->forward(infer_image);  //最好做编译器检查 检查是不是pipeline是不是genpipeline继承类
		std::vector<std::shared_ptr<memory::tensor<float>>> model_results_vector = sort_model_result(model_results);

		auto real_output = yoloconcat(model_results_vector,conf);

		auto nms_input  = centre_xywh2WH(real_output, pic_process_param_.pad_h, pic_process_param_.pad_w, 1.f/pic_process_param_.ratio);

        box_result_move_to_disjoint_region( nms_input, 100000);

        auto nms_result_index = object_nms(nms_input,iou_threshold);

        box_result_move_to_disjoint_region( nms_input, -100000);
		for (size_t i = 0; i < nms_result_index.size(); i++)
        {   
            int index = nms_result_index[i];
            std::vector<key_point> key_points;  
            const size_t offset = 6; 
            const size_t step = 3; 
            
            for (size_t i = 0; i < (nms_input[index].size() - offset) / step; ++i) {  
                key_points.emplace_back(  
                    std::round(nms_input[index][offset + i * step]),  
                    std::round(nms_input[index][offset + i * step + 1]),  
                    nms_input[index][offset + i * step + 2]  
                );  
            }

            out.emplace_back(std::round(nms_input[index][0]), std::round(nms_input[index][1]),std::round(nms_input[index][0]+nms_input[index][2]), std::round(nms_input[index][1]+nms_input[index][3]),
                            std::round(nms_input[index][5]),  nms_input[index][4], key_points
            );
            
            cv::rectangle(image, cv::Point(int(nms_input[index][0]), int(nms_input[index][1])), 
                    cv::Point(int(nms_input[index][0]+nms_input[index][2]), int(nms_input[index][1]+nms_input[index][3])), cv::Scalar(0, 255, 255), 2);
            for (size_t j = 0; j < key_points.size(); j++)
            {
                cv::circle(image, cv::Point((int)key_points[j].x, (int)key_points[j].y), 3, cv::Scalar(0, 0, 255), 2);
            }
	    }
		cv::imwrite("0331.jpg", image);
		return out;
		
	};

};

// YOLO 版本 8
template <typename T, bool LxyYolo, bool Posture=false>
class Yolov8 : public YoloBase< std::shared_ptr<T> > {
public:
    
        Yolov8(int model_input_width, int model_input_height,  std::shared_ptr<T> pipe) : 
        YoloBase< std::shared_ptr<T>>(model_input_width, model_input_height, pipe) {}

        void preprocess_detection(cv::Mat& src,  cv::Size input_shape = cv::Size(640, 640) ,bool BGR2RGB=true )  override
        {
            this->pic_process_param_.ratio = std::min((float)input_shape.width/(float)src.cols, (float)input_shape.height/(float)src.rows);
            cv::Mat cut_image;
            if( src.rows != input_shape.height || src.cols != input_shape.width)
            {      
                cv::resize(src, cut_image, cv::Size((int)(src.cols * this->pic_process_param_.ratio), (int)(src.rows * this->pic_process_param_.ratio)), cv::INTER_LINEAR);
                this->pic_process_param_.pad_h = std::round((input_shape.height - cut_image.rows) /2 ) ; 
                this->pic_process_param_.pad_w = std::round((input_shape.width - cut_image.cols) /2 ) ; 
                cv::copyMakeBorder(cut_image, this->infer_image, this->pic_process_param_.pad_h, input_shape.height-cut_image.rows-this->pic_process_param_.pad_h, this->pic_process_param_.pad_w, input_shape.width-cut_image.cols- this->pic_process_param_.pad_w, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
            }
            else 
                src.copyTo(this->infer_image);     
            if(BGR2RGB)
                cv::cvtColor(this->infer_image, this->infer_image, cv::COLOR_BGR2RGB);
        }


        std::vector<std::vector<float>> yoloconcat(std::vector<std::shared_ptr<memory::tensor<float>>>& outs,float conf ) override
        {
if constexpr (Posture)
            return yolov8concat_posture(outs, conf );
else
            return yolov8concat_general(outs, conf );
        }

        std::vector<std::vector<float>> yolov8concat_posture(std::vector<std::shared_ptr<memory::tensor<float>>>& outs,float conf )
        {
            conf = de_sigmoid(conf);
            int category = outs[0]->channels() - 64;
            std::vector<int> mul = {32,16,8};
            std::vector<std::vector<float>> output_new;
            for (size_t index = 0; index < outs.size(); index+=2 )
            {
                auto& stride_data_xywh    = outs[index+1]; //get xywh data
                auto& stride_data_posture = outs[index]; 

                auto data_shape = stride_data_xywh->data_shape();
                auto posture_shape = stride_data_posture->data_shape();
                //CHECK_EQ(data_shape.size()，4);
                int slice_box_size = data_shape[data_shape.size()-2]*data_shape[data_shape.size()-1];

                const float* conf_= stride_data_xywh->mutable_cpu_data() + slice_box_size * 64;

                std::vector<int> candicate_index;
                std::vector<int> category_label;
                for (size_t slice_index = 0; slice_index < slice_box_size ; slice_index++)
                    if(conf_[slice_index]>conf ) 
                        candicate_index.push_back( slice_index );

                if(!candicate_index.size() )  continue;
                std::vector<float> reshape_box( slice_box_size*64 );
                std::vector<float> posture_reshape_box( slice_box_size*64 );
                // std::cout<<"working\n";
                tranpose(stride_data_xywh->mutable_cpu_data(), reshape_box.data(), 64, slice_box_size);
                tranpose(stride_data_posture->mutable_cpu_data(), posture_reshape_box.data(), posture_shape[1], slice_box_size);

                for (size_t index_current = 0; index_current < candicate_index.size(); index_current++)
                {
                        int slice_index = candicate_index[index_current];
                        std::vector<float> temp_data(64);
                        std::vector<float> centre_xywh(4,0);
                        std::vector<float> out_centre_xywh(6+posture_shape[1],0);
                        for (size_t softmaxmove = 0; softmaxmove < 4; softmaxmove++)
                            Softmax(reshape_box.data()+64*slice_index+softmaxmove*16, temp_data.data()+softmaxmove*16, 16 );
                        
                        for(int i=0;i<16;i++)
                            for(int j=0;j<4;j++)
                                for (size_t k = 0; k < 1; k++)
                                    centre_xywh[j*1 + k] = centre_xywh[j*1 + k] + temp_data[k*16*4+j*16+i]*i;//             i*4*candidate_num+j ]*i; 
                        out_centre_xywh[0] = ((centre_xywh[2] - centre_xywh[0])/2.f + slice_index%data_shape[data_shape.size()-1] + 0.5f) * mul[index/2 ];
                        out_centre_xywh[1] = ((centre_xywh[3] - centre_xywh[1])/2.f + slice_index/data_shape[data_shape.size()-1] + 0.5f) * mul[index/2];
                        out_centre_xywh[2] = (centre_xywh[2] + centre_xywh[0]) * mul[index/2];
                        out_centre_xywh[3] = (centre_xywh[3] + centre_xywh[1]) * mul[index/2];
                        out_centre_xywh[4] = sigmoid_x (conf_[slice_index ]);
                        out_centre_xywh[5] = 0; //default single classify
                        float *posture_data2 = posture_reshape_box.data() + posture_shape[1]*slice_index;
                        for (size_t key_point = 0; key_point < posture_shape[1]/3; key_point++)
                        {
                            out_centre_xywh[6 + key_point*3 + 0] = (posture_data2[key_point*3+0]*2 + slice_index%data_shape[data_shape.size()-1])*mul[index/2 ];
                            out_centre_xywh[6 + key_point*3 + 1] = (posture_data2[key_point*3+1]*2 + slice_index/data_shape[data_shape.size()-1])*mul[index/2 ];
                            out_centre_xywh[6 + key_point*3 + 2] = sigmoid_x(posture_data2[key_point*3+2] );
                        }
                        output_new.push_back(out_centre_xywh);
                }
            }
            return output_new;  
        }
           

        std::vector<std::vector<float>> yolov8concat_general(std::vector<std::shared_ptr<memory::tensor<float>>>& outs,float conf ) 
        {
            conf = de_sigmoid(conf);
            int category = outs[0]->channels() - 64;
            std::vector<int> mul = {32,16,8,4};
            std::vector<std::vector<float>> output_new;
            for (size_t index = 0; index < outs.size(); index++)
            {
                auto& stride_data = outs[index]; 
                auto data_shape = stride_data->data_shape();
                int slice_box_size = data_shape[data_shape.size()-2]*data_shape[data_shape.size()-1];

                const float* conf_;
if constexpr (LxyYolo) 
                conf_ = stride_data->mutable_cpu_data();
else 
                conf_ = stride_data->mutable_cpu_data() + slice_box_size * 64;

                std::vector<int> candicate_index;
                std::vector<int> category_label;
                for (size_t slice_index = 0; slice_index < slice_box_size*category ; slice_index++)
                    if(conf_[slice_index]>conf ) 
                    {
                        candicate_index.push_back( slice_index % slice_box_size );
                        category_label.push_back(slice_index / slice_box_size  );
                    }

                if(!candicate_index.size() )  continue;
                std::vector<float> reshape_box( slice_box_size*64 );
if constexpr (LxyYolo) 
                tranpose(stride_data->mutable_cpu_data() + slice_box_size * category, reshape_box.data(), 64, slice_box_size);
else 
                tranpose(stride_data->mutable_cpu_data(), reshape_box.data(), 64, slice_box_size);

                for (size_t index_current = 0; index_current < candicate_index.size(); index_current++)
                {
                        int slice_index = candicate_index[index_current];
                        std::vector<float> temp_data(64);
                        std::vector<float> centre_xywh(4,0);
                        std::vector<float> out_centre_xywh(6,0);
                        for (size_t softmaxmove = 0; softmaxmove < 4; softmaxmove++)
                            Softmax(reshape_box.data()+64*slice_index+softmaxmove*16, temp_data.data()+softmaxmove*16, 16 );
                        
                        for(int i=0;i<16;i++)
                            for(int j=0;j<4;j++)
                                for (size_t k = 0; k < 1; k++)
                                    centre_xywh[j*1 + k] = centre_xywh[j*1 + k] + temp_data[k*16*4+j*16+i]*i;//             i*4*candidate_num+j ]*i; 
                        out_centre_xywh[0] = ((centre_xywh[2] - centre_xywh[0])/2.f + slice_index%data_shape[data_shape.size()-1] + 0.5f) * mul[index];
                        out_centre_xywh[1] = ((centre_xywh[3] - centre_xywh[1])/2.f + slice_index/data_shape[data_shape.size()-1] + 0.5f) * mul[index];
                        out_centre_xywh[2] = (centre_xywh[2] + centre_xywh[0]) * mul[index];
                        out_centre_xywh[3] = (centre_xywh[3] + centre_xywh[1]) * mul[index];
                        out_centre_xywh[4] = sigmoid_x (conf_[slice_index +category_label[index_current]*slice_box_size ]);
                        out_centre_xywh[5] = category_label[index_current];
                        output_new.push_back(out_centre_xywh);
                }
            }
            return output_new;  
        }

};
			


// #include <GenPipeline/GenPipeline.hpp>  注意链接对应GenPipeline库
// #include <YoloFamily/Yolo_wrapper.hpp>
// int main() {
// 	cv::Mat image;

    //姿态检测示例
    // std::shared_ptr<GenPipeline> net_posture_;
    // std::shared_ptr<Yolov8<GenPipeline, false,true>> yolov8_instance;
    // net_posture_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/posture960_17.rknn", device);
    // yolov8_instance = std::make_shared<Yolov8<GenPipeline,false,true>>(960,960, net_posture_); //3个模板变量分别对应 GenPipeline ，(通用yolov8)是否是李鑫尧的yolo  是否是姿态检测模型 

    // //通用yolov8示例(无关键点模型)
    // std::shared_ptr<GenPipeline> net_pump_;
    // std::shared_ptr<Yolov8<GenPipeline, false,true>> yolov8_instance;
    // net_posture_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/pump.rknn", device);
    // yolov8_instance = std::make_shared<Yolov8<GenPipeline,true>>(1152,640, net_posture_); //2个模板变量分别对应 GenPipeline ，(通用yolov8)是否是李鑫尧的yolo  第三个参数默认为false
    // auto yolov8_detect = yolov8_instance->get_objects( image,conf_threshold,iou_threshold);

    // yolov8_detect包含了指定置信度及其iou的对应模型检测结果 std::vector<ObjectInfo>

// }
