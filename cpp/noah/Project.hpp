#pragma once
#ifndef __PROJECT_HPP__
#define __PROJECT_HPP__
#include "ABICLASSIR.hpp"
#include <iostream>
#include <string>
#include "string_process.hpp"

namespace noah {
class Projcet {

	std::string module_name; //plugin_name e.g. ring heimdall

	std::vector<ABICLASS> ExpClasses_; // Main Classes for export

	std::vector<ABICLASS> AstClasses_; // assistant Classes e.g. class BoxInfo

	void CMakeLists_txt(const std::string& file_path);

	void exports_cpp(const std::string& file_path);

	void make_abi_hpp(const std::string& file_path);
	template<ClassType ClsTyp_>
	void write_abi_hpp_(const std::string& file_path, const ABICLASS& Cls);


	void make_impl_hpp(const std::string& file_path);
	template<ClassType ClsTyp_>
	void write_impl_hpp_(const std::string& file_path, const ABICLASS& Cls);


	void make_impl_cpp(const std::string& file_path);
	void write_impl_cpp_Exp_(const std::string& file_path, const ABICLASS& Cls);
	void write_impl_cpp_Ast_(const std::string& file_path, const ABICLASS& Cls);

	void make_internal_hpp(const std::string& file_path);
	void write_internal_hpp_Exp_(const std::string& file_path, const ABICLASS& ExpCls);
	void write_internal_hpp_Ast_(const std::string& file_path, const ABICLASS& AstCls);

	void make_internal_cpp(const std::string& file_path);
	void write_internal_cpp_Exp_(const std::string& file_path, const ABICLASS& ExpCls);

protected:

	Projcet();

	~Projcet();

	// set name_
	void init_(std::string name);

	std::string get_name_() const {
		return module_name;
	}

	void push_cls_(const ABICLASS& cls);

	void write_code_(std::string file_path);
	void write_cmake_(std::string file_path);
};

//////////// tool function

// TODO: form "param_hash_map<exposing::param_string, float>& param_map_abi" to gen std variable
// ".. param_map_abi" -> "std::map<..> param_map_std"
std::string maybe_MapParamName_ABI_2_STD_(const noah::CData& maybe_map_param);


}

#endif //!__PROJECT_HPP__