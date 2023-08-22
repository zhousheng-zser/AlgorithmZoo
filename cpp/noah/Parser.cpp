#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "Parser.hpp"
#include "head/files.hpp"

noah::FileInfo::FileInfo(std::string path)
{
	if (path.find('/') == std::string::npos) {
		this->dirName = "./";
		fileBaseName = path.substr(0, path.find_last_of('.'));
		if (path.find('.') != std::string::npos)
			fileTypeName = path.substr(path.find_last_of('.'));
	}
	else {
		ExtractFileName(path);
	}	
}

std::string noah::FileInfo::GetCompletePath() {
	std::string std_dirName = *dirName.rbegin() == '/' ? dirName : dirName + '/';
	if (fileTypeName[0] == '.')
		return dirName + fileBaseName + fileTypeName;
	else
		return dirName + fileBaseName + '.' + fileTypeName;
}

void noah::FileInfo::ExtractFileName(std::string str) {
	//目录、不带后缀名的文件名、后缀
	std::string _dir_name_;
	std::string _file_base_name_;
	std::string _file_type_name_;

	//待匹配的子序列
	std::string patternUnx = "/";
	std::string patternWin = "\\";
	// 查找容器内子序列的最后一次出现的位置，在[str.begin()，str.end ())内搜索由[pattern.begin（）, pattern.end（)）
	// 组成的子序列，然后将迭代器返回到其第一个元素，即pattern.begin(），若没有发现，返回-1
	// 与std::search（）类似，后者返回子序列第一次出现的位置

	auto resultUnx = std::find_end(str.begin(), str.end(), patternUnx.begin(), patternUnx.end());
	resultUnx = resultUnx == str.end() ? str.begin() : resultUnx;
	auto resultWin = std::find_end(str.begin(), str.end(), patternWin.begin(), patternWin.end());
	resultWin = resultWin == str.end() ? str.begin() : resultWin;
	std::string::iterator result = resultUnx > resultWin ? resultUnx : resultWin;
	if (result != str.end())
	{
		//substr()截取字符串子序列，第一个参数为开始索引，第二参数是子序列长度
		//substring(截取字符串子序列，第一个参数为开始索引，第二参数是结束索引
		//目录
		_dir_name_ = str.substr(0, std::distance(str.begin(), result) + 1);
		//带后缀名的文件名
		auto fileName = str.substr(std::distance(str.begin(), result) + 1);
		//不带后缀名的文件名
		_file_base_name_ = fileName.substr(0, fileName.find_last_of('.'));
		if (fileName.find('.') != std::string::npos)
			_file_type_name_ = fileName.substr(fileName.find_last_of('.'));
	}

	dirName = _dir_name_;
	fileBaseName = _file_base_name_;
	fileTypeName = _file_type_name_;
}

static std::map<std::string, noah::ClassType> ClassTypeMap{
	{"EXP_ABICLASS",noah::ClassType::ExpClass},
	{"ABICLASS",noah::ClassType::AstClass},
};

void noah::Parser::parse_global(std::string line) {
	auto spline = splite_words_by_char(line, ' '); // splite_by_space

	if (parse_stack_depth_ != 0) return;

	// 文本出现类定义，关键字判断类是否为导出类地位
	if (ClassTypeMap.count(spline[0])) {
		auto abicls_type = ClassTypeMap.at(spline[0]);
		temp_cls_ = std::make_unique<noah::ABICLASS>(this->get_name_());
		temp_cls_->set_class_name_and_status(spline[1], abicls_type);
	}
}

void noah::Parser::write(std::string& AlgorithmZooPath) {
	std::string src_path;
	std::string cmake_path;

	if (ACCESS(AlgorithmZooPath.c_str(), 0) != 0) {
		std::cout << "[Parser.cpp][Parser::write] invalid AlgorithmZooPath !" << std::endl;
		std::cout << "[Parser.cpp][Parser::write] using template code file path replace AlgorithmZooPath!" << std::endl;
		src_path = fileInfo.dirName + "cpp/" + fileInfo.fileBaseName;
		cmake_path = fileInfo.dirName + "cmake/" + noah::Capitalize(fileInfo.fileBaseName);
	}
	else {
		AlgorithmZooPath = *AlgorithmZooPath.rbegin() == '/' ? AlgorithmZooPath : AlgorithmZooPath + '/';
		src_path = AlgorithmZooPath + "cpp/" + fileInfo.fileBaseName;
		cmake_path = AlgorithmZooPath + "cmake/" + noah::Capitalize(fileInfo.fileBaseName);
	}

	noah::clearDirectory(src_path);
	noah::clearDirectory(cmake_path);
	this->write_code_(src_path);
	this->write_cmake_(cmake_path);
}

std::vector<std::string> noah::Parser::read_param_file_(std::string_view filepath)
{
	std::vector<std::string> output;
	std::ifstream in{ std::string(filepath) };
	std::string temp;
	if (!in.is_open())
	{
		return output;
	}
	while (std::getline(in, temp))
	{
		if (!temp.empty()) {
			auto temp_if_has_ = noah::removeWord(temp, "\t");
			output.push_back(temp);
		}
	}
	in.close();
	return output;
}

noah::Parser::Parser(std::string src) : fileInfo(src) {

	//std::cout << fileInfo.GetCompletePath();

	init_(fileInfo.fileBaseName);

	auto code_file = read_param_file(src);

	for (auto line : code_file) {
		erase_cpp_annotate(line);
		std::string vailineCheck = line;
		vailineCheck.erase(std::remove_if(vailineCheck.begin(), vailineCheck.end(), iscntrl), vailineCheck.end()); //clear contral
		vailineCheck.erase(std::remove_if(vailineCheck.begin(), vailineCheck.end(), isblank), vailineCheck.end()); //clear space(blank)
		removeWord(vailineCheck, ",");
		if (vailineCheck.empty()) //去除标点与空格后亦不为空
			continue;

		else if (parse_stack_depth_ == 0) {
			parse_global(line);
			if (line.find('{') != std::string::npos) parse_stack_depth_++;
		}
		else {
			if (line.find('}') != std::string::npos) {
				// 对ExpAbi强制注册的接口方法，如version
				if (temp_cls_->get_status() == ClassType::ExpClass) temp_cls_->functions.push_back(noah::Function{ "STR version()" });
				// ABI Class读入完毕，载入列表
				this->push_cls_(*temp_cls_);
				parse_stack_depth_--;
				temp_cls_.release();
			}
			else if (!line.empty()) {
				noah::Function function_temp{ line };
				//屏蔽version 手工为每个Expoprt ABI Class增加version接口，因为version是强制接口不接受外部定义
				if (function_temp.special_function_check() != noah::Function::functionType::version)
					temp_cls_->functions.push_back(function_temp);
			}
		}
	}
}