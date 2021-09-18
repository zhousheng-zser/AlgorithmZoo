#ifndef _FXHOG_HPP_
#define _FXHOG_HPP_
/*
* fxhog is an implementation of DPM feature(CVPR2008, A Discriminatively Trained, Multiscale, Deformable Part Model)
* in C++ and OpenCV3.0+.
* It's re-written accroding to a widely used fhog implementation(https://github.com/joaofaro/KCFcpp).
*/
#include <opencv2\core.hpp>

#define FXHOG_OK             0
#define FXHOG_ERR_FEATUREMAP 1
#define FXHOG_ERR_NORM       2
#define FXHOG_ERR_PCA        3

#define NUM_SECTOR 9

namespace glasssix
{
    class fxhog
    {
    public:
        fxhog() :imageSize(cv::Size(0, 0)),
            cellSize(0),
            sz(cv::Size(0, 0)),
            numFeatures(0) {};
        fxhog& operator=(const fxhog& f)
        {
            this->imageSize = f.imageSize;
            this->cellSize = f.cellSize;
            this->sz = f.sz;
            this->numFeatures = f.numFeatures;
            return *this;
        }
        ~fxhog() {};

        int static_Init(cv::Size _sz, int _cellSize);

        int compute(const cv::Mat& src, cv::Mat& dst, int _cellSize, float thres);

        cv::Size sz;
        int numFeatures;
    private:
        int getFeatureMaps(const cv::Mat& src);
        int normalizeAndTruncate(float thres);
        int PCAFeatureMaps();

        cv::Size imageSize;
        int cellSize;
        cv::Mat map; // sz.y*sz.x X numFeatures 
        cv::Mat originalFeature; // (sz.y+2)*(sz.x+2) X numFeatures X 1
        cv::Mat normalizedFeature; // sz.y*sz.x X numFeatures X 1

        // Used by getFeatureMaps()
        cv::Mat alpha; // (sz.y+2)*cellSize X (sz.x+2)*cellSize X 2
        cv::Mat r; // (sz.y+2)*cellSize X (sz.x+2)*cellSize X 1

        cv::Mat nearest; // CellSize
        cv::Mat w; // 2*CellSize

        cv::Mat dx; // img.rows x img.cols
        cv::Mat dy; // img.rows x img.cols
        cv::Mat dx_ori; // (img.rows+2) x (img.cols+2)
        cv::Mat dy_ori; // (img.rows+2) x (img.cols+2)
        cv::Mat imagePadded; // (img.rows+2) x (img.cols+2)

        const float boundary_x[10] = { 1.000000000, 0.939692616,  0.766044438,
                                         0.499999970, 0.173648104, -0.173648298,
                                        -0.500000060,-0.766044617, -0.939692676,
                                        -1.000000000 };
        const float boundary_y[10] = { 0.000000000, 0.342020154,  0.642787635,
                                         0.866025448, 0.984807789,	0.984807730,
                                         0.866025448, 0.642787457,  0.342020005,
                                         0.000000000 };

        // Used by normalizeAndTruncate()
        cv::Mat partOfNorm; // (sz.y+2)*(sz.x+2) (float)

        // Used by PCAFeatureMaps()

    };
}
#endif //!_FXHOG_HPP_