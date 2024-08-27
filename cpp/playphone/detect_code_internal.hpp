#ifndef __PLAYPHONE_DETECT_CODE_INTERNAL_HPP__
#define __PLAYPHONE_DETECT_CODE_INTERNAL_HPP__

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>
#include <opencv2/core.hpp>
#include "box_info.hpp"
#include "ObjBox.hpp"

namespace glasssix::playphone
{
	struct box_info_internal
	{
		int x1;
		int y1;
		int x2;
		int y2;
		int category = -1;//0 man with phone; 1 man; 2 man bodyerr; -1 unknow; 3 追踪中被删掉的人
		float confidence;
		int frequency = -1; //检测到玩手机的次数
		int id = -1;// 人对应的id

		exposing::param_vector<std::int32_t> phonelocal_list;
		exposing::param_vector<float> phonescore_list;

		box_info_internal() {
			phonelocal_list = exposing::make_param_vector<std::int32_t>();
			phonescore_list = exposing::make_param_vector<float>();
		}

		void set_man(PostureInfo& man) {
			auto man_regio = man.get_rect();
			x1 = man_regio.x;
			y1 = man_regio.y;
			x2 = man_regio.x + man_regio.width;
			y2 = man_regio.y + man_regio.height;
			confidence = man.score;
			category = 1;
		}

		void set_phone(PhoneBox& phone) {
			phonelocal_list.clear();
			phonescore_list.clear();
			phonelocal_list.push_back(phone.xmin);
			phonelocal_list.push_back(phone.ymin);
			phonelocal_list.push_back(phone.xmax);
			phonelocal_list.push_back(phone.ymax);
			phonescore_list.push_back(phone.score);

			category = 0;
		}

		void set_body_error(PostureInfo& man) {
			phonelocal_list.clear();
			phonescore_list.clear();
			//face
			phonescore_list.push_back(man.Kpoints_score[0]);
			phonescore_list.push_back(man.Kpoints_score[1]);
			phonescore_list.push_back(man.Kpoints_score[2]);
			//hand
			phonescore_list.push_back(man.Kpoints_score[9]);
			phonescore_list.push_back(man.Kpoints_score[10]);
			//set flag
			category = 2;
		}
	};

    class detect_code_internal
    {
    public:
        class impl;

        /// <summary>
        /// Creates an instance with a specified GPU core or the default CPU.
        /// </summary>
        /// <param name="racy_path">The model path</param>
        /// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
        detect_code_internal(std::string_view model_directory, int device);

        virtual ~detect_code_internal();

        detect_code_internal(const detect_code_internal&) = delete;
        detect_code_internal& operator=(const detect_code_internal&) = delete;

        std::string version();

        exposing::param_vector<playphone::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list, std::map<std::string, float>& param_map) const;

    private:
        std::unique_ptr<impl> impl_;
    };
}
#endif