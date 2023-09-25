
#include "../common/include/TRTWrapper/engine.hpp"

#include <opencv2/opencv.hpp>


cv::Mat letterbox(const cv::Mat& img, int32_t outWidth, int32_t outHeight)
{
	cv::Mat padded_img;

	int img_w = img.cols;
	int img_h = img.rows;

	if (img_h != outHeight || img_w != outWidth)
	{
		float scale = std::min(static_cast<float>(outWidth) / img_w, static_cast<float>(outHeight) / img_h);
		int pad_w = static_cast<int>(img_w * scale);
		int pad_h = static_cast<int>(img_h * scale);

		int pad_horizontal = (outWidth - pad_w) / 2;
		int pad_vertical = (outHeight - pad_h) / 2;

		cv::copyMakeBorder(img, padded_img, 0, pad_vertical + pad_vertical, 0, pad_horizontal + pad_horizontal, cv::BORDER_CONSTANT, cv::Scalar{ 0,0,0 });
	}
	else
	{
		padded_img = img.clone();
	}

	return padded_img;
}

int main()
{
	// option for load onnx to build engine 
	Options options;

	options.precision = Precision::FP16;

	options.deviceIndex = 0;

	Engine engine(options);

	// mean & std
	std::array<float, 3> mean{ 0.f, 0.f, 0.f };
	std::array<float, 3> var{ 1.f , 1.f, 1.f };
	bool normalize = true;

	auto model_path = "./tumble.fp16.engine";

	// load engine
	bool succ = engine.loadNetwork(model_path);

	if (!succ) {
		throw std::runtime_error("Unable to load TRT engine.");
	}

	//run inference
	// read image ( this image is after letterbox)
	const std::string image_path = "./test_640.jpg";
	auto input = cv::imread(image_path);
	if (input.empty()) {
		std::cout << "Image not found at: " << image_path << std::endl;
		return 0;
	}
	// letterbox

	int model_w = 640;
	int model_h = 640;
	auto blob = letterbox(input, model_w, model_h);

	size_t bathSize = options.optBatchSize;


	// convert blob into Excalibur Tensor
	std::shared_ptr<glasssix::memory::tensor<uint8_t>> input_tensor_blob(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, blob.rows, blob.cols, 3}, -1, glasssix::memory::NHWC));
	
	// mat convert into tensor
	std::copy(blob.data, blob.data + blob.step[0] * blob.rows, input_tensor_blob->mutable_cpu_data());

	// NHWC into NCHW tensor
	input_tensor_blob->convert_order();
	auto input_tensor = input_tensor_blob | glasssix::memory::tensor_convert_to<float>;

	auto blob_tensor = engine.blobFromMats(input_tensor, mean, var, normalize);

	// inference

	auto output = engine.runInference(blob_tensor);

	return 0;
}
