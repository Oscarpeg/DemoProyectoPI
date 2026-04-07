#ifndef OCR_EXTRACTOR_H
#define OCR_EXTRACTOR_H

#include <string>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>
#include "table_detector.h"

// Resultado individual del OCR para una region de imagen
struct OCRResult {
    std::string text;       // Texto reconocido
    double confidence;      // Nivel de confianza (0.0 a 1.0)
    cv::Rect bbox;          // Bounding box dentro de la imagen procesada
};

// Extractor de texto basado en EasyOCR (via servidor Python persistente)
// El modelo se carga UNA sola vez y se reutiliza para todas las peticiones.
class OCRExtractor {
public:
    OCRExtractor();
    explicit OCRExtractor(const std::string& python_script_path);
    ~OCRExtractor();

    // No copiar (tiene un proceso hijo)
    OCRExtractor(const OCRExtractor&) = delete;
    OCRExtractor& operator=(const OCRExtractor&) = delete;

    OCRResult extractFromCell(const cv::Mat& cell_img);
    std::vector<std::vector<OCRResult>> extractFromGrid(
        const cv::Mat& img, const TableGrid& grid);
    std::vector<OCRResult> extractFromRegion(const cv::Mat& region);
    std::vector<OCRResult> extractAll(const cv::Mat& image);

    bool startServer();
    void stopServer();
    bool isServerRunning() const;

private:
    std::string sendRequest(const std::string& json_request);
    std::vector<OCRResult> requestOCR(const std::string& image_path);
    std::vector<std::vector<OCRResult>> requestBatchOCR(
        const std::vector<std::string>& image_paths);
    std::vector<OCRResult> parseOCRResponse(const std::string& json_str);
    std::string saveTempImage(const cv::Mat& img, const std::string& prefix = "ocr");

    std::string temp_dir_;
    std::string python_script_path_;
    int temp_counter_ = 0;

    FILE* server_stdin_ = nullptr;
    FILE* server_stdout_ = nullptr;
    pid_t server_pid_ = -1;
    bool server_running_ = false;
};

#endif // OCR_EXTRACTOR_H
