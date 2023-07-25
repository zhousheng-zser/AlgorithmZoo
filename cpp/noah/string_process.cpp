#include <fstream>
#include <sstream>
#include <objbase.h> // windows lib to generate GUID
#include "string_process.hpp"

std::vector<std::string> noah::read_param_file(std::string_view filepath)
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
		removeWord(temp, "\t");
		if (!temp.empty()) {

			output.push_back(temp);
		}
	}
	in.close();
	return output;
}

bool noah::removeWord(std::string& str, std::string word)
{
	// Check if the word is present in string
	// If found, remove it using removeAll()
	if (str.find(word) != std::string::npos)
	{
		size_t p = -1;

		//// To cover the case
		//// if the word is at the
		//// beginning of the string
		//// or anywhere in the middle
		//std::string tempWord = word + " ";
		//while ((p = str.find(word)) != std::string::npos)
		//	str.replace(p, tempWord.length(), "");

		//// To cover the edge case
		//// if the word is at the
		//// end of the string
		//tempWord = " " + word;
		//while ((p = str.find(word)) != std::string::npos)
		//	str.replace(p, tempWord.length(), "");

		while ((p = str.find(word)) != std::string::npos)
			str.replace(p, word.length(), "");

		return true;
	}
	else {
		return false;
	}
}

std::string noah::GuidString()
{
	std::string ans;
	GUID guid;
	HRESULT h = CoCreateGuid(&guid);
	if (h == S_OK) {
		char buf[64] = { 0 };
		sprintf_s(buf, sizeof(buf),
			"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
			guid.Data1, guid.Data2, guid.Data3,
			guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
			guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
		ans = std::string(buf);
	}
	return ans;
}

std::vector<std::string> noah::splite_words_by_char(std::string& text, char space_char) {
	std::vector<std::string> words{};
	std::stringstream sstream(text);
	std::string word;
	while (std::getline(sstream, word, space_char)) {
		//word.erase(std::remove_if(word.begin(), word.end(), ispunct), word.end()); //È¥³ý ispunct:±êµã·ûºÅ
		if (!word.empty())
			words.push_back(word);
	}
	return words;
}
