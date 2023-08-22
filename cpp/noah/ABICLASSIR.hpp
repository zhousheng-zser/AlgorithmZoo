#pragma once
#ifndef __ABI_CLASSIR_HPP__
#define __ABI_CLASSIR_HPP__
#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include "head/header.hpp"
#include "txtlines.hpp"
#include <functional>

namespace noah {

	/// e.g. MAP<INT,FLOAT>  -> {"MAP", 2};
	/// abi type: abstruct like~ MAP, no including template parm to interged type
	/// abi type include base type(eg int float) & pure abi type(eg STR,MAP,LIST,SPAN)
	/// AType: 只记录元类名，模板参数个数，不记录模板参数实际数据类型信息(这会产生复杂的嵌套描述，由CData的nestype成员负责)
	struct AType
	{
		std::string basic_name;
		BasicAType basic_t;   // simple format fact abi type name; eg MAP<INT,FLOAT> is MAP, MAP<STR,SPAN<INT>> is MAP, LIST<INT> is LIST
		std::uint32_t t_num;  // template params num, eg LIST<STR> tnum is 1, MAP<INT,FLOAT> is 2

		AType(const AType& atype) {
			basic_t = atype.basic_t;
			t_num = atype.t_num;
			basic_name = AT_MAP_STRING.count(basic_t) ? AT_MAP_STRING[basic_t] : atype.basic_name;
		};

		AType(std::string tname, std::uint32_t num) : t_num(num) {
			basic_t = ATypeTrans(tname);
			basic_name = AT_MAP_STRING.count(basic_t) ? AT_MAP_STRING[basic_t] : tname;
		}

		bool is_void_type() {
			return basic_t == BasicAType::AT_VOID;
		}

		bool is_pure_abi_type() const {
			return PURE_ABI_TYPE.count(basic_t);
		}

	};

	struct CData // cpp data define:e.g. param_vector<param_string> infos
	{
	private:
		using NestedType = std::vector<AType>;
		// const reference ABI type , needed in ABI.hpp`s adapter, _impl.c/hpp`s Function in params
		// e.g.
		// const exposing::param_hash_map<exposing::param_string, float>& param_map_abi
		// const exposing::param_string& model_directory

		//noah::const_ref cref_status_ = noah::const_ref::no_cref; // handwork process by consumer
	public:
		//pack abstruct uint type 2 nested type : abstruct like~ MAP<STR,SPAN<INT>>
		NestedType nestype;
		std::string name;

		CData(std::string d_type= "VOID", std::string d_name = "cdata");
		CData(const CData& copyCData);
		CData& operator=(const CData& copyCData);

		bool is_void_type() const;

		bool is_pure_abi_type() const;

		
		/// 在ABI定义中，参数类型不出现常量引用类型(别名参数)
		/// 且方法的输入和输出有，尤其纯ABI类型
		/// (int,float等基本类型天然是ABI类型,故讨论string,vector等为纯ABI类型)
		/// 需要abi_in_t<>,abi_out_t<>包装
		/// 不会出现const &.别名修饰
		std::string get_pack_abi_in_t() const;

		std::string get_pack_abi_out_t() const;

		// 建议包装为别名参数
		std::string get_pack_suggest_constref() const;

		std::string get_stand_param() const;

		// TODO : std::vector<AType> trans to std::string 
		// call back function typeNameControl to Control mapping relu, like
		//	"std::map<std::string, float>" -> "param_hash_map<param_string, float>"
		//  "param_string" -> "std::string" or "std::string_view"
		// FormatControlCallBack: std::function<std::string(?&)>
		std::string dump_nestype(std::function<std::string(const noah::AType&)> typeBasicNameControl) const;

		std::string dump_nestype() const;

	protected:
		// string 2 std::vector<AType>
		// std::vector<AType> parseType(std::string d_type);
		std::vector<AType> read2nestype(std::string d_type);

	};


	/// init type,name,params
	/// NestedType func_name(NestedType p1_name, NestedType p2_name ...) { }  also means
	/// CData      func_name(CData      p1_name, CData      p2_name ...) { }
	struct Function : CData {
	public:
		std::vector<CData> params;

		enum class functionType { other, init, version };

	private:
		std::map<std::string, functionType> speacial_function_{ { "init", Function::functionType::init }, { "version", Function::functionType::version } };

	public:
		Function();
		//Function(const Function& other);
		Function(const std::string Temline); //Temline e.g. "LIST<box_info> detect(SPAN<UINT8> bitmap, INT channels, INT height, INT width, STR path, MAP<STR,FLOAT> param_map_abi)"
		
		// manual checking speacial function
		functionType special_function_check(std::string fname) const {
			return speacial_function_.count(fname) ? speacial_function_.at(fname) : functionType::other;
		}
		// speacial function maybe has speacial write format
		functionType special_function_check() const {
			return speacial_function_.count(this->name) ? speacial_function_.at(this->name) : functionType::other;
		}

		noah::TXTLines abi_define() const;

		noah::TXTLines abi_vtable() const;

		noah::TXTLines abi_adapter() const;

		std::string impl_function(std::string region = "") const;

	private:
		// TODO: {...,"xxx,"} -> {...,"xxx"}
		void del_end_comma_(std::vector<std::string>& strlines) {
			if (*(*strlines.rbegin()).rbegin() == ',') (*strlines.rbegin()).pop_back();
		}
	};

	enum class ClassType { AstClass, ExpClass };
	class ABICLASS {
		// status : is_ExpAbi == true means use Impl mode (mian export ABI class)
		ClassType cls_type_ = ClassType::AstClass;
		std::string plugin_name_; // e.g. heimdall
		std::string class_name_;  // e.g. material_code

		inline std::string all_class_name() {
			return plugin_name_ + "::" + class_name_; // heimdall::material_code
		}

	public:
		std::vector<Function> functions;

	public:
		ABICLASS();
		ABICLASS(const ABICLASS& cls);
		ABICLASS(const std::string& p_name);
		ABICLASS(std::string p_name, std::string c_name);
		ABICLASS(std::string p_name, std::string c_name, ClassType AbiClsType = ClassType::AstClass);

		void set_plugin_name(std::string p_name) {
			plugin_name_ = p_name;
		}

		/// <summary>
		/// set abi class name and status
		/// </summary>
		/// <param name="c_name">class name</param>
		/// <param name="is_ExpAbi">export abi class(EXP_ABICLASS)</param>
		void set_class_name_and_status(std::string c_name, ClassType AbiClsType = ClassType::AstClass) {
			class_name_ = c_name;
			cls_type_ = AbiClsType;
		}

		std::string get_plugin_name() const {
			return plugin_name_;
		}

		std::string get_class_name() const {
			return class_name_;
		}

		/// true: 主ABI类(导出ABI/Export ABI)
		/// false: 辅ABI类
		/// 设为主ABI,一般主ABI使用IMPL模式，并且拥有承担实际逻辑运行的内部类XX_insternal.cpp
		/// 主ABI类的impl.hpp文件中维护的对象为std::unique_ptr<TYPE_INTERNAL> impl_
		/// 辅助ABI类一般较简单，其impl.hpp文件中维护的对象为 TYPE_INTERNAL internal_			
		void reset_status(ClassType AbiClsType) {
			cls_type_ = AbiClsType;
		}

		// true: is_ExpAbi
		ClassType get_status() const {
			return cls_type_;
		}
	};


} // namespace noah
#endif // __ABI_CLASSIR_HPP__