#include "head/header.hpp"
#include "head/files.hpp"
#include "Parser.hpp"
#include <fstream>
#include <algorithm>
#include <iterator>
#include <set>
#include <chrono>
#include <random>
#include <iostream>
#include <sys/stat.h>

using std::cout; 
using std::cin;
using std::endl;
using std::vector;
using std::string;
using zfc = std::string;

inline void run(const std::string& template_src, std::string zoopath = "") {

	noah::Parser parser(template_src);
	parser.write(zoopath);
}

int main(int argc, char** argv) {
	if (argc < 2)
	{
		noah::show_usage();
		return -1;
	}
	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			noah::show_usage();
			return -1;
		}
	}

	std::string template_src_code = noah::absolutePath(std::string(argv[1]));

	std::string AlgorithmzooPath = template_src_code.substr(0, template_src_code.find_last_of('/')); // 未指定参数二, 以template_src_code所在路径为输出路径
	if (argc == 3) {
		AlgorithmzooPath = std::string(argv[2]);

		auto last_path_cur = AlgorithmzooPath.find_last_of('/');
		std::string last_path = AlgorithmzooPath.substr(last_path_cur + 1, AlgorithmzooPath.size() - last_path_cur);
		// 极为粗糙的误输入(与AlgorithmZoo相似的字符串)检测
		float Sim = noah::findStringCharSetSimilarity(last_path, "AlgorithmZoo");
		if (Sim > 0.7 && Sim < 0.998) {
			std::cout << "[ ERROR ] MayBe U used error AlgorithmZoo path: " << AlgorithmzooPath << std::endl;
			std::cout << "          Please check, and program exit !" << std::endl;
			return -1;
		}
	}
	
	std::cout << "[ INFO ] Use Export Path: " << AlgorithmzooPath << std::endl;

	run(template_src_code, AlgorithmzooPath);

}
