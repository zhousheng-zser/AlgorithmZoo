#pragma once
#ifndef HEADER
#define HEADER

#include <iostream>
#include <string>
#include <cctype>
#include <map>
#include <set>

#include <vector>
#include <array>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iterator>


namespace noah {
	static inline void show_usage() {
		std::ostringstream ss;
		ss << "Usage: noah.exe [template.txt]\n"
		   << "    or noah.exe [template.txt] [AlgorithmZooPath]\n"
		   << "( e.g. noah.exe selene.txt D:/glasssix/AlgorithmZoo )\n";
		fprintf(stderr, ss.str().c_str());
	}

	enum class BasicAType
	{
		AT_VOID,
		AT_BOOL,
		AT_INT32,
		AT_UINT32,
		AT_INT8,
		AT_UINT8,
		AT_FLOAT,
		AT_STR,
		AT_MAP,
		AT_LIST,
		AT_SPAN,
		AT_OTHER
	};

	static std::set<BasicAType> PURE_ABI_TYPE{
		BasicAType::AT_STR,
		BasicAType::AT_MAP,
		BasicAType::AT_LIST,
		BasicAType::AT_SPAN,
	};

	// ABI参数中,在ABI.hpp的适配器、impl.cpp/hpp函数中,建议
	static std::set<BasicAType> SUGGEST_CONST_REF{
		BasicAType::AT_STR,
		BasicAType::AT_MAP,
		BasicAType::AT_LIST,
	};
	enum class const_ref { no_cref, is_cref };

	static std::map<std::string, BasicAType> STRING_MAP_AT{
		{"VOID",BasicAType::AT_VOID},
		{"void",BasicAType::AT_VOID},
		{"BOOL",BasicAType::AT_BOOL},
		{"bool",BasicAType::AT_BOOL},

		{"INT",BasicAType::AT_INT32},
		{"int",BasicAType::AT_INT32},
		{"I32",BasicAType::AT_INT32},
		{"i32",BasicAType::AT_INT32},
		{"INT32",BasicAType::AT_INT32},
		{"int32",BasicAType::AT_INT32},

		{"UINT32",BasicAType::AT_UINT32},
		{"uint32",BasicAType::AT_UINT32},
		{"UINT",BasicAType::AT_UINT32},
		{"uint",BasicAType::AT_UINT32},

		{"UINT8",BasicAType::AT_UINT8},
		{"uint8",BasicAType::AT_UINT8},
		{"U8",BasicAType::AT_UINT8},
		{"UI8",BasicAType::AT_UINT8},
		{"u8",BasicAType::AT_UINT8},

		{"FLOAT",BasicAType::AT_FLOAT},
		{"float",BasicAType::AT_FLOAT},
		{"F32",BasicAType::AT_FLOAT},
		{"f32",BasicAType::AT_FLOAT},

		{"STR",BasicAType::AT_STR},
		{"MAP",BasicAType::AT_MAP},
		{"LIST",BasicAType::AT_LIST},
		{"VEC",BasicAType::AT_LIST},
		{"VECTOR",BasicAType::AT_LIST},
		{"SPAN",BasicAType::AT_SPAN},
	};

	static std::map<BasicAType, std::string> AT_MAP_STRING{
		// map self
		{BasicAType::AT_VOID,"void"},
		{BasicAType::AT_BOOL,"bool"},
		{BasicAType::AT_INT32,"std::int32_t"},
		{BasicAType::AT_UINT32,"std::uint32_t"},
		{BasicAType::AT_UINT8,"std::uint8_t"},
		{BasicAType::AT_FLOAT,"float"},
		{BasicAType::AT_STR,"exposing::param_string"},
		{BasicAType::AT_MAP,"exposing::param_hash_map"},
		{BasicAType::AT_LIST,"exposing::param_vector"},
		{BasicAType::AT_SPAN,"exposing::param_span"},
	};

	static inline BasicAType ATypeTrans(std::string typestr) {
		return STRING_MAP_AT.count(typestr) ? STRING_MAP_AT[typestr] : BasicAType::AT_OTHER;
	}

	static inline std::string ATypeTrans(BasicAType basic_t) {
		return AT_MAP_STRING.count(basic_t) ? AT_MAP_STRING[basic_t] : "UNKNOWN_TYPE";
	}

};

#endif // HEADER