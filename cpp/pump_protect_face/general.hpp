#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include "Excalibur/pipeline.hpp"
#include "Primitives/tensor_conversions.hpp"

using namespace glasssix;

struct Bbox
{
    int x1;
    int y1;
    int x2;
    int y2;
    int category;
    float score;
    int frame_id;
    std::vector<float> key_points;
    Bbox(int x11, int y11, int x22, int y22, int category_, float score_, int frame_index) :x1(x11), x2(x22), y1(y11), y2(y22), category(category_), score(score_), frame_id(frame_index)
    {}
    Bbox(const Bbox& input) :x1(input.x1), x2(input.x2), y1(input.y1), y2(input.y2), category(input.category), score(input.score), frame_id(input.frame_id), key_points(input.key_points)
    {}
    Bbox(int x11, int y11, int x22, int y22) :x1(x11), x2(x22), y1(y11), y2(y22), category(-1), score(-1), frame_id(-1)
    {}

    Bbox& operator=(const Bbox& input)
    {
        if (this != &input) // 避免自我赋值
        {
            x1 = input.x1;
            x2 = input.x2;
            y1 = input.y1;
            y2 = input.y2;
            category = input.category;
            score = input.score;
            frame_id = input.frame_id;
            key_points = input.key_points;
        }
        return *this;
    }

    int area()
    {
        return (y2 - y1) * (x2 - x1);
    }
};
