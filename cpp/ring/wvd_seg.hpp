#pragma once
#ifndef _WVD_SEG_HPP_
#define _WVD_SEG_HPP_

#include<vector>
#include<opencv2/opencv.hpp>

namespace glasssix
{
namespace ring
{

void slant_text_correction_test(cv::Mat& pic);

cv::Mat slant_text_correction(cv::Mat& image);

template <typename DType = float>
class WVD {
	using SignalType = std::vector<std::complex<DType>>;
	SignalType signal;
	int n_fbins;
public:

	WVD(SignalType& inSignal) {
		signal = inSignal;
		n_fbins = inSignal.size();
	}

	std::vector<std::vector<std::complex<float>>> compute(bool faster_CV_DFT = false) {
		int tlength = n_fbins;
		int N = n_fbins;
		std::vector<std::complex<float>> sigwv(3 * tlength, { 0,0 });
		std::vector<std::complex<float>> sigwvconj(3 * tlength, { 0,0 });
		for (int i = 0; i < tlength; i++) {
			sigwv[i + tlength] = signal[i];
			sigwvconj[i + tlength] = std::conj(sigwv[i + tlength]);
		}

		std::vector<std::complex<float>> C(N, { 0,0 });
		std::vector<std::vector<std::complex<float>>> tfr;

		for (int t = 0; t < tlength; t++) {
			for (int i = 0; i < (int)std::ceil(tlength / 2); i++) {
				C[i] = sigwv[t + tlength + i] * sigwvconj[t + tlength - i];
				// Magic
				if (t >= i) { C[i] = sigwv[t + tlength + i] * sigwvconj[t + tlength - i]; }
				else { C[i] = { 0,0 }; }
				if (i >= 1 && i <= (int)std::round(tlength / 2) - 1) { C[tlength - i] = std::conj(C[i]); }
			}

			std::vector<std::complex<float>> Co = STD_CV_DFT(C, faster_CV_DFT);
			tfr.push_back(Co);
		}
		return tfr;
	}

	std::vector<std::complex<float>> STD_CV_DFT(std::vector<std::complex<float>> yn, bool faster = false) {
		// copy
		const int len = yn.size();
		const int best_len = faster ? cv::getOptimalDFTSize(len) : len;
		std::vector<float> rel_data(best_len, 0);
		std::vector<float> img_data(best_len, 0);

		for (int i = 0; i < len; ++i) {
			rel_data[i] = yn[i].real();
			img_data[i] = yn[i].imag();
		}

		cv::Mat relMat = cv::Mat(rel_data);
		cv::Mat imgMat = cv::Mat(img_data);
		cv::Mat plane[] = { cv::Mat_<float>(relMat), cv::Mat_<float>(imgMat) };

		cv::Mat complexIm;
		cv::merge(plane, 2, complexIm); // 合并通道 （把两个矩阵合并为一个2通道的Mat类容器）
		cv::dft(complexIm, complexIm, 0); // 进行傅立叶变换，结果保存在自身

		cv::split(complexIm, plane); // plannes[0]=Re(DFT(I))即实部,plannes[1]=Im(DFT(I))即虚部
		std::vector<float> amplite_Re;
		amplite_Re = plane[0];//cv::Mat转为vector
		std::vector<float> amplite_Im;
		amplite_Im = plane[1];

		std::vector<std::complex<float>> rst(best_len);
		for (int i = 0; i < best_len; ++i) {
			rst[i] = { amplite_Re[i], amplite_Im[i] };
		}
		return rst;
	}
};

} // ring
} // glasssix
#endif // _WVD_SEG_HPP_