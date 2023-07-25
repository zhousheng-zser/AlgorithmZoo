#include "../ABICLASSIR.hpp"
#include "../string_process.hpp"
#include "../head/loop_push_param_format_control.hpp"

struct TemlinePars {
	std::string name;
	std::string raw_type; // "const int" is raw, not CData::NestedType

	TemlinePars() {};

	TemlinePars(std::vector<std::string>& Temline) {
		TemlinePars_(Temline);
	}

	// "MAP<STR,FLOAT> param_map_abi"
	TemlinePars(std::string TemlineCol) {

		auto s_head = TemlineCol.cbegin();
		auto s_end = TemlineCol.crbegin();

		// filter head punctuation ',' and blank: 
		// ", INT width" -> "INT width"
		// ", INT width, " -> "INT width"
		while (s_head != TemlineCol.cend() && (*s_head == ',' || *s_head == ' ')) {
			s_head++;
		}
		while (s_end.base() != s_head && (*s_end == ',' || *s_end == ' ')) {
			s_end++;
		}

		std::string TemlineColPure(s_head, s_end.base());

		std::vector<std::string> Temline = noah::splite_words_by_char(TemlineColPure, ' ');
		TemlinePars_(Temline);
	}
private:
	void TemlinePars_(std::vector<std::string>& Temline) {
		int out_cdata_size = Temline.size();
		try {
			if (Temline.empty())
				throw Temline.size();
		}
		catch (...) {
			std::cout << "(Function.cpp) TemlinePars Temline parse error" << std::endl;
		}

		if (out_cdata_size > 0) {
			if (out_cdata_size == 1) {
				raw_type = "VOID";
				name = Temline[0];
			}
			else {
				for (int i = 0; i < out_cdata_size - 1; i++) {
					raw_type += Temline[i];
				}
				name = *Temline.rbegin();
			}
		}
	}
};

noah::Function::Function() :CData("void", "") {}

noah::Function::Function(const std::string Temline){

	// Temline e.g.
	// "LIST<box_info> detect(SPAN<UINT8> bitmap, INT channels, INT height, INT width)"
	auto p_start = Temline.find_first_of('(');
	auto p_end = Temline.find_first_of(')');
	std::string funInfoRaw = Temline.substr(0, p_start);
	std::vector<std::string> funInfo = splite_words_by_char(funInfoRaw, ' ');
	TemlinePars FunInfoPars(funInfo);

	// construction
	this->nestype = read2nestype(FunInfoPars.raw_type);
	this->name = FunInfoPars.name;

	std::string inParamListString = Temline.substr(p_start + 1, p_end - p_start - 1);

	if (!inParamListString.empty()) {
		auto s_head = inParamListString.cbegin();
		auto s_tail = inParamListString.cbegin();
		for (int region = 0;; s_tail++) {

			if (region == 0 && s_tail == inParamListString.cend()) {
				std::string substr(s_head, s_tail);
				TemlinePars inparam(substr);
				this->params.push_back({ inparam.raw_type, inparam.name });
				break;
			}

			// split tag ','
			// region == 0 for not splitting"<X,Y>"
			if (region == 0 && *s_tail == ',') {
				std::string substr(s_head, s_tail);
				TemlinePars inparam(substr);
				this->params.push_back({ inparam.raw_type, inparam.name });
				s_head = s_tail; // up cur
			}
			if (*s_tail == '<')
				region++;
			else if (*s_tail == '>')
				region--;
		}
	}
}

//noah::Function::Function(const Function& other) {
//	this->nestype = other.nestype;
//	this->name = other.name;
//	this->params = other.params;
//}

noah::TXTLines noah::Function::abi_define() const {
	TXTLines txtlines;

	txtlines.push("virtual std::int32_t G6_ABI_CALL ",this->name,"(");

	loop_push_param_format_control<1>{}(txtlines, *this,
		[](const noah::CData& param) { return param.get_pack_abi_in_t(); },
		[](const noah::Function& f) { return f.get_pack_abi_out_t(); }
	);

	txtlines.end_add(") noexcept = 0;");
	return txtlines;
}

noah::TXTLines noah::Function::abi_vtable() const {
	TXTLines txtlines;

	txtlines.push("virtual std::int32_t G6_ABI_CALL " + this->name + "(");

	loop_push_param_format_control<1>{}(txtlines, *this, 
		[](const noah::CData& param) { return param.get_pack_abi_in_t(); },
		[](const noah::Function& f) { return f.get_pack_abi_out_t(); }
	);

	txtlines.end_add(") noexcept override");

	txtlines.push("{");
	if (!this->is_void_type()) { //Function类型不为空，即方法存在返回值
		txtlines.push<1>("return abi_safe_call([&]");
		txtlines.push<2>("{");
		txtlines.push<3>("*result = detach_abi(this->self()." + this->name + "("); // detach_abi

		loop_push_param_format_control<4>{}(txtlines, *this, [](const noah::CData& param) {
			return param.is_pure_abi_type() ? "create_from_abi<" + param.dump_nestype() + ">(" + param.name + ")" : param.name;
			});

		txtlines.end_add("));");
		txtlines.push<2>("}");
		txtlines.push<1>(");");
	}
	else { // Function类型为空，方法无返回值
		txtlines.push<1>("return abi_safe_call([&]");
		txtlines.push<2>("{this->self()." + this->name + "(");

		loop_push_param_format_control<3>{}(txtlines, *this, [](const noah::CData& param) {
			return param.is_pure_abi_type() ? "create_from_abi<" + param.dump_nestype() + ">(" + param.name + ")" : param.name;
			});

		txtlines.end_add(");");
		if (params.empty())
			txtlines.end_add("});");
		else {
			txtlines.push<2>("});");
		}
	}
	txtlines.push("}");
	return txtlines;
}

noah::TXTLines noah::Function::abi_adapter() const {
	TXTLines txtlines;

	txtlines.push(dump_nestype() + " " + this->name + "(");

	loop_push_param_format_control<1>{}(txtlines, *this,
		[](const noah::CData& param) { return param.get_pack_suggest_constref(); }
	);

	txtlines.end_add(") const");
	txtlines.push("{");

	if (!this->is_void_type()) { //Function类型不为空，方法存在返回值
		txtlines.push<1>(dump_nestype() + " result");
		if (this->is_pure_abi_type())
			txtlines.end_add("{ nullptr };");
		else
			txtlines.end_add(";");
		txtlines.push<1>("return (check_abi_result(this->self_abi()." + this->name + "(");
	}
	else {
		txtlines.push<1>("check_abi_result(this->self_abi()." + this->name + "(");
	}

	loop_push_param_format_control<2>{}(txtlines, *this,
		[](const noah::CData& param) { return param.is_pure_abi_type()? "get_abi(" + param.name + ")": param.name; },
		[](const noah::Function& f) { return "put_abi(result))), result);"; }
	);

	if (this->is_void_type()) txtlines.end_add("));");
	txtlines.push("}");
	return txtlines;
}

std::string noah::Function::impl_function(std::string region) const {
	std::string func;
	func += dump_nestype() + " " + region + name + "("; // function type
	for (auto& inparm : params) {
		func += inparm.get_pack_suggest_constref() + ", ";
	}
	func = func.substr(0, func.find_last_of(','));
	func += ")";
	return func;
}
