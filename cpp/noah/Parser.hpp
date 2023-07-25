#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "Project.hpp"
#include "string_process.hpp"

namespace noah {
struct FileInfo {
	std::string dirName;
	std::string fileBaseName;
	std::string fileTypeName;

	FileInfo(std::string path);
	std::string GetCompletePath();
	void ExtractFileName(std::string str);
};

class Parser: Projcet {

	FileInfo fileInfo;
	int parse_stack_depth_ = 0;
	std::unique_ptr<ABICLASS> temp_cls_;

public:
	Parser(std::string src);
	~Parser() {}

	void parse_global(std::string line);
	void write(std::string& AlgorithmZooPath);

private:
	std::vector<std::string> read_param_file_(std::string_view filepath);
};

} // namespace noah