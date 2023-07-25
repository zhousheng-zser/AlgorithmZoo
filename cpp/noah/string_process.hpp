#pragma once
#ifndef __STRING_PROCESS_HPP__
#define __STRING_PROCESS_HPP__
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <set>

namespace noah {
/// <summary>
/// 按行读取txt文件
/// </summary>
/// <param name="filepath"></param>
/// <returns></returns>
std::vector<std::string> read_param_file(std::string_view filepath);

/// <summary>
/// 移除字符串指定单词
/// </summary>
/// <param name="str"></param>
/// <param name="word"></param>
/// <returns></returns>
bool removeWord(std::string& str, std::string word);

std::string GuidString();

std::vector<std::string> splite_words_by_char(std::string& text, char space_char = ' ');

static inline std::string Capitalize(std::string str) {
	std::string Capstr = str;
	Capstr[0] = std::toupper(Capstr[0]);
	return Capstr;
}

static inline std::string lower(std::string str) {
	std::string lowerstr = str;
	std::transform(lowerstr.begin(), lowerstr.end(), lowerstr.begin(), std::tolower);
	return lowerstr;
}

static inline std::string UPPER(std::string str) {
	std::string UPPERstr = str;
	std::transform(UPPERstr.begin(), UPPERstr.end(), UPPERstr.begin(), std::toupper);
	return UPPERstr;
}

static inline void erase_cpp_annotate(std::string& str) {
	std::string pattern = "//";
	auto cur = std::find_first_of(str.begin(), str.end(), pattern.begin(), pattern.end());
	str = str.substr(0, std::distance(str.begin(), cur));
}


static inline float findStringCharSetSimilarity(std::string compare_str, std::string stand_str) {
	if (compare_str == stand_str)
		return 1;
	else {
		std::set<char> firstSet(compare_str.begin(), compare_str.end());

		int counter = 0;
		for (auto& c : stand_str) {
			counter += firstSet.count(c);
		}
		
		float cr = counter;
		float len = stand_str.size() > compare_str.size() ? stand_str.size() : compare_str.size();
		return cr / len - 0.1;
	}

}

} // namespace noah
#endif