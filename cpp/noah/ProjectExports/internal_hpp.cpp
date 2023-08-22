#include "../Project.hpp"
#include <iostream>
#include <string>
#include "../head/loop_push_param_format_control.hpp"
#include "internal.hpp"


void noah::Projcet::make_internal_hpp(const std::string& file_path) {
	for (auto& ExpCls : ExpClasses_) {
		write_internal_hpp_Exp_(file_path, ExpCls);
	}
	for (auto& AstCls : AstClasses_) {
		write_internal_hpp_Ast_(file_path, AstCls);
	}
}

void noah::Projcet::write_internal_hpp_Exp_(const std::string& file_path, const ABICLASS& ExpCls) {
	std::string this_internal_cls = ExpCls.get_class_name() + "_internal";
	FILE* fp = fopen((file_path + this_internal_cls + ".hpp").c_str(), "wb");

	TXTLines txtlines;
	txtlines.push("#pragma once");
	txtlines.push();
	txtlines.push("#include <memory>");
	txtlines.push("#include <string>");
	txtlines.push("#include <vector>");
	txtlines.push("#include <map>");
	txtlines.push("#include <cstddef>");
	txtlines.push("#include <cstdint>");
	txtlines.push("#include <abi/param_span.hpp>");
	txtlines.push();

	for (auto& AsitCls : this->AstClasses_) {
		txtlines.push("#include \"", AsitCls.get_class_name(), ".hpp\"");
	}
	txtlines.push();
	txtlines.push("namespace glasssix::", this->module_name);
	txtlines.push("{");
	txtlines.push<1>("class ", this_internal_cls);
	txtlines.push<1>("{");
	txtlines.push<1>("public:");
	txtlines.push<2>("class impl;");
	txtlines.push();
	txtlines.push<2>(this_internal_cls, "(const ", this_internal_cls, " &) = delete;"); //delete copy construct
	txtlines.push();
	txtlines.push<2>(this_internal_cls, " &operator=(const ", this_internal_cls, " &) = delete;"); //delete copy construct
	txtlines.push();

	// *_internal constructor function form init function(ABI define)
	// init function special processing as ExpCLs`s internal.hpp class *_internal constructed function param list
	// e.g. 
	// "VOID init(STR path, INT device, MAP<STR,FLOAT> param_map_abi)" TO
	// "material_code_internal(std::string_view path, int device, std::map<std::string, float>& param_map)"
	txtlines.push<2>(this_internal_cls, "(");
	for (const auto& fun : ExpCls.functions) {
		if (fun.special_function_check() == noah::Function::functionType::init) {
			process_Internal_hpp_Class_Func_Param_list(txtlines, fun);
			break;
		}
	}	
	txtlines.end_add(");");
	txtlines.push();

	txtlines.push<2>("virtual ~", this_internal_cls, "();"); // destructor
	txtlines.push();

	for (const auto& general_function : ExpCls.functions) {
		if (general_function.special_function_check() == Function::functionType::other) {
			// internal class 成员函数自身的类型(返回值类型)要求普通STL容器即可，
		    // 且本身为返回值也不用去考虑const、&等修饰
			std::string ftype= general_function.dump_nestype([](const noah::AType& inType) {
				std::map<noah::BasicAType, std::string> internal_hpp_func_inparam_mapping_table{
					{noah::BasicAType::AT_INT32,"std::int32_t"},
					{noah::BasicAType::AT_UINT32,"std::uint32_t"},
					{noah::BasicAType::AT_UINT8,"std::uint8_t"},
					{noah::BasicAType::AT_FLOAT,"float"},
					{noah::BasicAType::AT_STR,"exposing::param_string"},
					{noah::BasicAType::AT_MAP,"exposing::map"},
					{noah::BasicAType::AT_LIST,"exposing::param_vector"}, // f out type
					{noah::BasicAType::AT_SPAN,"exposing::param_span"},
				};

				std::string outTypeBasicName =
					internal_hpp_func_inparam_mapping_table.count(inType.basic_t) ?
					internal_hpp_func_inparam_mapping_table.at(inType.basic_t) : inType.basic_name;
				return outTypeBasicName;
				});

			// 装载一版成员函数(除init隐式代表的构造函数、标准写法的version函数外的)
			txtlines.push<2>(ftype, " ", general_function.name, "(");
			// 函数输入参数列表
			process_Internal_hpp_Class_Func_Param_list(txtlines, general_function);
			txtlines.end_add(");");
			txtlines.push();
		}
	}

	// *_internal version function
	txtlines.push<2>("std::string version();");
	txtlines.push();
	txtlines.push<1>("private:");
	txtlines.push<2>("std::unique_ptr<impl> impl_;");
	txtlines.push<1>("};");	
	txtlines.push("}");
	fprintf(fp, "%s", txtlines.export_string().c_str());
	fclose(fp);
}

void noah::Projcet::write_internal_hpp_Ast_(const std::string& file_path, const ABICLASS& AstCls) {
	std::string this_internal_cls = AstCls.get_class_name() + "_internal";
	FILE* fp = fopen((file_path + this_internal_cls + ".hpp").c_str(), "wb");

	TXTLines txtlines;
	txtlines.push("#pragma once");
	txtlines.push();
	txtlines.push("#include <memory>");
	txtlines.push("#include <string>");
	txtlines.push("#include <vector>");
	txtlines.push("#include <map>");
	txtlines.push("#include <cstddef>");
	txtlines.push("#include <cstdint>");
	txtlines.push("#include <abi/param_span.hpp>");
	txtlines.push();
	txtlines.push("namespace glasssix::", this->module_name);
	txtlines.push("{");


	txtlines.push<1>("struct ", this_internal_cls);
	txtlines.push<1>("{");
	for (auto& number : AstCls.functions) {
		txtlines.push<2>(number.dump_nestype(), " ", number.name, ";");
	}
	txtlines.push<1>("};");
	txtlines.push();

	txtlines.push("}");

	fprintf(fp, "%s", txtlines.export_string().c_str());
	fclose(fp);

}
