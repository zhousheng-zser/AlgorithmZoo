#pragma once
#ifndef _TXTLINES_HPP_
#define _TXTLINES_HPP_
#include <iostream>
#include <string>
#include <vector>

namespace noah {

	class TXTLines {
		std::vector<std::string> lines_;

		// Ëõ½ø·ûºÅ
		std::string indentSymbol_(int indenNum) {
			std::string indentSymbol_;
			for (int i = 0; i < indenNum; i++) {
				indentSymbol_ += "    ";
			}
			return indentSymbol_;
		};

		template<typename SS, typename T> void sstr_(SS& o, T t)
		{
			o << t;
		}

	public:
		TXTLines() {};

		TXTLines(const TXTLines& other) {
			lines_ = other.lines_;
		};

		TXTLines& operator+= (TXTLines& other)
		{
			lines_.insert(lines_.end(), other.lines_.begin(), other.lines_.end());
			return *this;
		}

		template<unsigned int Indent_ = 0>
		void push(TXTLines other) {
			for (const auto& line : other.lines_)
				lines_.push_back(indentSymbol_(Indent_) + line);
		};

		void push(int indent, std::string sentence) {
			lines_.push_back(indentSymbol_(indent) + sentence);
		};

		template<unsigned int Indent_ = 0, typename... ARG> 
		void push(ARG... arg)
		{
			std::stringstream sstream;
			sstream << indentSymbol_(Indent_);
			int arr[] = { (sstr_(sstream,arg),0)... };
			lines_.push_back(sstream.str());
		}

		// return "// ..." (code annotation format)
		template<unsigned int Indent_ = 0, typename... ARG>
		void suggest_push(ARG... arg)
		{
			std::stringstream sstream;
			sstream << indentSymbol_(Indent_) << "//";
			int arr[] = { (sstr_(sstream,arg),0)... };
			lines_.push_back(sstream.str());
		}

		void push(std::string sentence="") {
			lines_.push_back(sentence);
		};

		// symbol 0:"" 1:<>
		template<bool Symbol_ = false>
		void push_include(std::string srcfile) {
			if constexpr (Symbol_) {
				lines_.push_back("#include <" + srcfile + ">");
			}
			else {
				lines_.push_back("#include \"" + srcfile + "\"");
			}
		};

		// TODO: {...,"xxx"}end_add("_abc") -> {...,"xxx_abc"}
		void end_add(std::string add_str) {
			*lines_.rbegin() += add_str;
		};

		std::string export_string() {
			std::string textline;
			for (const auto& line : lines_) {
				textline += line + "\n";
			}
			return textline;
		};

		// TODO: {...," xxx,  "} -> {...," xxx"}
		void del_end_comma() {
			auto& tail_string = *lines_.rbegin();
			tail_string = tail_string.substr(0, tail_string.find_last_of(','));			
		}
	};

}
#endif // !_TXTLINES_HPP_
