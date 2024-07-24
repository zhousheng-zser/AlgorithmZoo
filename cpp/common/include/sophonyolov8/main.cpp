//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-DEMO is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//
// #include <fstream>
// #include <string.h>
// #include <dirent.h>
// #include <unistd.h>
// #include <sys/stat.h>
// #include "json.hpp"
// #include "opencv2/opencv.hpp"
// #include "ff_decode.hpp"
// #include "yolov8.hpp"
// #include <chrono>
// #include "SophonYolov8Wrapper.hpp"
// using json = nlohmann::json;
// using namespace std;

// #define DEBUG 1

//     int main()
//     {


//         std::string infer_image ="/home/linaro/cw/test/img/pedestrian.jpg";
//         string bmodel_file =std::string("/home/linaro/cw/updated_model_with_constant.bmodel");
//         int device =0;

//         SophonYolov8Wrapper  infer(bmodel_file);
//         infer.init(0.2);
//  auto start = std::chrono::high_resolution_clock::now();
 
//  for (size_t i = 0; i < 100; i++)
//  {
//         cv::Mat opemcv_image =  cv::imread(infer_image);
//         auto results = infer.get_objects(opemcv_image);
//         for(auto var : results)
//         {
//             std::cout<<var.x1<<" "<<var.y1<<" "<<var.x2<<" "<<var.y2<<" "<<var.score<<std::endl;
//         }
    
//  }


//         auto end = std::chrono::high_resolution_clock::now();
//         std::chrono::duration<float> duration = end - start; //记录经过了多长时间
//         std::cout << duration.count()/100 << "sssss" << std::endl; //输出运行时间


//     // std::cout<<"heelo\n";
//     // int dev_id = 0;
//     // string bmodel_file =std::string("/home/linaro/cw/updated_model_with_constant.bmodel");
//     // BMNNHandlePtr handle = make_shared<BMNNHandle>(dev_id);
//     // bm_handle_t h = handle->handle();

//     // // load bmodel
//     // shared_ptr<BMNNContext> bm_ctx = make_shared<BMNNContext>(handle, bmodel_file.c_str());

//     // float conf_thresh = 0.2;
//     // float nms_thresh = 0.6;
//     // // initialize net
//     // YoloV8 yolov8(bm_ctx);
//     // CV_Assert(0 == yolov8.Init(
//     //     conf_thresh,
//     //     nms_thresh));

//     // // get batch_size
//     //     std::cout<<"heelo111\n";
//     // // int batch_size = yolov8.batch_size();

//     //  auto start = std::chrono::high_resolution_clock::now();
    
    
//     // std::string infer_image ="/home/linaro/cw/test/img/pedestrian.jpg";
//     //     std::cout<<"heel2\n";

//     // vector<bm_image> batch_imgs;
//     // vector<string> batch_names;
//     // YoloV8BoxVec boxes;
//     // vector<json> results_json;
//     // // int cn = files_vector.size();
//     // int id = 0;
//     // for (size_t i = 0; i < 10; i++)
//     // {
//     //     string img_file =infer_image; 
//     //     id++;

//     //     bm_image bmimg;

//     //     // picDec(h, img_file.c_str(), bmimg);

//     //     std::cout<<"bmimg.  image_format: " << bmimg.image_format<<std::endl;
//     //     std::cout<<"bmimg.  data_type:    " << bmimg.data_type<<std::endl;

//     //     cv::Mat opemcv_image =  cv::imread(infer_image);
//     //     // bm_image_from_mat(bm_handle_, opemcv_image, bmimg);
//     //     cv::bmcv::toBMI(opemcv_image, &bmimg, true);
        
//     //     {
//     //         // predict
//     //         CV_Assert(0 == yolov8.Detect(bmimg, boxes));

//     //         for(int i = 0; i < 1; i++){
//     //             vector<json> bboxes_json;

//     //             if(boxes.size() != 0){
//     //                 for (auto bbox : boxes) {
//     //                     // draw image
//     //                     float bboxwidth = bbox.x2-bbox.x1;
//     //                     float bboxheight = bbox.y2-bbox.y1;
//     //                     // draw image
//     //                     // if(bbox.score > 0.25)
//     //                     //     yolov8.draw_bmcv(h, bbox.class_id, bbox.score, bbox.x1, bbox.y1, bboxwidth, bboxheight, bmimg);
//     //                     // save result
//     //                     json bbox_json;
//     //                     bbox_json["category_id"] = bbox.class_id;
//     //                     bbox_json["score"] = bbox.score;
//     //                     bbox_json["bbox"] = {bbox.x1, bbox.y1, bboxwidth, bboxheight};
//     //                     bboxes_json.push_back(bbox_json);
//     //                     std::cout<<"class_id: "<<bbox.class_id<<std::endl;
//     //                     std::cout<<"bbox.score: "<<bbox.score<<std::endl;
//     //                     std::cout<<"bbox.x y: "<<bbox.x1<<" "<<bbox.x2<<" "<<bbox.y1<<" "<<bbox.y2<<" "<<std::endl;
//     //                 }
//     //             }

//     //             json res_json;
//     //             // res_json["image_name"] = batch_names[i];
//     //             res_json["bboxes"] = bboxes_json;
//     //             results_json.push_back(res_json);

//     //             bm_image_destroy(bmimg);
//     //         }

//     //         batch_imgs.clear();
//     //         batch_names.clear();
//     //         boxes.clear();
//     //     }

//     // }

//     // auto end = std::chrono::high_resolution_clock::now();
//     // std::chrono::duration<float> duration = end - start; //记录经过了多长时间
//     // std::cout << duration.count() << "sssss" << std::endl; //输出运行时间

//     return 0;
// }