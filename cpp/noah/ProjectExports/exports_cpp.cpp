#include "../Project.hpp"
#include <iostream>
#include <string>

void noah::Projcet::exports_cpp(const std::string& file_path) {
	FILE* exp_file = fopen((file_path + "exports.cpp").c_str(), "wb");
	TXTLines txtlines;
	std::vector<std::string> impl_classes;
	for (const auto& cls : ExpClasses_) {
		impl_classes.push_back(cls.get_class_name() + "_impl");
		txtlines.push_include(cls.get_class_name() + "_impl.hpp");
	}
	txtlines.push();
	txtlines.push("#include <abi/abi_standard_export.hpp>");
	txtlines.push();
	std::string exports_command("MAKE_ABI_STANDARD_EXPORT_FUNCTIONS(u8\"g6.library.algorithmZoo." + this->module_name + "\", ");
	for (auto& ipcls : impl_classes) {
		std::string region = "glasssix::" + this->module_name + "::";
		exports_command += region + ipcls + ", ";
	}
	exports_command = exports_command.substr(0, exports_command.find_last_of(','));
	exports_command += ')';

	txtlines.push(exports_command);

	fprintf(exp_file, "%s", txtlines.export_string().c_str());
	fclose(exp_file);
}

