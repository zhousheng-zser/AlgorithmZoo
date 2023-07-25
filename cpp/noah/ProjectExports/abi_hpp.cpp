#include "../Project.hpp"
#include <iostream>
#include <string>
#include "../string_process.hpp"

void noah::Projcet::make_abi_hpp(const std::string& file_path) {
	for (auto& ExpCls : ExpClasses_) {
		write_abi_hpp_<ClassType::ExpClass>(file_path, ExpCls);
	}
	for (auto& AstCls : AstClasses_) {
		write_abi_hpp_<ClassType::AstClass>(file_path, AstCls);
	}
}

template<noah::ClassType ClsTyp_>
void noah::Projcet::write_abi_hpp_(const std::string& file_path, const ABICLASS& Cls) {
	std::string HPP_ONCE_MACRO = "_" + noah::UPPER(this->module_name) + "_" + noah::UPPER(Cls.get_class_name()) + "_HPP_";
	std::string INTERG_CLS_NAME = this->module_name+"::"+ Cls.get_class_name(); // heimdall::material_code

	FILE* fp = fopen((file_path + Cls.get_class_name() + ".hpp").c_str(), "wb");


	TXTLines txtlines;
	txtlines.push("#pragma once");
	txtlines.push("#ifndef " + HPP_ONCE_MACRO);
	txtlines.push("#define " + HPP_ONCE_MACRO);
	txtlines.push("");
	
	if (ClsTyp_ == ClassType::ExpClass) {
		for (auto& Asts : AstClasses_) {
			txtlines.push("#include \"" + Asts.get_class_name() + ".hpp\""); // inclu = "file", 强制指定后缀			
		}
	}
	txtlines.push("#include <abi/consumer.hpp>");

	txtlines.push("");
	txtlines.push("namespace glasssix::" + this->module_name);
	txtlines.push("{");
	txtlines.push<1>("struct " + Cls.get_class_name() + ";");
	txtlines.push("}");
	txtlines.push("");
	txtlines.push("namespace glasssix::exposing::impl"); // START "namespace glasssix::exposing::impl"
	txtlines.push("{");
	// abi class define
	txtlines.push<1>("template<>");
	txtlines.push<1>("struct abi<" + INTERG_CLS_NAME + ">");
	txtlines.push<1>("{");
	txtlines.push<2>("using identity_type = type_identity_interface;");
	txtlines.push<2>("static constexpr guid id{ \"{" + GuidString() + "}\" };");
	txtlines.push("");
	txtlines.push<2>("struct type : abi_unknown_object");
	txtlines.push<2>("{");
	for (auto& cls_func : Cls.functions) {
		txtlines.push<3>(cls_func.abi_define());
		txtlines.push("");
	}
	txtlines.push<2>("};");
	txtlines.push<1>("};");
	txtlines.push("");
	// abi vtable
	txtlines.push<1>("template <typename Derived>");
	txtlines.push<1>("struct interface_vtable<Derived, " + INTERG_CLS_NAME + "> : interface_vtable_base<Derived, " + INTERG_CLS_NAME + ">");
	txtlines.push<1>("{");

	for (auto& cls_func : Cls.functions) {
		txtlines.push<2>(cls_func.abi_vtable());
		txtlines.push("");
	}

	txtlines.push<1>("};");
	txtlines.push("");
	// abi adapter
	txtlines.push<1>("template <>");
	txtlines.push<1>("struct abi_adapter<" + INTERG_CLS_NAME + ">");
	txtlines.push<1>("{");
	txtlines.push<2>("template <typename Derived>");
	txtlines.push<2>("struct type : enable_self_abi_awareness<Derived, " + INTERG_CLS_NAME + ">");
	txtlines.push<2>("{");

	for (auto& cls_func : Cls.functions) {
		txtlines.push<3>(cls_func.abi_adapter());
		txtlines.push("");
	}

	txtlines.push<2>("};");
	txtlines.push<1>("};");
	txtlines.push("}"); // END "namespace glasssix::exposing::impl"

	txtlines.push("");
	txtlines.push("namespace glasssix::" + this->module_name);
	txtlines.push("{");
	txtlines.push<1>("struct " + Cls.get_class_name() + " : exposing::inherits<" + Cls.get_class_name() + ">");
	txtlines.push<1>("{");
	txtlines.push<2>("using inherits::inherits;");

	txtlines.push<1>("};");
	txtlines.push("}");
	txtlines.push("");
	txtlines.push("#endif");

	fprintf(fp, "%s", txtlines.export_string().c_str());
	fclose(fp);
}