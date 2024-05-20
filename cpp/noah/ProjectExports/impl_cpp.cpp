#include "../Project.hpp"
#include <iostream>
#include <string>
#include "../head/loop_push_param_format_control.hpp"
#include "../ABICLASSIR.hpp"
#include "../string_process.hpp"

void noah::Projcet::make_impl_cpp(const std::string& file_path) {
	for (auto& ExpCls : ExpClasses_) {
		write_impl_cpp_Exp_(file_path, ExpCls);
	}
	for (auto& AstCls : AstClasses_) {
		write_impl_cpp_Ast_(file_path, AstCls);
	}
}

void noah::Projcet::write_impl_cpp_Exp_(const std::string& file_path, const ABICLASS& Cls) {
	FILE* fp = fopen((file_path + Cls.get_class_name() + "_impl.cpp").c_str(), "wb");
	TXTLines txtlines;

	std::string implClass = Cls.get_class_name() + "_impl";
	std::string CLASS_REGION = implClass + "::";

	txtlines.push_include(Cls.get_class_name() + "_impl.hpp");
	txtlines.push_include(Cls.get_class_name() + "_internal.hpp");

	txtlines.push("#include <map>");

	txtlines.push("");
	txtlines.push("namespace glasssix::" + this->module_name);
	txtlines.push("{");
	txtlines.push<1>(CLASS_REGION + implClass + "() {}"); // constructor
	txtlines.push("");

	txtlines.push<1>(CLASS_REGION + '~' + implClass + "() {}"); // destructor
	txtlines.push("");

	std::map<noah::BasicAType, std::string> impl_cpp_function_body_nestype_map_STL{
		// map self
		{noah::BasicAType::AT_MAP,"std::map"},
		{noah::BasicAType::AT_STR,"std::string"},
		{noah::BasicAType::AT_PAIR,"std::pair"},
		{noah::BasicAType::AT_INT32,"int"},
	};

	for (auto& abi_func : Cls.functions) {
		if (abi_func.special_function_check() == Function::functionType::version) continue;
		std::string func_str = abi_func.impl_function(CLASS_REGION);
		txtlines.push<1>(func_str);
		txtlines.push<1>("{");

		// exposing::param_hash_map Box std::map
		auto param_copy = abi_func.params; // function in params map type check
		for (auto& inparm : param_copy) {
			if (inparm.nestype[0].basic_t == BasicAType::AT_MAP) {
				auto& nst = inparm.nestype;
				for (auto& atype : nst) {
					atype.basic_name =
						impl_cpp_function_body_nestype_map_STL.count(atype.basic_t) ?
						impl_cpp_function_body_nestype_map_STL[atype.basic_t] : atype.basic_name;
				}

				auto map_param_std_name = maybe_MapParamName_ABI_2_STD_(inparm);
				txtlines.push<2>(inparm.dump_nestype(), " ", map_param_std_name, ";");
				txtlines.push<2>("for (auto it : ", inparm.name, ") {");
				txtlines.push<3>(map_param_std_name, ".insert(std::make_pair(it.key(), it.value()));");
				txtlines.push<2>("}");
				txtlines.push();
			}
		}

		if(abi_func.special_function_check() == Function::functionType::init) {
			txtlines.push<2>("impl_ = std::make_unique<" , Cls.get_class_name() , "_internal>(");
			
			loop_push_param_format_control<2, false>{}(txtlines, abi_func,
				[](const noah::CData& fparam) {
					if (fparam.nestype[0].basic_t == BasicAType::AT_STR) {
						return "exposing::to_narrow_string(" + fparam.name + ")";
					}
					else if (fparam.nestype[0].basic_t == BasicAType::AT_MAP) {
						return maybe_MapParamName_ABI_2_STD_(fparam);
					}
					else
						return fparam.name;
				}
			);
			txtlines.end_add(");");
		}
		else {
			txtlines.push<2>("if (!impl_)");
			txtlines.push<3>("throw exposing::abi_invalid_operation(u8\"", this->module_name," ",Cls.get_class_name()," internal object not initialized\"); ");
			txtlines.push();
			txtlines.push<2>("return impl_->", abi_func.name,"(");
			loop_push_param_format_control<2, false>{}(txtlines, abi_func,
				[](const noah::CData& fparam) {
					if (fparam.nestype[0].basic_t == BasicAType::AT_STR) {
						return "exposing::to_narrow_string(" + fparam.name + ")";
					}
					else if (fparam.nestype[0].basic_t == BasicAType::AT_MAP) {
						return maybe_MapParamName_ABI_2_STD_(fparam);
					}
					else
						return fparam.name;
				}
			);
			txtlines.end_add(");");
		}

		txtlines.push<1>("}");
		txtlines.push("");
	}
	// impl class implement version()
	txtlines.push<1>("exposing::param_string ", implClass, "::version() const");
	txtlines.push<1>("{");
	txtlines.push<2>("return exposing::to_param_string(impl_->version());");
	txtlines.push<1>("}");



	txtlines.push("}");

	fprintf(fp, "%s", txtlines.export_string().c_str());
	fclose(fp);
}

void noah::Projcet::write_impl_cpp_Ast_(const std::string& file_path, const ABICLASS& Cls) {
	FILE* fp = fopen((file_path + Cls.get_class_name() + "_impl.cpp").c_str(), "wb");
	TXTLines txtlines;

	std::string implClass = Cls.get_class_name() + "_impl";
	std::string CLASS_REGION = implClass + "::";


	txtlines.push_include(Cls.get_class_name() + "_impl.hpp");
	txtlines.push("");
	txtlines.push("namespace glasssix::" + this->module_name);
	txtlines.push("{");
	txtlines.push<1>(CLASS_REGION + implClass + "() {}"); // constructor
	txtlines.push("");
	txtlines.push<1>(CLASS_REGION + implClass + "(const ", Cls.get_class_name() + "_internal", " &internal) : internal_(internal) {}"); // destructor
	txtlines.push("");
	txtlines.push<1>(CLASS_REGION + '~' + implClass + "() {}"); // destructor
	txtlines.push("");

	std::map<noah::BasicAType, std::string> impl_cpp_function_body_nestype_mapping_STL{
		// map self
		{noah::BasicAType::AT_MAP,"std::map"},
		{noah::BasicAType::AT_STR,"std::string"},
		{noah::BasicAType::AT_PAIR,"std::pair"},
		{noah::BasicAType::AT_INT32,"int"},
	};

	for (auto& abi_func : Cls.functions) {
		std::string func_str = abi_func.impl_function(CLASS_REGION);
		txtlines.push<1>(func_str);
		txtlines.push<1>("{");

		// exposing::param_hash_map Box std::map
		auto param_copy = abi_func.params;
		for (auto& inparm : param_copy) {
			if (inparm.nestype[0].basic_t == BasicAType::AT_MAP) {
				auto& nst = inparm.nestype;
				for (auto& atype : nst) {
					atype.basic_name =
						impl_cpp_function_body_nestype_mapping_STL.count(atype.basic_t) ?
						impl_cpp_function_body_nestype_mapping_STL[atype.basic_t] : atype.basic_name;
				}

				txtlines.push<2>(inparm.dump_nestype(), " ", "param_map;");
				txtlines.push<2>("for (auto it : ", inparm.name, ") {");
				txtlines.push<3>("param_map.insert(std::make_pair(it.key(), it.value()));");
				txtlines.push<2>("}");
				txtlines.push();
			}
		}
		txtlines.push<2>("return internal_." + abi_func.name, ";");
		txtlines.push<1>("}");
		txtlines.push("");
	}
	txtlines.push("}");
	fprintf(fp, "%s", txtlines.export_string().c_str());
	fclose(fp);
}
