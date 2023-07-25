#include "../Project.hpp"
#include <iostream>
#include <string>


template<noah::ClassType ClsTyp_>
void noah::Projcet::write_impl_hpp_(const std::string& file_path, const ABICLASS& Cls) {
	std::string this_internal_cls = Cls.get_class_name() + "_internal";
	std::string cls_impl_name = Cls.get_class_name() + "_impl";
	std::string qualified_name = this->module_name + "_" + Cls.get_class_name()+ "_qualified_name";
	FILE* fp = fopen((file_path + Cls.get_class_name() + "_impl.hpp").c_str(), "wb");

	TXTLines txtlines;

	txtlines.push("#pragma once");
	txtlines.push("");
	txtlines.push_include(Cls.get_class_name() + ".hpp"); // inclu = "file", 强制指定后缀

	if (ClsTyp_ == ClassType::ExpClass) {
		txtlines.push("#include <memory>");
		txtlines.push("#include <abi/consumer.hpp>");
	}
	else {
		txtlines.push("#include \"", this_internal_cls,".hpp\"");
	}

	txtlines.push("");
	txtlines.push("namespace glasssix::" + this->module_name);
	txtlines.push("{");
	txtlines.push<1>("inline constexpr exposing::utf8_string_view " +
		qualified_name + "{ u8\"g6." + this->module_name + "." + Cls.get_class_name() + "\" };");

	txtlines.push("");
	if (ClsTyp_ == ClassType::ExpClass)
		txtlines.push<1>("class " + this_internal_cls, ';'); // declare impl_ type

	txtlines.push("");
	txtlines.push<1>("class " + cls_impl_name +
		" : public exposing::implements<" + cls_impl_name + ", " + Cls.get_class_name() + ">, " +
		"public exposing::make_external_qualified_name<" + qualified_name + ">");
	txtlines.push<1>("{");

	txtlines.push<1>("public:");
	txtlines.push<2>(cls_impl_name + "();");

	if (ClsTyp_ == ClassType::AstClass) {
		txtlines.push<2>(cls_impl_name + "(const " + this_internal_cls + " &internal);");//单纯的辅助类，需要拷贝构造
	}

	txtlines.push<2>('~' + cls_impl_name + "();");
	for (auto& abi_func : Cls.functions) {
		if (abi_func.special_function_check() == noah::Function::functionType::version) {
			txtlines.push<2>("exposing::param_string version() const;");
		}
		else {
			std::string func_str = abi_func.impl_function();
			if (*func_str.rbegin() != '; ') func_str += ';';
			txtlines.push<2>(func_str);
		}
	}
	txtlines.push("");
	txtlines.push<1>("private:");

	if (ClsTyp_ == ClassType::ExpClass)
		txtlines.push<2>("std::unique_ptr<" + this_internal_cls + "> impl_;");
	else
		txtlines.push<2>(this_internal_cls + " internal_;");

	txtlines.push<1>("};");
	txtlines.push("}");

	fprintf(fp, "%s", txtlines.export_string().c_str());
	fclose(fp);
}


void noah::Projcet::make_impl_hpp(const std::string& file_path) {
	for (auto& ExpCls : ExpClasses_) {
		write_impl_hpp_<ClassType::ExpClass>(file_path, ExpCls);
	}
	for (auto& AstCls : AstClasses_) {
		write_impl_hpp_<ClassType::AstClass>(file_path, AstCls);
	}
}