#include "GenPipline.hpp"

using namespace glasssix;

cv::Mat GenPipline::letter_image_(cv::Mat img, int hope_w, int hope_h)
{
	int H = img.rows;
	int W = img.cols;

	if (H == hope_h && W == hope_w) return img;

	float ratio_w = (float)W / (float)hope_w;
	float ratio_h = (float)H / (float)hope_h;
	cv::Mat resize_img;
	if (ratio_w == ratio_h)
		cv::resize(img, resize_img, cv::Size2i{ hope_w, hope_h });
	else if (ratio_w > ratio_h) {
		int new_x = hope_w;
		int new_y = (int)(H / ratio_w);
		int pad1 = (int)((hope_h - new_y) / 2);
		int pad2 = hope_h - new_y - pad1;
		cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
		cv::copyMakeBorder(resize_img, resize_img, pad1, pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
	}
	else {
		int new_y = hope_h;
		int new_x = (int)(W / ratio_h);
		int pad1 = (int)((hope_w - new_x) / 2);
		int pad2 = hope_w - new_x - pad1;
		cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
		cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT, cv::Scalar{ 127,127,127 });
	}
	return resize_img;
}

std::string GenPipline::pipTypeInfo() {
	switch (pipType_)
	{
	case GenPipline::PipType::rknn:
		return "rknn";
		break;
	case GenPipline::PipType::excalibur:
		return "excalibur";
		break;
	case GenPipline::PipType::onnx:
		return "onnx";
		break;
	default:
		return "unknown";
		break;
	}
}

int GenPipline::pipTypeID() {
	return static_cast<int>(pipType_);
}

void GenPipline::set_postprocessing(postprocessing_function ppfunc) {
	if_use_ppfunc = true;
	ppfunc_ = ppfunc;
}

void GenPipline::set_postprocessing(std::string ppfunc_name, std::map<std::string, postprocessing_function>& postprocessing_market) {
	if (postprocessing_market.count(ppfunc_name)) {
		std::cout << pipTypeInfo() << " pipline load postprocessing \"" << ppfunc_name << "\"" << std::endl;
		set_postprocessing(postprocessing_market[ppfunc_name]);
	}
	else if (!ppfunc_name.empty()) {
		std::cout << "postprocessing market not exits \"" << ppfunc_name << "\"" << std::endl;
		std::cout << "[INFO] POSTPROCESSING MARKET: { ";
		for (auto& ppf : postprocessing_market) std::cout << ppf.first << ", ";
		std::cout << "}" << std::endl;
		std::cout << "please check postprocessing config!" << std::endl;
	}
}

void GenPipline::set_image_preprocess(int imgLetterReSize, bool convertBGR = false) {
	if (imgLetterReSize > 0 || convertBGR) {
		if_image_preprocess_ = true;
		convertBGR_ = convertBGR;
		imgLetterReSize_ = imgLetterReSize;
		std::cout << pipTypeInfo() << " net set image preprocess for forward, convertBGR_ = " << convertBGR_ << ", imgLetterReSize = " << imgLetterReSize_ << std::endl;
		std::cout << "if program crash, check imgLetterReSize^2 EQ rknn input shape" << std::endl;;
	}
}