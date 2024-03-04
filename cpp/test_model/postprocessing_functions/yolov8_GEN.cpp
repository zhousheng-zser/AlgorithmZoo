#include "../postprocessing_register.hpp"
#include "yolov8_GEN.hpp"

class pp_yo8_gen : public Postprocessing
{
public:
	virtual const std::map<std::string, PostprocessingFunction> parser_postprocessing_dump() const override
	{
		std::map<std::string, PostprocessingFunction> pp_map;

		/// general yolov8 net out raw-location(64) + raw-scores(N)
		pp_map["yolov8_loc_c1"] = &yolov8_GEN<1>;
		pp_map["yolov8_loc_c2"] = &yolov8_GEN<2>;
		//pp_map["yolov8_loc_c3"] = &yolov8_GEN<3>;
		//pp_map["yolov8_loc_c4"] = &yolov8_GEN<4>;

		/// lixinyao net type raw-scores(N) + raw-location(64)
		pp_map["yolov8_c1_loc"] = &yolov8_GEN<1, false>;
		pp_map["yolov8_c2_loc"] = &yolov8_GEN<2, false>;
		//pp_map["yolov8_c3_loc"] = &yolov8_GEN<3, false>;
		//pp_map["yolov8_c4_loc"] = &yolov8_GEN<4, false>;

		/// if category more 3, define by yourself ...

		return pp_map;
	}
};

REGISTE_POSTPROCESSING(pp_yo8_gen)
