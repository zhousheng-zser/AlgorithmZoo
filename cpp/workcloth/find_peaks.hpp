#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

namespace glasssix::workcloth
{
	template <class T>
	static inline std::vector<size_t> find_peaks(std::vector<T>& src, T height, int distance)
	{
		size_t length = src.size();
		if (length <= 1) return std::vector<size_t>();

		//we dont need peaks at start and end points
		std::vector<int> sign(length, -1);
		std::vector<T> difference(length, 0);
		std::vector<size_t> temp_out;
		//first-order difference (sign)
		std::adjacent_difference(src.begin(), src.end(), difference.begin());
		difference.erase(difference.begin());
		difference.pop_back();
		for (int i = 0; i < difference.size(); ++i) {
			if (difference[i] >= 0) sign[i] = 1;
		}
		//second-order difference
		for (int j = 1; j < length - 1; ++j)
		{
			int  diff = sign[j] - sign[j - 1];
			if (diff < 0) {
				temp_out.push_back(j);
			}
		}
		if (temp_out.size() == 0 || distance == 0) return temp_out;
		//sort peaks from large to small by src value at peaks
		std::sort(temp_out.begin(), temp_out.end(), [&src](size_t a, size_t b) {
			return (src[a] > src[b]);
			});

		std::vector<size_t> ans;

		std::unordered_map<size_t, int> except;
		for (auto it : temp_out) {
			if (!except.count(it)) // if not included
			{
				ans.push_back(it);
				// update
				size_t left = it - distance > 0 ? it - distance : 0;
				size_t right = it + distance > length - 1 ? length - 1 : it + distance;
				for (size_t i = left; i <= right; ++i)
					++except[i];
			}
		}
		//sort the ans from small to large by index value
		std::sort(ans.begin(), ans.end());
		return ans;
	}

}
