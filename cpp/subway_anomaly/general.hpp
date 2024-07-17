#ifndef _GENERAL_HPP_
#define _GENERAL_HPP_

#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include "hardcode.hpp"
#include "Excalibur/pipeline.hpp"
#include "Primitives/tensor_conversions.hpp"


#define lxyyolo

using namespace glasssix;
float MIN_BATTERY_CAR_SCALE = 40;
float AERO_RATE_BETWEEN_BATTERY_CAR_AND_PEOPLE = 0.15;
float IOU_BETWEEN_BATTERY_CAR = 0.8;
float IOU_BETWEEN_BATTERY = 0.8;

struct Bbox
{
    int x1;
    int y1;
    int x2;
    int y2;
    int category;
    float score;
    int frame_id;
    std::vector<float> key_points;
    Bbox(int x11, int y11, int x22, int y22, int category_, float score_, int frame_index) :x1(x11), x2(x22), y1(y11), y2(y22), category(category_), score(score_), frame_id(frame_index)
    {}
    Bbox(const Bbox& input) :x1(input.x1), x2(input.x2), y1(input.y1), y2(input.y2), category(input.category), score(input.score), frame_id(input.frame_id), key_points(input.key_points)
    {}
    Bbox(int x11, int y11, int x22, int y22) :x1(x11), x2(x22), y1(y11), y2(y22), category(-1), score(-1), frame_id(-1)
    {}

    Bbox& operator=(const Bbox& input)
    {
        if (this != &input) // 避免自我赋值
        {
            x1 = input.x1;
            x2 = input.x2;
            y1 = input.y1;
            y2 = input.y2;
            category = input.category;
            score = input.score;
            frame_id = input.frame_id;
            key_points = input.key_points;
        }
        return *this;
    }

    int area()
    {
        return (y2 - y1) * (x2 - x1);
    }
};

struct car_person_batery
{
    Bbox car;
    Bbox person;
    Bbox battery;

    car_person_batery(Bbox& car_, Bbox& person_, Bbox& battery_) : car(car_), person(person_), battery(battery_)
    {}
};

float bbox_iou(Bbox& input1, Bbox& input2)
{
    float w = std::max(input1.x2, input2.x2) - std::min(input1.x1, input2.x1);
    float h = std::max(input1.y2, input2.y2) - std::min(input1.y1, input2.y1);
    float ww = input1.x2 + input2.x2 - input1.x1 - input2.x1;
    float hh = input1.y2 + input2.y2 - input1.y1 - input2.y1;
    if (ww < w || hh < h)
        return 0;
    else
        return  (ww - w) * (hh - h) / static_cast<float>((input1.x2 - input1.x1) * (input1.y2 - input1.y1));
}

Bbox get_min_rect_in_car_person(Bbox& car, Bbox& person)
{
    int x1_min = car.x1 < person.x1 ? car.x1 : person.x1;
    int y1_min = car.y1 < person.y1 ? car.y1 : person.y1;
    int x2_max = car.x2 > person.x2 ? car.x2 : person.x2;
    int y2_max = car.y2 > person.y2 ? car.y2 : person.y2;
    Bbox rect(x1_min, y1_min, x2_max, y2_max);
    return rect;
}

bool is_battery_in_rect(Bbox& rect, Bbox& battery)
{
    bool in_rect = battery.x1 > rect.x1 && battery.x2<rect.x2 && battery.y1>rect.y1 && battery.y2 < rect.y2;
    return in_rect;
}

static inline float sigmoid_x(float x)
{
    return static_cast<float>(1.f / (1.f + exp(-x)));
}

void tranpose(const float* sou, float* dest, int sourows, int soucols)
{
    for (int i = 0; i < sourows; i++)
        for (int j = 0; j < soucols; j++)
            dest[j * sourows + i] = sou[i * soucols + j];
}

static void Softmax(float* data, int num)
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

static inline float de_sigmoid(float x)
{
    if (x >= 1 || x < 0)
        return NAN;
    return static_cast<float> (log(x / (1 - x)));
}

static void concat_pic(std::vector<cv::Mat>& candicate_images, float* dst_data)
{
    std::array<float, 3> means{ 123.675,116.28,103.53 };
    std::array<float, 3> stds{ 1.f / 58.395,1.f / 57.12,1.f / 57.375 };
    //     std::array<float,3> means{0,0,0};
    // std::array<float,3> stds{1.f,1.f,1.f};
    int channels_size = candicate_images.size() * 3;
    for (size_t i = 0; i < candicate_images.size(); i++)
    {
        int base = i * 3;
        uchar* ptr_sour = candicate_images[i].ptr<uchar>();
        for (size_t j = 0; j < 256 * 256; j++)
        {
            dst_data[base + j * channels_size] = (ptr_sour[j * 3] - means[0]) * stds[0];
            dst_data[base + j * channels_size + 1] = (ptr_sour[j * 3 + 1] - means[1]) * stds[1];
            dst_data[base + j * channels_size + 2] = (ptr_sour[j * 3 + 2] - means[2]) * stds[2];
        }
    }
}

std::vector<Bbox> get_candicate_rect(std::vector<car_person_batery>& candicates1,
    std::vector<car_person_batery>& candicates2)
{
    std::vector<Bbox> valid_crop_box_list;
    for (auto& candicate2 : candicates2)
        for (auto& candicate1 : candicates1)
            if (bbox_iou(candicate2.car, candicate1.car) > IOU_BETWEEN_BATTERY_CAR && bbox_iou(candicate2.battery, candicate1.battery) < IOU_BETWEEN_BATTERY)
                valid_crop_box_list.push_back(get_min_rect_in_car_person(candicate2.car, candicate2.person));
    return valid_crop_box_list;
}

std::vector<car_person_batery> deal_one_frame(std::vector<Bbox>& input)
{
    std::vector<car_person_batery> result;
    std::vector<Bbox> cars;
    std::vector<Bbox> batteries;
    std::vector<Bbox> persons;

    for (auto& var : input)
    {
        if (var.category == 0)
        {
            persons.push_back(var);
        }
        else if (var.category == 1 && var.area() > MIN_BATTERY_CAR_SCALE)
        {
            cars.push_back(var);
        }
        else if (var.category == 2)
        {
            batteries.push_back(var);
        }
    }

    if (batteries.size() == 0 || cars.size() == 0 || persons.size() == 0)
    {
        return result;
    }
    else
    {
        for (auto& car : cars)
        {
            for (auto& person : persons)
            {
                if (bbox_iou(person, car) > AERO_RATE_BETWEEN_BATTERY_CAR_AND_PEOPLE)
                {
                    Bbox rect = get_min_rect_in_car_person(car, person);
                    std::vector<Bbox> tmp_batteries;
                    for (auto& battery : batteries)
                    {
                        if (battery.area() < person.area() && is_battery_in_rect(rect, battery))
                        {
                            tmp_batteries.push_back(battery);
                        }
                    }
                    if (!tmp_batteries.size())
                    {
                        //当前两者无匹配电瓶
                    }
                    else
                    {
                        if (tmp_batteries.size() == 1)
                        {
                            result.emplace_back(car, person, tmp_batteries[0]);
                        }
                        else //获取置信度最大的电瓶
                        {
                            int index = 0;
                            float score = tmp_batteries[0].score;
                            for (size_t i = 1; i < tmp_batteries.size(); i++)
                            {
                                if (tmp_batteries[i].score > score)
                                {
                                    index = i;
                                    score = tmp_batteries[i].score;
                                }
                            }
                            result.emplace_back(car, person, tmp_batteries[index]);

                        }
                    }
                }
            }
        }
    }
    //解析每帧数据
    //返回当前帧符合要求的车和人，电瓶数目
    return result;
}


std::shared_ptr<memory::tensor<float>> Yolov8s_Concat(std::vector<std::shared_ptr<memory::tensor<float>>>& outs, float conf, int& candicate_num,
    const std::vector<float>& add_weight_, const std::vector<float>& mul_weight_, std::vector<int>& category_mask, int width = 1280, int height = 1280)
{
    conf = de_sigmoid(conf);
    int input = width * height;
    int category = outs[0]->channels() - 64;
    int box_tmp_size = 64;
    int stride_8_num = input / 64;
    int stride_16_num = input / 256;
    int stride_32_num = input / 1024;

    int candidate_num = stride_8_num + stride_16_num + stride_32_num;
    int totol_size = stride_8_num + stride_16_num + stride_32_num;
    //20 40 80 
    const float* data_stride_8 = outs[2]->cpu_data();
    const float* data_stride_16 = outs[1]->cpu_data();
    const float* data_stride_32 = outs[0]->cpu_data();

    std::vector<int> match_index;

#ifdef lxyyolo
    const float* data_stride_8_conf = data_stride_8;
    const float* data_stride_16_conf = data_stride_16;
    const float* data_stride_32_conf = data_stride_32;

#else
    const float* data_stride_8_conf = data_stride_8 + stride_8_num * 64;
    const float* data_stride_16_conf = data_stride_16 + stride_16_num * 64;
    const float* data_stride_32_conf = data_stride_32 + stride_32_num * 64;
#endif 


    for (size_t i = 0; i < stride_8_num * category; i++)
        if (data_stride_8_conf[i] > conf)
        {
            match_index.push_back(i % stride_8_num);
            category_mask.push_back(i / stride_8_num);
        }


    for (size_t i = 0; i < stride_16_num * category; i++)
        if (data_stride_16_conf[i] > conf)
        {
            match_index.push_back(i % stride_16_num + stride_8_num);
            category_mask.push_back(i / stride_16_num);
        }


    for (size_t i = 0; i < stride_32_num * category; i++)
        if (data_stride_32_conf[i] > conf)
        {
            match_index.push_back(i % stride_16_num + stride_8_num + stride_16_num);
            category_mask.push_back(i / stride_32_num);
        }
    //concat the 80*40 40*40 20*20 p
    std::vector<float> cat(outs[0]->channels() * candidate_num); //1*65*candidate_num = 64*candidate_num + 1*candidate_num        
    for (int i = 0; i < outs[0]->channels(); i++)
    {
        std::copy(data_stride_8 + i * stride_8_num, data_stride_8 + (i + 1) * stride_8_num, cat.data() + i * candidate_num);
        std::copy(data_stride_16 + i * stride_16_num, data_stride_16 + (i + 1) * stride_16_num, cat.data() + i * candidate_num + stride_8_num);
        std::copy(data_stride_32 + i * stride_32_num, data_stride_32 + (i + 1) * stride_32_num, cat.data() + i * candidate_num + stride_8_num + stride_16_num);
    }

    //tranpose and softmax
    std::vector<float> reshape_box(candidate_num * 64);


    auto start = std::chrono::high_resolution_clock::now();

#ifdef lxyyolo
    tranpose(cat.data() + totol_size * category, reshape_box.data(), 64, candidate_num);
#else
    tranpose(cat.data(), reshape_box.data(), 64, candidate_num);
#endif 

    candidate_num = match_index.size();

    candicate_num = candidate_num;
    std::vector<float> reshape_boxtmp(candidate_num * 64);
    std::shared_ptr<glasssix::memory::tensor<float>> output0
    (new memory::tensor<float>(std::vector<int>{1, 5, candidate_num}, -1, memory::NCHW));

    for (size_t i = 0; i < match_index.size(); i++)
        std::copy(reshape_box.data() + match_index[i] * 64, reshape_box.data() + match_index[i] * 64 + 64, reshape_boxtmp.data() + i * 64);

    int index = 0;
    for (int i = 0; i < candidate_num; i++)
        for (int j = 0; j < 4; j++)
            Softmax(reshape_boxtmp.data() + 16 * index++, 16);//inplace softamax

    std::vector<float> reshape_box2(16 * 4 * candidate_num);
    for (int i = 0; i < candidate_num; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 16; k++)
                reshape_box2[k * 4 * candidate_num + j * candidate_num + i] = reshape_boxtmp[i * 16 * 4 + j * 16 + k];

    //16 channels 1*1convolution
    std::vector<float> conv(4 * candidate_num, 0);
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < 4 * candidate_num; j++)
            conv[j] = conv[j] + reshape_box2[i * 4 * candidate_num + j] * i;

    std::vector<float>  concat(candidate_num * 4);
    for (int i = 0; i < candidate_num * 2; i++)
    {
        concat[i] = (conv[i + candidate_num * 2] - conv[i]) / 2.f + add_weight_[i < candidate_num ? match_index[i] : (match_index[i - candidate_num] + totol_size)] + 0.5;
        concat[i + candidate_num * 2] = (conv[i + candidate_num * 2] + conv[i]);      // add_data[i]-sub_data[i]) ;  
    }

    //concat the output
    float* output = output0->mutable_cpu_data();
    for (int i = 0; i < candidate_num; i++)
    {
        output[candidate_num * 0 + i] = concat[candidate_num * 0 + i] * mul_weight_[match_index[i]];
        output[candidate_num * 1 + i] = concat[candidate_num * 1 + i] * mul_weight_[match_index[i]];
        output[candidate_num * 2 + i] = concat[candidate_num * 2 + i] * mul_weight_[match_index[i]];
        output[candidate_num * 3 + i] = concat[candidate_num * 3 + i] * mul_weight_[match_index[i]];
#ifdef lxyyolo
        output[candidate_num * 4 + i] = sigmoid_x(cat[totol_size * category_mask[i] + match_index[i]]);
#else
        output[candidate_num * 4 + i] = sigmoid_x(cat[totol_size * 64 + totol_size * category_mask[i] + match_index[i]]);
#endif               
    }
    return  output0;
}

std::tuple<cv::Mat, float> preprocess_detection(cv::Mat& src, int& pad_h, int& pad_w, cv::Size input_shape = cv::Size(640, 640))
{
    float scale = std::min((float)input_shape.width / (float)src.cols, (float)input_shape.height / (float)src.rows);
    cv::Mat mask_image;
    if (src.rows != input_shape.height || src.cols != input_shape.width)
    {
        cv::resize(src, mask_image, input_shape, cv::INTER_LINEAR);
    }
    else
    {
        src.copyTo(mask_image);
    }
    cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2RGB);
    return { mask_image,scale };
}

void box_result_move_to_disjoint_region(std::vector<std::vector<float>>& sou_data, std::vector<int>& category_mask, int bias = 100000)
{
    for (size_t i = 0; i < sou_data.size(); i++)
        sou_data[i][0] = sou_data[i][0] + category_mask[i] * bias;
}

std::vector<std::vector<float>> XYXY2WH(std::shared_ptr<memory::tensor<float>>& net_result, int pad_h, int pad_w, float scale,
    int candicate_num, std::vector<int>& class_mask, float threshold = 0.0, float iou_thres = 0.8)
{
    std::vector<std::vector<float>> output;
    int shape = 5;
    const int candidate_num = candicate_num;
    std::shared_ptr<glasssix::memory::tensor<float>> dest
    (new glasssix::memory::tensor<float>(candidate_num, shape, -1, glasssix::memory::NCHW, nullptr));
    tranpose(net_result->cpu_data(), dest->mutable_cpu_data(), shape, candidate_num);
    const float* dest_ptr = dest->cpu_data();

    std::vector<cv::Rect2d> xywh_boxes;
    std::vector<std::vector<float>> key_points;
    std::vector<float> scores;

    for (int i = 0; i < candidate_num; i++)
    {
        std::vector<float> temp(5);
        cv::Rect2d boxwh;
        boxwh.x = static_cast<double>((dest_ptr[shape * i] - dest_ptr[shape * i + 2] / 2) - pad_w) * scale;
        boxwh.y = static_cast<double>((dest_ptr[shape * i + 1] - dest_ptr[shape * i + 3] / 2) - pad_h) * scale;
        boxwh.width = static_cast<double>(dest_ptr[shape * i + 2]) * scale;
        boxwh.height = static_cast<double>(dest_ptr[shape * i + 3]) * scale;

        temp[0] = boxwh.x;
        temp[1] = boxwh.y;
        temp[2] = boxwh.width;
        temp[3] = boxwh.height;
        temp[4] = dest_ptr[shape * i + 4];

        output.push_back(temp);
    }
    return output;
}

std::vector<int> nms_process(std::vector<std::vector<float>>& nms_input, float threshold = 0.0, float iou_thres = 0.9)
{
    std::vector<cv::Rect2d> xywh_boxes(nms_input.size());;
    std::vector<float> scores(nms_input.size());
    std::vector<int> indices_body(nms_input.size());;//候选框顺序

    for (size_t i = 0; i < nms_input.size(); i++)
    {
        cv::Rect2d boxwh;
        boxwh.x = nms_input[i][0];
        boxwh.y = nms_input[i][1];
        boxwh.width = nms_input[i][2];
        boxwh.height = nms_input[i][3];
        xywh_boxes[i] = boxwh;
        scores[i] = nms_input[i][4];
        indices_body[i] = i;
    }
    std::vector<int> indices_body_copy(indices_body.size());
    for (int i = 0; i < indices_body_copy.size(); i++)
        indices_body_copy[i] = i;
    cv::dnn::NMSBoxes(xywh_boxes, scores, threshold, iou_thres, indices_body_copy, 1.f, 0);

    return indices_body_copy;
}




#endif // !_GENERAL_HPP_