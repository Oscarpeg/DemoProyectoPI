#ifndef IMAGE_PREPROCESSOR_H
#define IMAGE_PREPROCESSOR_H

#include <opencv2/opencv.hpp>

enum class BinarizeMethod {
    OTSU,
    ADAPTIVE,
    MANUAL
};

class ImagePreprocessor {
public:
    cv::Mat toGrayscale(const cv::Mat& img);
    cv::Mat binarize(const cv::Mat& gray, BinarizeMethod method = BinarizeMethod::ADAPTIVE,
                     int manual_threshold = 127);
    cv::Mat denoise(const cv::Mat& img, int kernel_size = 3);
    cv::Mat enhanceContrast(const cv::Mat& gray, double clip_limit = 2.0,
                            cv::Size tile_size = cv::Size(8, 8));
    cv::Mat deskew(const cv::Mat& img);
    cv::Mat fullPreprocess(const cv::Mat& img);
};

#endif // IMAGE_PREPROCESSOR_H
