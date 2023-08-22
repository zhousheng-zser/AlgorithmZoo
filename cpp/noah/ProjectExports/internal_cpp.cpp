#include "../Project.hpp"
#include <iostream>
#include <string>
#include "internal.hpp"

void noah::Projcet::make_internal_cpp(const std::string& file_path) {
	for (auto& ExpCls : ExpClasses_) {
		write_internal_cpp_Exp_(file_path, ExpCls);
	}
}

void noah::Projcet::write_internal_cpp_Exp_(const std::string& file_path, const ABICLASS& ExpCls) {
	std::string this_internal_cls = ExpCls.get_class_name() + "_internal";
	std::string interClsRegion = this_internal_cls + "::";


	FILE* fp = fopen((file_path + this_internal_cls + ".cpp").c_str(), "wb");
	TXTLines txtlines;

	txtlines.push("#include \"", ExpCls.get_class_name(), "_internal.hpp\"");
	for (auto& AstCls : AstClasses_) {
		txtlines.push("#include \"", AstCls.get_class_name(), "_internal.hpp\"");
	}

	txtlines.push();
	txtlines.push("#include <algorithm>");
	txtlines.push("#include <numeric>");

	txtlines.push();
	txtlines.push("#include <Excalibur/pipeline.hpp>");
	txtlines.push("#include <Primitives/pool_allocator.hpp>");
	txtlines.push("#include <Primitives/tensor_conversions.hpp>");
	txtlines.push("#include <Excalibur/operation_safty_cut.hpp>");
	txtlines.push("#include <Excalibur/operation_safty_cut.hpp>");
	txtlines.push("#include \"Primitives/tensor_conversions.hpp\"");
	txtlines.push("#include \"Excalibur/operation_make_border.hpp\"");
	txtlines.push("#include \"Excalibur/operation_resize.hpp\"");
	txtlines.push("#include \"Excalibur/operation_rgb2gray.hpp\"");
	txtlines.push();

	txtlines.push("#include <opencv2/core/core.hpp>");
	txtlines.push("#include <opencv2/imgproc/imgproc.hpp>");
	txtlines.push();
	txtlines.push("#ifdef USE_CUDA");
	txtlines.push("#include <cuda_runtime_api.h>");
	txtlines.push("#endif");
	txtlines.push();
	txtlines.push("namespace glasssix::", this->module_name);
	txtlines.push("{");

	auto initFnIter = ExpCls.functions.cbegin();
	for (; initFnIter != ExpCls.functions.cend(); initFnIter++) {
		if (initFnIter->special_function_check() == noah::Function::functionType::init) {
			break;
		}
	}

	// ============== internal impl implement
	txtlines.push<1>("class ", this_internal_cls, "::impl");
	txtlines.push<1>("{");
	txtlines.push<1>("public:");
	txtlines.push<2>("impl(");
	process_Internal_hpp_Class_Func_Param_list(txtlines, *initFnIter);
	txtlines.end_add(")");
	txtlines.push<2>("{");
	txtlines.push<3>("/* code */");
	txtlines.push<2>("}");
	txtlines.push();

	// impl general function

	for (const auto& general_function : ExpCls.functions) {
		if (general_function.special_function_check() == Function::functionType::other) {
			// internal class 成员函数自身的类型(返回值类型)要求普通STL容器即可，
			// 且本身为返回值也不用去考虑const、&等修饰
			std::string ftype = general_function.dump_nestype([](const noah::AType& inType) {
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

			txtlines.push<2>(ftype, " ", general_function.name, "(");
			// 函数输入参数列表
			process_Internal_hpp_Class_Func_Param_list(txtlines, general_function);
			txtlines.end_add(")");
			txtlines.push<2>("{");
			txtlines.push<3>("std::cout << std::endl << \"hello ", this->get_name_() + "::" + ExpCls.get_class_name() + "::" + general_function.name, " !\" << std::endl;");

			if (!general_function.is_pure_abi_type()) {
				txtlines.push<3>(general_function.dump_nestype(), " result;");
			}
			else {
				noah::CData FnCopy = general_function;
				std::map<BasicAType, std::string> make_phai_interface{
					{BasicAType::AT_MAP,"exposing::make_param_hash_map"},
					{BasicAType::AT_STR,"exposing::make_param_string"},
					{BasicAType::AT_LIST,"exposing::make_param_vector"}
				};
				if (make_phai_interface.count(FnCopy.nestype[0].basic_t)) {
					FnCopy.nestype[0].basic_name = make_phai_interface.at(FnCopy.nestype[0].basic_t);
					FnCopy.nestype[0].basic_t = BasicAType::AT_OTHER;
				}

				txtlines.push<3>("auto result = ", FnCopy.dump_nestype(), "();");
			}
			txtlines.push<3>("return result;");

			txtlines.push<2>("}");
			txtlines.push();
		}
	}

	// impl version function
	txtlines.push<2>("std::string version()");
	txtlines.push<2>("{");
	txtlines.push<3>("const std::string algo_module_version = \"1.0.0\";");
	txtlines.push<0>("#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)");
	txtlines.push<3>("//#if 0");
	txtlines.push<3>("std::string nn_frame_version = instance_->version();");
	txtlines.push<0>("#else");
	txtlines.push<3>("std::string nn_frame_version = instance_->version();");
	txtlines.push<0>("#endif");
	txtlines.push<3>("return fmt::format(R\"({ {\"nn_frame_version\":\"{}\", \"algo_module_version\" : \"{}\"} })\", nn_frame_version, algo_module_version);");
	txtlines.push<2>("}");
	txtlines.push();


	txtlines.push<1>("private:");
	txtlines.push<0>("#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)");
	txtlines.push<2>("std::unique_ptr<rknnwrapper::rknn_wrapper> instance_;");
	txtlines.push<0>("#else");
	txtlines.push<2>("std::unique_ptr<excalibur::pipeline<float>> instance_;");
	txtlines.push<0>("#endif");

	txtlines.push<1>("};");
	txtlines.push();


	// ============== internal class implement
	txtlines.push<1>(interClsRegion + this_internal_cls, "(");


	process_Internal_hpp_Class_Func_Param_list(txtlines, *initFnIter);

	txtlines.end_add(")");
	txtlines.push<2>(": impl_{ std::make_unique<impl>(");
	loop_push_param_format_control<0, false>{}(txtlines, *initFnIter,
		[](const noah::CData& fparam) { return maybe_MapParamName_ABI_2_STD_(fparam); });
	txtlines.end_add(") }");
	txtlines.push<1>("{");
	txtlines.push<1>("}");
	txtlines.push();

	txtlines.push<1>(interClsRegion + "~" + this_internal_cls, "()"); // destructor
	txtlines.push<1>("{");
	txtlines.push<1>("}");
	txtlines.push();

	for (const auto& general_function : ExpCls.functions) {
		if (general_function.special_function_check() == Function::functionType::other) {
			// internal class 成员函数自身的类型(返回值类型)要求普通STL容器即可，
			// 且本身为返回值也不用去考虑const、&等修饰
			std::string ftype = general_function.dump_nestype([](const noah::AType& inType) {
				std::map<noah::BasicAType, std::string> internal_hpp_func_inparam_mapping_table{
					{noah::BasicAType::AT_INT32,"int"},
					{noah::BasicAType::AT_UINT32,"std::uint32_t"},
					{noah::BasicAType::AT_UINT8,"std::uint8_t"},
					{noah::BasicAType::AT_FLOAT,"float"},
					{noah::BasicAType::AT_STR,"exposing::param_string"},
					{noah::BasicAType::AT_MAP,"std::map"},
					{noah::BasicAType::AT_LIST,"exposing::param_vector"},
					{noah::BasicAType::AT_SPAN,"exposing::param_span"},
				};

				std::string outTypeBasicName =
					internal_hpp_func_inparam_mapping_table.count(inType.basic_t) ?
					internal_hpp_func_inparam_mapping_table.at(inType.basic_t) : inType.basic_name;
				return outTypeBasicName;
				});

			// 装载一版成员函数(除init隐式代表的构造函数、标准写法的version函数外的)
			txtlines.push<1>(ftype, " ", interClsRegion, general_function.name, "(");
			// 函数输入参数列表
			process_Internal_hpp_Class_Func_Param_list(txtlines, general_function);
			txtlines.end_add(")");
			txtlines.push<1>("{");
			txtlines.push<2>("return impl_->", general_function.name, "(");
			loop_push_param_format_control<0, false>{}(txtlines, general_function,
				[](const noah::CData& fparam) {
					auto s = maybe_MapParamName_ABI_2_STD_(fparam);
					return s;
				});

			txtlines.end_add(");");
			txtlines.push<1>("}");
			txtlines.push();
		}
	}


	// *_internal version function
	txtlines.push<1>("std::string ", interClsRegion, "version()");
	txtlines.push<1>("{");
	txtlines.push<2>("return impl_->version();");
	txtlines.push<1>("}");


	txtlines.push("}");

	fprintf(fp, "%s", txtlines.export_string().c_str());
	fclose(fp);
}


