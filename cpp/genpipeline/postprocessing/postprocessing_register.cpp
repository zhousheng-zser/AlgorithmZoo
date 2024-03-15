#include "postprocessing_register.hpp"
#include <vector>

PostprocessingRegister::PostprocessingRegister(std::shared_ptr<Postprocessing> pplugin) {
	postprocessing_instance().push_back(pplugin);
}

std::vector<std::shared_ptr<Postprocessing>>& PostprocessingRegister::postprocessing_instance()
{
	static std::vector<std::shared_ptr<Postprocessing>> postprocessing_list;
	return postprocessing_list;
}