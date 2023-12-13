#include "postprocessing_register.hpp"
#include <vector>

PostprocessingRegister::PostprocessingRegister(std::shared_ptr<Postprocessing> pplugin) {
	postprocessing_list_instance().push_back(pplugin);
}

std::vector<std::shared_ptr<Postprocessing>>& postprocessing_list_instance()
{
	static std::vector<std::shared_ptr<Postprocessing>> postprocessing_list;
	return postprocessing_list;
}

void AddPostprocessing(std::map<std::string, postprocessing_function>& postprocessing_map)
{
	for (auto p : postprocessing_list_instance())
	{
		auto tmp_pproce = p->parser_postprocessing_dump();
		postprocessing_map.insert(tmp_pproce.begin(), tmp_pproce.end());
	}
}


