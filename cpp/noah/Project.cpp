#include "Project.hpp"
#include <iostream>
#include <string>
#include "string_process.hpp"

void noah::Projcet::init_(std::string name) {
	if (!name.empty()) {
		this->module_name = name;
		std::cout << "[ INFO ][ Projcet::init ] Create Projcetion ~" << std::endl;
	}
	else {
		std::cout << "[ ERROR ][ Projcet::init ] Projcetion name empyt !" << std::endl;
	}
}

noah::Projcet::Projcet() {}

noah::Projcet::~Projcet() {}

void noah::Projcet::push_cls_(const ABICLASS& cls) {
	auto status = cls.get_status();
	if (this->module_name == cls.get_plugin_name()) {
		if (status == ClassType::ExpClass)
			ExpClasses_.push_back(cls);
		else
			AstClasses_.push_back(cls);
	}
	else {
		std::cout << "[ WARN ][ Projcet::push_cls ] Class not belong projection !" << std::endl;
	}
}

void noah::Projcet::write_code_(std::string file_path) {
	if (*file_path.rbegin() != '/') file_path += '/';

	exports_cpp(file_path);
	make_abi_hpp(file_path);
	make_impl_hpp(file_path);
	make_impl_cpp(file_path);
	make_internal_hpp(file_path);
	make_internal_cpp(file_path);

}
void noah::Projcet::write_cmake_(std::string file_path) {
	if (*file_path.rbegin() != '/') file_path += '/';
	CMakeLists_txt(file_path);
}

std::string noah::maybe_MapParamName_ABI_2_STD_(const noah::CData& maybe_map_param) {
	if (maybe_map_param.nestype[0].basic_t == noah::BasicAType::AT_MAP) {
		std::string map_param_std_name = maybe_map_param.name;
		noah::removeWord(map_param_std_name, "abi");
		noah::removeWord(map_param_std_name, "std");
		map_param_std_name += "std";
		return map_param_std_name;
	}
	else {
		return maybe_map_param.name;
	}
}