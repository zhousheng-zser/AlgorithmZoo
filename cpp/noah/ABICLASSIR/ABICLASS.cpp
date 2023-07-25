#include "../ABICLASSIR.hpp"
#include "../string_process.hpp"

noah::ABICLASS::ABICLASS() {
	cls_type_ = ClassType::AstClass;
};

noah::ABICLASS::ABICLASS(const ABICLASS& cls) {
	this->class_name_ = cls.class_name_;
	this->functions = cls.functions;
	this->plugin_name_ = cls.plugin_name_;
	this->cls_type_ = cls.cls_type_;
}

noah::ABICLASS::ABICLASS(const std::string& p_name) : ABICLASS() {
	plugin_name_ = p_name;
};

noah::ABICLASS::ABICLASS(std::string p_name, std::string c_name) : ABICLASS() {
	plugin_name_ = p_name;
	class_name_ = c_name;
};

noah::ABICLASS::ABICLASS(std::string p_name, std::string c_name, ClassType AbiClsType) {
	plugin_name_ = p_name;
	class_name_ = c_name;
	cls_type_ = AbiClsType;
};

