#include "../postprocessing_register.hpp"
#include <vector>

PostprocessingRegister::PostprocessingRegister(std::shared_ptr<Postprocessing> pplugin) {
	postprocessing_list_instance().push_back(pplugin);
}

std::vector<std::shared_ptr<Postprocessing>>& postprocessing_list_instance()
{
	static std::vector<std::shared_ptr<Postprocessing>> postprocessing_list;
	return postprocessing_list;
}

std::map<std::string, PostprocessingFunction> AddPostprocessing()
{
	std::map<std::string, PostprocessingFunction> postprocessing_map;
	AddPostprocessing(postprocessing_map);
	return postprocessing_map;
}

void AddPostprocessing(std::map<std::string, PostprocessingFunction>& postprocessing_map)
{
	for (auto p : postprocessing_list_instance())
	{
		auto tmp_pproce = p->parser_postprocessing_dump();
		postprocessing_map.insert(tmp_pproce.begin(), tmp_pproce.end());
	}
}

void DumpShowPostprocessingMarket(const std::map<std::string, PostprocessingFunction>& postprocessing_market)
{
	printf("[INFO] POSTPROCESSING MARKET[%d]: { ", postprocessing_market.size());
	for (auto&& ppf : postprocessing_market) std::cout << ppf.first << ", ";
	std::cout << "}" << std::endl;
}

