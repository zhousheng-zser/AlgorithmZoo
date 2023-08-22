#pragma once
#include "../ABICLASSIR.hpp"

/// <summary>
/// TODO: dump noah::Function like Ty f(Ty1 p1,Ty2 p2,..Tyn pn) to string or str list
/// -> "format(Ty1 p1),format(Ty2 p2), .., format(Tyn pn)"
/// -> or ["format(Ty1 p1)", .., "format(Tyn pn)"] with indentation
/// </summary>
template<int BaseIndent_, bool If_Newline_ = true>
struct loop_push_param_format_control {	
	/* 
	fparam_fmt_callbk e.g.:
	std::string vtable_fbody_fmt_(noah::CData& param) {
		return param.is_pure_abi_type() ? "create_from_abi<" + param.dump_nestype() + ">(" + param.name + ")," : param.name;
    } 
	*/

	using fparam_fmt_callbk = std::function<std::string(const noah::CData&)>;
	using func_t_fmt_callbk = std::function<std::string(const noah::Function&)>;
	using cref_fp_fmtcb = const fparam_fmt_callbk&;
	using cref_ft_fmtcb = const func_t_fmt_callbk&;
	using noahTL = noah::TXTLines;
	using noahFn = noah::Function;

	// 模板参数<缩进等级,是否换行> 在是否换行为false(不换行)下，缩进等级参数不启用
	// 复用参数写入方式，以回调函数来控制函数输入参数的装饰写法，自动添","及末尾清除
	void operator()(noahTL& txtlines, const noahFn& function, cref_fp_fmtcb p_callbk) {
		std::vector<std::string> parmalines;
		fparam_load_(parmalines, function, p_callbk);
		push_rst_(parmalines, txtlines);
	}

	// 特殊复用参数写入方式 : function_name(abi_in, .., abi_in, ..， abi_out)，
	// 适用ABI定义中，abi方法将输入参数与方法返回值参数混写入定义参数列表的写法
	void operator()(noahTL& txtlines, const noahFn& function, cref_fp_fmtcb p_callbk, cref_ft_fmtcb t_callbk) {
		std::vector<std::string> parmalines;
		fparam_load_(parmalines, function, p_callbk);
		ftype_load_(parmalines, function, t_callbk);
		push_rst_(parmalines, txtlines);
	}

	loop_push_param_format_control() {}
private:

	void fparam_load_(std::vector<std::string>& parmalines, const noahFn& function, cref_fp_fmtcb p_callbk) {
		for (auto& inparm : function.params) {
			std::string fparam_format = p_callbk(inparm);
			if (!fparam_format.empty())
				parmalines.push_back(fparam_format);
		}
	}

	// used in ABI define.hpp
	void ftype_load_(std::vector<std::string>& parmalines, const noahFn& function, cref_ft_fmtcb t_callbk) {
		if (!function.is_void_type()) {
			std::string ft_format = t_callbk(function);
			if (!ft_format.empty())
				parmalines.push_back(ft_format);
		}
	}

	void push_rst_(std::vector<std::string>& parmalines, noahTL& txtlines) {
		if (parmalines.empty()) return;
		if (parmalines.size() == 1) {
			txtlines.end_add(parmalines[0]);
		}
		else {
			for (auto& line : parmalines) {
				if(If_Newline_)
					txtlines.push<BaseIndent_>(line + ",");
				else
					txtlines.end_add(line + ", ");
			}
			txtlines.del_end_comma();
		}
	}
};

