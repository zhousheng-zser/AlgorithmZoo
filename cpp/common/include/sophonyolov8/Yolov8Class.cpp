//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-DEMO is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#include "Yolov8Class.hpp"

#define USE_ASPECT_RATIO 1
#define DUMP_FILE 0
#define USE_MULTICLASS_NMS 1


  Yolov8Class::Yolov8Class(std::shared_ptr<BMNNContext> context)
    : YoloV8(context), posturetype(PostureType::K17)
    {
        std::cout<<"Yolov8Class construct \n";
        PostureKeyinfo = {{PostureType::K17, 17}, {PostureType::K12, 12}};
    }

    void Yolov8Class::setyolov8_type(PostureType yolov8type)
    {
        this->posturetype = yolov8type;
    }

    int Yolov8Class::post_process(const bm_image& image, YoloV8BoxVec& detected_boxes,float conf,float nms_thresh) {
    YoloV8BoxVec yolobox_vec;
    std::vector<cv::Rect> bbox_vec;
    std::vector<std::shared_ptr<BMNNTensor>> outputTensors(output_num);
    // std::cout<<"Yolov8Class post_process \n";
    for (int i = 0; i < output_num; i++) {
        outputTensors[i] = m_bmNetwork->outputTensor(i);
    }

    // for (int batch_idx = 0; batch_idx < images.size(); ++batch_idx) 
    {
        yolobox_vec.clear();
        auto& frame = image;
        int frame_width = frame.width;
        int frame_height = frame.height;

        int min_idx = 0;
        int box_num = 0;

        // Single output
        auto out_tensor = outputTensors[min_idx];
        
        // if(posturetype == PostureType::K17 )

        m_class_num = out_tensor->get_shape()->dims[1] - mask_num - 4 - PostureKeyinfo[posturetype] * 3;
        
        int feat_num = out_tensor->get_shape()->dims[2]; //19320
        float* output_data = nullptr;
        // LOG_TS(m_ts, "post 1: get output");
        assert(box_num == 0 || box_num == out_tensor->get_shape()->dims[1]);
        // box_num = out_tensor->get_shape()->dims[1];
        output_data = (float*)out_tensor->get_cpu_data() + 0 * feat_num * (m_class_num + mask_num + 4);
        // LOG_TS(m_ts, "post 1: get output");

        // Candidates
        // LOG_TS(m_ts, "post 2: get detections matrix nx6 (xyxy, conf, cls)");
       
        float* cls_conf = output_data + 4 * feat_num;
        for (int i = 0; i < feat_num; i++) {
#if USE_MULTICLASS_NMS
            // multilabel
            for (int j = 0; j < m_class_num; j++) {
                float cur_value = cls_conf[i + j * feat_num];
                
                if (cur_value >= conf) {
                    
                    std::vector<sophonkey_point> key_points;
                    YoloV8Box box;
                    box.score = cur_value;
                    box.class_id = j;
                    int c = box.class_id * max_wh;
                    float centerX = output_data[i + 0 * feat_num];
                    float centerY = output_data[i + 1 * feat_num];
                    float width = output_data[i + 2 * feat_num];
                    float height = output_data[i + 3 * feat_num];
                    for (size_t k = 0; k < PostureKeyinfo[posturetype]  ; k++)
                    {
                        float posture_x =  output_data[i + (4 + m_class_num + k *3) * feat_num];
                        float posture_y =  output_data[i + (5 + m_class_num + k *3) * feat_num];
                        float posture_score = output_data[i + (6 + m_class_num + k *3) * feat_num];

                        sophonkey_point ypkp(posture_x, posture_y, posture_score);
                        key_points.push_back(ypkp);
                        // std::cout<< posture_x<<" "<<posture_y<<" "<<posture_score<<std::endl;
                    }

                    box.x1 = centerX - width / 2 + c;
                    box.y1 = centerY - height / 2 + c;
                    box.x2 = box.x1 + width;
                    box.y2 = box.y1 + height;
                    box.key_points = key_points;
                    yolobox_vec.push_back(box);
                }
            }
#else
            // best class
            float max_value = 0.0;
            int max_index = 0;
            for (int j = 0; j < m_class_num; j++) {
                float cur_value = cls_conf[i + j * feat_num];
                if (cur_value > max_value) {
                    max_value = cur_value;
                    max_index = j;
                }
            }

            if (max_value >= conf) {
                YoloV8Box box;
                box.score = max_value;
                box.class_id = max_index;
                int c = box.class_id * max_wh;
                float centerX = output_data[i + 0 * feat_num];
                float centerY = output_data[i + 1 * feat_num];
                float width = output_data[i + 2 * feat_num];
                float height = output_data[i + 3 * feat_num];
                for (size_t k = 0; k < PostureKeyinfo[posturetype]  ; k++)
                {
                    float posture_x =  output_data[i + (4 + m_class_num + k *3) * feat_num];
                    float posture_y =  output_data[i + (5 + m_class_num + k *3) * feat_num];
                    float posture_score = output_data[i + (6 + m_class_num + k *3) * feat_num];
                    // std::cout<< posture_x<<" "<<posture_y<<" "<<posture_score<<std::endl;
                }

                box.x1 = centerX - width / 2 + c;
                box.y1 = centerY - height / 2 + c;
                box.x2 = box.x1 + width;
                box.y2 = box.y1 + height;

                yolobox_vec.push_back(box);
            }
#endif
        }
        // LOG_TS(m_ts, "post 2: get detections matrix nx6 (xyxy, conf, cls)");

        // LOG_TS(m_ts, "post 3: nms");
        NMS(yolobox_vec, nms_thresh);

        if (yolobox_vec.size() > max_det) {
            yolobox_vec.erase(yolobox_vec.begin(), yolobox_vec.begin() + (yolobox_vec.size() - max_det));
        }

        for (int i = 0; i < yolobox_vec.size(); i++) {
            int c = yolobox_vec[i].class_id * max_wh;
            yolobox_vec[i].x1 = yolobox_vec[i].x1 - c;
            yolobox_vec[i].y1 = yolobox_vec[i].y1 - c;
            yolobox_vec[i].x2 = yolobox_vec[i].x2 - c;
            yolobox_vec[i].y2 = yolobox_vec[i].y2 - c;
        }

        float tx1 = 0, ty1 = 0;
        bool isAlignWidth = false;
        float ratio = get_aspect_scaled_ratio(frame.width, frame.height, m_net_w, m_net_h, &isAlignWidth);
        if (isAlignWidth) {
            ty1 = (m_net_h - (float)(frame_height * ratio)) / 2;
        } else {
            tx1 = (m_net_w - (float)(frame_width * ratio)) / 2;
        }
        for (int i = 0; i < yolobox_vec.size(); i++) {
            float centerx = ((yolobox_vec[i].x2 + yolobox_vec[i].x1) / 2 - tx1) / ratio;
            float centery = ((yolobox_vec[i].y2 + yolobox_vec[i].y1) / 2 - ty1) / ratio;
            float width = (yolobox_vec[i].x2 - yolobox_vec[i].x1) / ratio;
            float height = (yolobox_vec[i].y2 - yolobox_vec[i].y1) / ratio;
            yolobox_vec[i].x1 = centerx - width / 2;
            yolobox_vec[i].y1 = centery - height / 2;
            yolobox_vec[i].x2 = centerx + width / 2;
            yolobox_vec[i].y2 = centery + height / 2;
            for (size_t j = 0; j < PostureKeyinfo[posturetype] ; j++)
            {
                yolobox_vec[i].key_points[j].x = (yolobox_vec[i].key_points[j].x - tx1) / ratio;
                yolobox_vec[i].key_points[j].y = (yolobox_vec[i].key_points[j].y - ty1) / ratio;
            }
        }
        clip_boxes(yolobox_vec, frame_width, frame_height);

        // LOG_TS(m_ts, "post 3: nms");
        detected_boxes = yolobox_vec;
        // detected_boxes.push_back(yolobox_vec);
    }

    return 0;
}

