#include "../ABICLASSIR.hpp"
#include "../string_process.hpp"


noah::const_ref peelCRef_(std::string& raw_type) {
	std::vector<std::string> words{ "CONST_REF","C&","C_REF","const_ref","c_ref","c&" };
	bool has_cf = false;
	for (auto& word : words) {
		has_cf = noah::removeWord(raw_type, word) || has_cf;
	}
	return has_cf ? noah::const_ref::is_cref : noah::const_ref::no_cref;
}

// 去除模板代码文本中别名、静态、内联等类型修饰符
void peelTypeModifier(std::string& raw_type) {
	auto ifconst_ref = peelCRef_(raw_type);
}

noah::CData::CData(std::string raw_type, std::string d_name) : name(d_name) {
	peelTypeModifier(raw_type);
	raw_type.erase(std::remove_if(raw_type.begin(), raw_type.end(), isblank), raw_type.end()); // clear space(blank)
	nestype = read2nestype(raw_type); //split d_type and parse to NestedType
};

noah::CData::CData(const CData& copyCData) {
	name = copyCData.name;
	nestype = copyCData.nestype;
};

noah::CData& noah::CData::operator=(const noah::CData& copyCData) {
	if (this == &copyCData) return *this;
	name = copyCData.name;
	nestype = copyCData.nestype;
	return *this;
}

bool noah::CData::is_void_type() const {
	return nestype.empty() || nestype[0].basic_t==BasicAType::AT_VOID;
}

bool noah::CData::is_pure_abi_type() const {
	return nestype[0].is_pure_abi_type();
}

std::string noah::CData::get_pack_abi_in_t() const {
	if (is_pure_abi_type())
		return "abi_in_t<" + dump_nestype() + "> " + name;
	else
		return dump_nestype() + " " + name;
}

std::string noah::CData::get_pack_abi_out_t() const {
	return "abi_out_t<" + dump_nestype() + "> result";
}

std::string noah::CData::get_pack_suggest_constref() const {
	//if(cref_status_==noah::const_ref::is_cref)
	if (SUGGEST_CONST_REF.count(this->nestype[0].basic_t))
		return "const " + dump_nestype() + "& " + name;
	else
		return dump_nestype() + " " + name;
}

std::string noah::CData::get_stand_param() const {
	return dump_nestype() + " " + name;
}

std::string noah::CData::dump_nestype() const {
	return dump_nestype([](const AType& atype) {return atype.basic_name; });
}

std::string noah::CData::dump_nestype(std::function<std::string(const noah::AType&)> typeBasicNameControl) const {
	std::string rst;

	if (nestype.size() == 1) {
		return typeBasicNameControl(nestype[0]);
	}
	//rst += typelist;
	std::stack<int> neststk; neststk.push(1);

	for (auto& a_type : nestype) {
		if (a_type.t_num) {
			rst += typeBasicNameControl(a_type);
			rst += '<';
			neststk.push(a_type.t_num);
		}
		else { // t_num==0
			rst += typeBasicNameControl(a_type);
			if (neststk.top() > 1) {
				neststk.top()--;
				rst += ',';
			}
			else if (neststk.top() == 1) {
				neststk.top()--;
				if (*rst.rbegin() == ',') rst.pop_back();
				rst += '>';
			}
		}
		if (neststk.top() <= 0) {
			neststk.pop();
			/*if (!neststk.empty())*/ neststk.top()--;
			rst += ',';
		}
		if (/*!neststk.empty() && */neststk.top() <= 0) {
			if (*rst.rbegin() == ',')rst.pop_back();
			rst += '>';
			rst += ',';
		}
	}
	if (*rst.rbegin() == ',')rst.pop_back();
	auto cl = std::count(rst.begin(), rst.end(), '<');
	auto cr = std::count(rst.begin(), rst.end(), '>');
	if (cl < cr)
		rst.pop_back();
	else if (cl > cr)
		rst += ">";

	return rst;
}

void parseType_(std::vector<noah::AType>& rst, std::string d_type) {
	auto h = d_type.find('<');
	auto t = d_type.rfind('>');
	if (h == std::string::npos || t == std::string::npos) {
		rst.push_back({ d_type ,0 });
		return;
	}
	auto sub0 = d_type.substr(0, h); //outer type
	auto sub1 = d_type.substr(h + 1, t - h - 1);// inter types
	int deep = 0;
	for (auto iter = sub1.begin(); iter != sub1.end() - 1; iter++) {
		char& chr = *iter;
		switch (chr)
		{
		case '<':
			deep++;
			break;
		case '>':
			deep--;
			break;
		case ',':
			if (deep == 0) *iter = '@';
			break;
		default:
			break;
		}
	}
	std::vector<std::string> interTypes = noah::splite_words_by_char(sub1, '@');
	noah::AType temp{ sub0 ,static_cast<uint32_t>(interTypes.size()) };
	rst.push_back(temp);

	for (auto subtypes : interTypes) {
		parseType_(rst, subtypes);
	}
}

std::vector<noah::AType> noah::CData::read2nestype(std::string d_type) {
	std::vector<AType> rst;
	parseType_(rst, d_type); // recursion
	return rst;
}
