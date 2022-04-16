#include "utils.hpp"


void utils::visualizeDetection(cv::Mat& image, std::vector<Detection>& detections)
{
    for (const Detection& detection : detections)
    {
        cv::rectangle(image, detection.box, cv::Scalar(229, 160, 21), 2);

        int x = detection.box.x;
        int y = detection.box.y;

        int conf = (int)std::round(detection.conf * 100);
        std::string label = detection.cls;

        int baseline = 0;
        cv::Size size = cv::getTextSize(label, cv::FONT_ITALIC, 0.4, 2, &baseline);

        cv::rectangle(image,
            cv::Point(x, y - size.height * 1.3), cv::Point(x + size.width, y),
            cv::Scalar(229, 160, 21), -1);

        cv::putText(image, label,
            cv::Point(x, y - 3), cv::FONT_ITALIC,
            0.4, cv::Scalar(255, 255, 255), 1.2);
    }
}
