#include "deep_sort.hpp"

DeepSort::DeepSort(std::wstring model_path, const int nn_budget, const float max_cosine_distance)
{
	//Ort::AllocatorWithDefaultOptions allocator;

	//my_tracker = new tracker(max_cosine_distance, nn_budget);

	//try
	//{
	//	pipeline_ = new Ort::Session(env, model_path.c_str(), Ort::SessionOptions());
	//}
	//catch (const std::exception& e)
	//{
	//	std::cout << e.what() << std::endl;
	//}
	//
	//inputNames.push_back(pipeline_->GetInputName(0, allocator));
	//outputNames.push_back(pipeline_->GetOutputName(0, allocator));
}

DeepSort::DeepSort(std::string_view deepsort_racy_path, int device, const int nn_budget, const float max_cosine_distance)
	: DeepSort(glasssix::hardcode::get_model_params("deepsort"), deepsort_racy_path, device)
{
}

DeepSort::DeepSort(std::string_view deepsort_phai, std::string_view deepsort_racy_path, int device, const int nn_budget, const float max_cosine_distance)
	: deepsort_instance_(deepsort_phai, deepsort_racy_path, device)
{
	my_tracker = new tracker(max_cosine_distance, nn_budget);
}


DeepSort::DeepSort(const std::vector<std::string>& deepsort_phai, std::string_view deepsort_racy_path, int device, const int nn_budget, const float max_cosine_distance)
	: deepsort_instance_(deepsort_phai, deepsort_racy_path, device)
{
	my_tracker = new tracker(max_cosine_distance, nn_budget);
}

std::vector<RESULT_DATA> DeepSort::update(cv::Mat frame, std::vector<Detection> &detect_bbox)
{
	// STEP 1:
	//load detections : {box, confidence, feature}
	DETECTIONS detections;
	for (size_t in = 0; in < detect_bbox.size(); in++) {
		get_detections(DETECTBOX(detect_bbox[in].box.x, detect_bbox[in].box.y, detect_bbox[in].box.width, detect_bbox[in].box.height), 1.0, detections);
	}
	get_all_feature(frame, detections);

	// STEP 2:
	// tracker predict 
	// KM prediction
	my_tracker->predict();

	// STEP 3:
	// detect + predict
	// Hungarian match { tracker predict & detections }
	// tracker KM update
	my_tracker->update(detections);

	std::vector<RESULT_DATA> result;
	for (Track& track : my_tracker->tracks) {
		if (!track.is_confirmed() || track.time_since_update > 1) continue;
		result.push_back(std::make_pair(track.track_id, track.to_tlwh()));
	}

	return result;
}

void DeepSort::get_all_feature(cv::Mat& frame, DETECTIONS& de) {
	for (int in = 0; in < de.size(); in++) {
		get_feature(frame, de[in]);
	}
}

void DeepSort::get_feature(cv::Mat& frame, DETECTION_ROW& d) {
	//Ort::TypeInfo inputTypeInfo = pipeline_->GetInputTypeInfo(0);
	//std::vector<int64_t> input_shape_suit = inputTypeInfo.GetTensorTypeAndShapeInfo().GetShape();

	this->deepsort_instance_;

	/*cv::Mat img(height, width, CV_8UC3);
	std::copy(data, data + 3 * width * height, img.data);*/
	std::vector<float> mean_{ 0.485, 0.456, 0.406 };
	std::vector<float> std_{ 0.229, 0.224, 0.225 };
	//cv::Mat input_img(img.rows, img.cols, img.type());
	cv::Rect cor;
	cv::Mat suit_img;
	cor.x = d.tlwh[0];
	cor.y = d.tlwh[1];
	cor.width = d.tlwh[2];
	cor.height = d.tlwh[3];
	suit_img = cut_img(frame, cor);//nhwc

	std::vector<float> suit_img_input = trans_num_suit(suit_img, cv::Size(64, 128)/*wh*/, mean_, std_);

	// -------- defualt
	// make input
	//std::vector<Ort::Value> input_tensors_suit;

	// load input
	//Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(
	//	OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
	//input_tensors_suit.emplace_back(Ort::Value::CreateTensor<float>(
	//	memoryInfo,
	//	suit_img_input.data(),
	//	suit_img.cols * suit_img.rows * suit_img.channels(),
	//	input_shape_suit.data(),
	//	input_shape_suit.size()
	//	));

	// run 
	//std::vector<Ort::Value> output_tensors_suit = this->pipeline_->Run(Ort::RunOptions{ nullptr },
	//	inputNames.data(),
	//	input_tensors_suit.data(),
	//	1,
	//	outputNames.data(),
	//	outputNames.size());

	// ouput
	//auto singel_feature = output_tensors_suit[0].GetTensorData<float>();

	//auto shape_info = output_tensors_suit[0].GetTensorTypeAndShapeInfo().GetShape();
	//std::copy(singel_feature, singel_feature + shape_info[0] * shape_info[1], d.feature.data());
	
	//----glasssix
	// make input
	std::shared_ptr<glasssix::memory::tensor<std::uint8_t>> input_tensors_suit;
	std::vector<int> shape{ 1, 128, 64, 3};//NHWC
	auto cache_ = std::make_shared<glasssix::memory::tensor<std::uint8_t>>(shape, -1, glasssix::memory::orderType::NHWC/*, &memory::pool_allocator_default<std::uint8_t>::get()*/);
	std::copy(suit_img_input.begin(), suit_img_input.end(), cache_->mutable_cpu_data());
	glasssix::excalibur::resize_cpu(cache_, input_tensors_suit, shape[1], shape[2]);
	cache_->convert_order();
	auto input_tensor = cache_ | glasssix::memory::tensor_convert_to<float>;
	// run
	auto ouput_tensor = this->deepsort_instance_.forward(input_tensor);


	auto output_tensor_some = ouput_tensor["output0"];//临时 how to write this
	auto size = output_tensor_some->data_shape();
	int count = size[0] * size[1] * size[2] * size[3];
	auto singel_feature = output_tensor_some->mutable_cpu_data();
	assert(count == 512);//临时
	std::copy(singel_feature, singel_feature + count, d.feature.data());

}

void DeepSort::get_detections(DETECTBOX box, float confidence, DETECTIONS& d)
{
	DETECTION_ROW tmpRow;
	tmpRow.tlwh = box;//DETECTBOX(x, y, w, h);

	tmpRow.confidence = confidence;
	d.push_back(tmpRow);
}

inline void DeepSort::safty_cut(cv::Mat& img, cv::Mat& dst, cv::Rect roi)
{
	int width = roi.width;
	int height = roi.height;
	int x = roi.x;
	int y = roi.y;

	cv::Mat mat(height, width, img.type(), cv::Scalar(0));//赋值一个mat，确定通道数，并把Scalar中的值填到mat中
	int _x = x;
	int _y = y;
	int _width = width;
	int _height = height;
	if (x < 0)
	{
		_x = 0;
		_width = width + x;
	}

	if (_x + _width > img.cols)
		_width = img.cols - _x;

	if (y < 0)
	{
		_y = 0;
		_height = height + y;
	}

	if (_y + _height > img.rows)
		_height = img.rows - _y;

	img(cv::Rect(_x, _y, _width, _height)).copyTo(mat(cv::Rect(_x - x, _y - y, _width, _height))); //图片剪切
	dst = mat;
}
cv::Mat DeepSort::cut_img(cv::Mat img, cv::Rect box_info) {
	cv::Mat tempo;
	cv::Mat dst;

	cv::resize(img, dst, cv::Size(64, 128));
	return dst;
}
std::vector<float> DeepSort::trans_num_suit(cv::Mat& dst, const cv::Size& output_shape, const std::vector<float>& mean_, const std::vector<float>& std_)
{
	cv::cvtColor(dst, dst, cv::COLOR_BGR2RGB);
	dst.convertTo(dst, CV_32F, 1.0 / 255);

	//RGB格式归一化方式
	int width = output_shape.width;
	int height = output_shape.height;
	int channels = 3;

	const int row = height;
	const int col = width;

	//把cv::mat数据放在std:vector里
	//HWC转CHW排序格式,并做减均值除方差归一化处理
	std::vector<float> input_image(width * height * channels, 0);

	//std::copy(reinterpret_cast<float*>(dst.data), reinterpret_cast<float*>(dst.data) + width * height * channels, input_image.data());
	fill(input_image.begin(), input_image.end(), 0.f);
	for (int c = 0; c < 3; c++) {
		for (int i = 0; i < row; i++) {
			for (int j = 0; j < col; j++) {
				input_image[c * row * col + i * col + j] = (dst.ptr<float>(i)[j * 3 + c] - mean_[c]) / std_[c];
			}
		}
	}
	return input_image;
}

