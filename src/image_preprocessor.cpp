#include "image_preprocessor.h"
#include <cmath>
#include <iostream>

cv::Mat ImagePreprocessor::toGrayscale(const cv::Mat& img) {
    if (img.channels() == 1) return img.clone();
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    return gray;
}

cv::Mat ImagePreprocessor::binarize(const cv::Mat& gray, BinarizeMethod method,
                                     int manual_threshold) {
    cv::Mat binary;
    cv::Mat input = gray;
    if (input.channels() > 1) input = toGrayscale(input);

    switch (method) {
        case BinarizeMethod::OTSU:
            cv::threshold(input, binary, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);
            break;
        case BinarizeMethod::ADAPTIVE:
            cv::adaptiveThreshold(input, binary, 255,
                                   cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                                   cv::THRESH_BINARY, 15, 10);
            break;
        case BinarizeMethod::MANUAL:
            cv::threshold(input, binary, manual_threshold, 255, cv::THRESH_BINARY);
            break;
    }
    return binary;
}

cv::Mat ImagePreprocessor::denoise(const cv::Mat& img, int kernel_size) {
    if (kernel_size % 2 == 0) kernel_size++;
    cv::Mat denoised;
    cv::medianBlur(img, denoised, kernel_size);
    return denoised;
}

cv::Mat ImagePreprocessor::enhanceContrast(const cv::Mat& gray, double clip_limit,
                                            cv::Size tile_size) {
    cv::Mat input = gray;
    if (input.channels() > 1) input = toGrayscale(input);
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(clip_limit, tile_size);
    cv::Mat enhanced;
    clahe->apply(input, enhanced);
    return enhanced;
}

cv::Mat ImagePreprocessor::deskew(const cv::Mat& img) {
    cv::Mat gray = img;
    if (gray.channels() > 1) gray = toGrayscale(img);

    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY_INV + cv::THRESH_OTSU);

    std::vector<cv::Point> points;
    cv::findNonZero(binary, points);
    if (points.empty()) return img.clone();

    cv::RotatedRect rect = cv::minAreaRect(points);
    double angle = rect.angle;
    if (angle < -45.0) angle += 90.0;
    if (std::abs(angle) < 0.5 || std::abs(angle) > 10.0) return img.clone();

    std::cout << "[ImagePreprocessor] Corrigiendo inclinacion: " << angle << " grados" << std::endl;

    cv::Point2f center(static_cast<float>(img.cols) / 2.0f,
                       static_cast<float>(img.rows) / 2.0f);
    cv::Mat rot_matrix = cv::getRotationMatrix2D(center, angle, 1.0);
    cv::Mat rotated;
    cv::warpAffine(img, rotated, rot_matrix, img.size(),
                    cv::INTER_CUBIC, cv::BORDER_REPLICATE);
    return rotated;
}

cv::Mat ImagePreprocessor::fullPreprocess(const cv::Mat& img) {
    std::cout << "[ImagePreprocessor] Iniciando preprocesamiento completo..." << std::endl;

    cv::Mat gray = toGrayscale(img);
    std::cout << "  [1/5] Conversion a escala de grises" << std::endl;

    cv::Mat deskewed = deskew(gray);
    std::cout << "  [2/5] Correccion de inclinacion" << std::endl;

    cv::Mat denoised = denoise(deskewed, 3);
    std::cout << "  [3/5] Reduccion de ruido" << std::endl;

    cv::Mat enhanced = enhanceContrast(denoised);
    std::cout << "  [4/5] Mejora de contraste (CLAHE)" << std::endl;

    cv::Mat binary = binarize(enhanced, BinarizeMethod::ADAPTIVE);
    std::cout << "  [5/5] Binarizacion adaptativa" << std::endl;

    return binary;
}
