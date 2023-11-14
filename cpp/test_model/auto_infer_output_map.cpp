
#include "auto_infer_output_map.hpp"
#include <map>
#include <set>


std::map<std::string, std::string> auto_infer_output_map_(
	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& results_rknn,
	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& results_excb) 
{
	std::map<std::string, std::string> output_map;

	auto printOutsMap = [](const std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& rst_map, std::string info) {
		std::cout << info << " outs map : {" << std::endl;
		for (auto& node_p : rst_map) {
			std::cout << "\t\"" << node_p.first << '"' << ":" << printVect(node_p.second->data_shape()) << std::endl;
		}
		std::cout << "}" << std::endl;
	};

	std::cout << "\n-------------" << std::endl;
	printOutsMap(results_excb, "excalibur pipline");
	printOutsMap(results_rknn, "rknn pipline");
	std::cout << "-------------" << std::endl;

	// auto judge

	if (results_rknn.size() == 1 && results_excb.size() == 1) {
		auto& single_rknn = *results_rknn.begin();
		auto& single_excb = *results_excb.begin();
		if (single_rknn.second->count() == single_excb.second->count()) {
			output_map[single_excb.first] = single_rknn.first;
			return output_map;
		}
	}

	if (results_rknn.size() != results_excb.size()) return output_map;

	std::set<std::vector<int>> shapes_rknn;
	std::set<std::vector<int>> shapes_excb;

	auto shapes_statistics = [](const std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& rst_map, std::set<std::vector<int>>& shapes)->bool {
		for (auto& node : rst_map) {
			shapes.insert(node.second->data_shape());
		}

		return shapes.size() != rst_map.size();
	};

	if (shapes_statistics(results_rknn, shapes_rknn)) return output_map;
	if (shapes_statistics(results_excb, shapes_excb)) return output_map;

	if (shapes_rknn == shapes_excb) {
		for (auto& single_rknn : results_rknn) {
			for (auto& single_excb : results_excb) {
				if (single_excb.second->data_shape() == single_rknn.second->data_shape())
					output_map[single_excb.first] = single_rknn.first;
			}
		}

		return output_map;
	}

	return output_map;
}

bool auto_infer_output_map(glasssix::rknnwrapper::rknn_wrapper& rknn_pipeline,
	glasssix::excalibur::pipeline<float>& excalibur_pipeline, 
	std::string fimg, std::map<std::string, std::string>& output_map)
{
	cv::Mat img = cv::imread(fimg);
	std::shared_ptr<glasssix::memory::tensor<uint8_t>> input_tensor_u8(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, img.rows, img.cols, 3}, -1, glasssix::memory::NHWC));
	std::copy(img.data, img.data + img.step[0] * img.rows, input_tensor_u8->mutable_cpu_data());
	input_tensor_u8->convert_order();

	auto results_rknn = rknn_pipeline.forward(img.data, { 1, img.rows, img.cols, 3 }, RKNN_TENSOR_NHWC);
	auto results_excalibur = excalibur_pipeline.forward(input_tensor_u8 | glasssix::memory::tensor_convert_to<float>);
	auto automic_map = auto_infer_output_map_(results_rknn, results_excalibur);
	if (automic_map.empty()) {
		std::cout << "-- failed to get output map automatically !" << std::endl;
		std::cout << "-- please specify output map regulation (xx.json) by hand" << std::endl;
		std::cout << "// xx.json" << std::endl;
		std::cout << "{" << std::endl;
		std::cout << "	\"output\":" << std::endl;
		std::cout << "	{" << std::endl;
		std::cout << "		\"excalibur_ouput_1\":\"rknn_ouput_1\"" << std::endl;
		std::cout << "	}" << std::endl;
		std::cout << "}" << std::endl;
		return false;
	}
	else {
		output_map = automic_map;
		std::cout << "succeed to get output map automatically ~" << std::endl;
		std::cout << "<auto_map> excalibur - rknn " << std::endl;

		for (auto output_map_elm : output_map) {
			std::cout << "\t" << output_map_elm.first << ':' << printVect(results_excalibur[output_map_elm.first]->data_shape())
				<< " - " << output_map_elm.second << ':' << printVect(results_rknn[output_map_elm.second]->data_shape()) << std::endl;
		}
		std::cout << "</auto_map>" << std::endl;
		std::cout << "-------------" << std::endl;
	}

	return 0;
}