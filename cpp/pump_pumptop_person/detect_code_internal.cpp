#include "detect_code_internal.hpp"
#include "box_info_internal.hpp"
#include "box_info_impl.hpp"
#include <algorithm>
#include <numeric>

#include <opencv2/opencv.hpp>
#include <GenPipeline/PrePostProcessGenPipeline.hpp>
#include <GenPipeline/GetPostprocessing.hpp>
#include "../genpipeline/market/yolov8_GEN.hpp"
#if defined(USE_BMNN)
#include <sophonyolov8/SophonYolov8Wrapper.hpp>
#endif

namespace glasssix::pump_pumptop_person
{
    class detect_code_internal::impl
    {
    public:
        impl() {}
        impl(std::string_view model_directory, int device) :impl()
        {
            std::string model_dir = exposing::to_narrow_string(model_directory);
            if (*model_dir.rbegin() != '/') model_dir += '/';
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            ioprocess_pipeline_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "pumptop_person.rknn", 0);
            ioprocess_pipeline_->manual_possible_normalization(0, 1.f / 255);
            ioprocess_pipeline_->set_postprocessing(yolov8_GEN<1, 1>);
#elif defined(USE_BMNN)
            ioprocess_pipeline_ = std::make_shared<SophonYolov8Wrapper>(model_dir + "pumptop_person.bmodel");
            ioprocess_pipeline_->init();
#endif
        }

        exposing::param_vector<pump_pumptop_person::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int height, int width, const std::vector<PedestrianInfo>& pedestrian_info_list, std::map<std::string,float>& param_map_std)
        {
            auto result = exposing::make_param_vector<pump_pumptop_person::box_info>();

            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(bitmap.size(), 3 * height * width);
            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));

            float conf_thres = param_map_std.count("conf_thres") ? param_map_std["conf_thres"] : 0.7f;
            float person_area_ratio_thres = param_map_std.count("person_area_ratio_thres") ? param_map_std["person_area_ratio_thres"] : 0.2f;

            auto pumptop_list = pumptop_detect(image, conf_thres);
            
            for (auto& pump_rrect_crop : pumptop_list) {

                cv::RotatedRect RotatePump = pump_rrect_crop.getRotatePump(-15);
                std::vector<cv::Point2f> pump_polygon= RotatedRect2Polygon(RotatePump);

                for (auto pedestrian_info : pedestrian_info_list) {
					auto person_box_crop = pedestrian_info.get_rect();

                    auto low_person_point = cv::Point((pedestrian_info.x1 + pedestrian_info.x2) / 2, pedestrian_info.y2);

                    bool is_person_in_pump_regions = isPointInsidePolygon(low_person_point, pump_polygon);
                    
					box_info_internal person_state;
					person_state.x1 = pedestrian_info.x1;
					person_state.y1 = pedestrian_info.y1;
					person_state.x2 = pedestrian_info.x2;
					person_state.y2 = pedestrian_info.y2;
					person_state.score = pedestrian_info.score;
					person_state.pump = exposing::make_param_vector<std::int32_t>();

					// install pump location
					for (auto pump_point: pump_polygon) {
						person_state.pump.push_back(int(pump_point.x));
						person_state.pump.push_back(int(pump_point.y));
					}

                    constexpr int OUT_PUMP = 0;//不在泵内
                    constexpr int IN_PUMP = 1;//在泵（均在在四边形内和掩码内）
                    constexpr int OUT_MASK = 2;//在四边形内，但不在掩码内
                    constexpr int MAN_INTERSECT_AREA_LITTE = 3;//人和泵框相交度不足，认为不在泵内

					person_state.category = is_person_in_pump_regions ? IN_PUMP : OUT_PUMP;

					if (person_state.category == IN_PUMP) {
                        auto intersection_area = get_intersection_area(person_box_crop, RotatePump);
                        float in_area_ratio = intersection_area / person_box_crop.area();

                        if (in_area_ratio < person_area_ratio_thres) {
                            person_state.category = MAN_INTERSECT_AREA_LITTE;
                        }
                    }

                    result.push_back(glasssix::exposing::make_as_first<box_info_impl>(person_state));
                }
            }

            return result;
        }

        struct PptopBBox :public GenPipTools::YoloBoxBase {
        public:
            using YoloBoxBase::YoloBoxBase; //Inheriting Constructors

            cv::RotatedRect getRotatePump(float angleDegrees) {
                cv::Rect rect = this->get_rect();

                cv::Point2f center = this->get_center();
                // cv::RotatedRect requires a rotation angle (counterclockwise as positive),
                // while getRotationMatrix2D requires a rotation angle (counterclockwise as negative), so negate it here.
                float rotationAngle = -angleDegrees;
                float scale = 1.0f;
                cv::Mat rotationMatrix = cv::getRotationMatrix2D(center, rotationAngle, scale);
                cv::RotatedRect rotatedRect(center, cv::Size2f(rect.width, rect.height), angleDegrees);
                return rotatedRect;
            }
        };

        std::vector<PptopBBox> pumptop_detect(cv::Mat& image, float conf_thres) {

            const int letter_h = 736;
            const int letter_w = 1280;
            std::vector<PptopBBox> box_list;

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            GenPipTools::LetterInfo letter_op;
            auto letter_img = GenPipTools::letter_image(image, letter_w, letter_h, letter_op, true);
            auto tensor_out = ioprocess_pipeline_->forward(letter_img).begin()->second;
            const int vf_nums = tensor_out->height(); //vf, visual field
            const int per_vf_len = tensor_out->width();
            for (size_t idx = 0; idx < vf_nums; idx++) {
                float* pdata = tensor_out->mutable_cpu_data() + idx * per_vf_len;
                float conf = pdata[4];
                if (conf > conf_thres) {
                    PptopBBox obj_box(pdata[0] * letter_w, pdata[1] * letter_h, pdata[2] * letter_w, pdata[3] * letter_h, conf, 0);
                    box_list.push_back(obj_box);
                }
            }
            GenPipTools::nms_cpu(box_list, 0.6);
            GenPipTools::letter_map_origin_location(box_list, letter_op);
#elif defined(USE_BMNN)
            auto cropped_result = ioprocess_pipeline_->get_objects(image, conf_thres, 0.6);// 泵检测
            for (auto& object : cropped_result)
            {
                PptopBBox obj_box((object.x1 + object.x2) * 0.5, (object.y1 + object.y2) * 0.5, object.x2 - object.x1, object.y2 - object.y1, object.score, 0);
                box_list.push_back(obj_box);
            }
#endif  
            return box_list;
        }

        std::vector<cv::Point2f> RotatedRect2Polygon(cv::RotatedRect rrect) {
            cv::Point2f vertices[4];
            rrect.points(vertices);
            std::vector<cv::Point2f> polygon;
            for (int i = 0; i < 4; ++i) {
                polygon.push_back(vertices[i]);
            }
            return polygon;
        }

        bool isPointInsidePolygon(const cv::Point& point, const std::vector<cv::Point2f>& polygon) {
            // If the result is greater than 0, the point is inside polygon
            return cv::pointPolygonTest(polygon, point, true) > 0;
        }

        bool isPointInsideRotatedRect(const cv::Point& point, const cv::RotatedRect& rrect) {
            return isPointInsidePolygon(point, RotatedRect2Polygon(rrect));
        }

        float get_intersection_area(cv::Rect& person, cv::RotatedRect& pump) {
            cv::Point2f center(person.x + person.width / 2, person.y + person.height / 2);
            cv::Size2f size(person.width, person.height);
            float angle = 0.0f;
            cv::RotatedRect rperson(center, size, angle);

            std::vector<cv::Point2f> intersectingRegion;
            auto ic = cv::rotatedRectangleIntersection(rperson, pump, intersectingRegion);
            if (ic <= 0) {
                return 0.f;
            }
            else {
                auto area = cv::contourArea(intersectingRegion);
                return area;
            }
        }

		std::string version()
		{
			const std::string algo_module_version = "2.0.0";
			std::string nn_frame_version = "2.0.0";
			return fmt::format(R"({ {"nn_frame_version":"{}", "algo_module_version" : "{}"} })", nn_frame_version, algo_module_version);
		}

    private:
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        std::shared_ptr<PrePostProcessGenPipeline> ioprocess_pipeline_;
#elif defined(USE_BMNN)
        std::shared_ptr<SophonYolov8Wrapper> ioprocess_pipeline_;
#endif
    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal()
    {
    }

    exposing::param_vector<pump_pumptop_person::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int height, int width, const std::vector<PedestrianInfo>& pedestrian_info_list, std::map<std::string,float>& param_map_std)
    {
        return impl_->detect(bitmap, height, width, pedestrian_info_list, param_map_std);
    }

    std::string detect_code_internal::version()
    {
        return impl_->version();
    }
}
