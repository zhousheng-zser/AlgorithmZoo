#pragma once

#include "../Project.hpp"
#include <iostream>
#include <string>
#include "../head/loop_push_param_format_control.hpp"

// internal.hpp class function param list need speacail type trans and format contral
// internall类成员函数，函数输入参数格式控制：
// STR->std::string_view
// MAP->std::map 且为引用
// INT修饰为普通写法int
// 其余无特殊要求
static inline void process_Internal_hpp_Class_Func_Param_list(noah::TXTLines& txtlines,const noah::Function& function){

	loop_push_param_format_control<0, false>{}(txtlines, function,
		[](const noah::CData& fparam) {
			std::string pname = maybe_MapParamName_ABI_2_STD_(fparam);

			std::string ptype = fparam.dump_nestype([](const noah::AType& inType) {
				std::map<noah::BasicAType, std::string> internal_hpp_func_inparam_mapping_table{
					{noah::BasicAType::AT_INT32,"int"},
					{noah::BasicAType::AT_UINT32,"std::uint32_t"},
					{noah::BasicAType::AT_UINT8,"std::uint8_t"},
					{noah::BasicAType::AT_FLOAT,"float"},
					{noah::BasicAType::AT_STR,"std::string"},
					{noah::BasicAType::AT_MAP,"std::map"},
					{noah::BasicAType::AT_LIST,"std::vector"}, //输入参数列表中出现param_vector，可以在impl.cpp中去转换
					{noah::BasicAType::AT_SPAN,"exposing::param_span"},
				};

				std::string outTypeBasicName =
					internal_hpp_func_inparam_mapping_table.count(inType.basic_t) ?
					internal_hpp_func_inparam_mapping_table.at(inType.basic_t) : inType.basic_name;
				return outTypeBasicName;
				});

			if (fparam.nestype[0].basic_t == noah::BasicAType::AT_MAP) ptype += '&';
			if (fparam.nestype[0].basic_t == noah::BasicAType::AT_LIST) ptype += '&';
			if (fparam.nestype[0].basic_t == noah::BasicAType::AT_STR) ptype += "_view";

			return ptype + ' ' + pname;
		}
	);

}
