#ifndef FINDPEAKS_H
#define FINDPEAKS_H

#include <vector>

namespace glasssix::workcloth
{

	using namespace std;

	template<typename T>
	inline vector<int> findPeaks(
		vector<T> x,
		vector<T> height = {},
		size_t distance = 0)
	{
		vector<int> peaks;
		for (int i = 1; i < x.size() - 1; i++) {

			if (x[i - 1] < x[i]) {
				int iahead = i + 1;
				while (iahead < x.size() - 1 && x[iahead] == x[i]) {
					iahead++;
				}
				if (x[iahead] < x[i]) {
					bool peakflag = true;
					// Evaluate height condition
					if (height.size() == 2) {

						int currentPeakIndex = (i + iahead - 1) / 2;

						if (x[currentPeakIndex] < height[0] || x[currentPeakIndex] > height[1]) {

							peakflag = false;
						}
					}
					if (peakflag) {
						peaks.push_back((i + iahead - 1) / 2);
					}
					i = iahead;
				}
			}
		}

		// Evaluate distance condition
		if (distance > 0) {
			vector<bool> eraseIndex(peaks.size(), false);
			vector<int> sortPeaks = peaks;
			sort(sortPeaks.begin(), sortPeaks.end(), [&x](int pos1, int pos2) {return (x[pos1] > x[pos2]); });	//sort peaks by the value of x[peaks]

			for (int i = 0; i < sortPeaks.size(); i++) {

				int j = find(peaks.begin(), peaks.end(), sortPeaks[i]) - peaks.begin();

				if (eraseIndex[j]) {
					continue;
				}

				int k = j - 1;

				while (k >= 0 && std::abs(peaks[j] - peaks[k]) < distance) {
					eraseIndex[k] = true;
					k--;
				}

				k = j + 1;

				while (k < peaks.size() && std::abs(peaks[j] - peaks[k]) < distance) {
					eraseIndex[k] = true;
					k++;
				}
			}

			int eraseCount = 0;
			for (int i = 0; i < eraseIndex.size(); i++) {
				if (eraseIndex[i]) {
					peaks.erase(peaks.begin() + i - eraseCount);
					eraseCount++;
				}
			}
		}
		return peaks;
	}

}

#endif
