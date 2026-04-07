#ifndef PAGE_CLASSIFIER_H
#define PAGE_CLASSIFIER_H

#include <string>
#include <opencv2/opencv.hpp>
#include "ocr_extractor.h"

// Estructura REAL del PDF de Acciones & Valores:
//   Pagina 1 (portrait): Resumen del portafolio
//   Pagina 2 (portrait): Defensor del consumidor (se omite)
//   Pagina 3 (LANDSCAPE): Tablas bordeadas (Saldos, Renta Fija, FIC)
//   Pagina 4 (portrait): Extracto de Fondos
enum class PageType {
    RESUMEN,
    DETALLE_RENTA_FIJA,
    EXTRACTO_FONDOS,
    DESCONOCIDO
};

class PageClassifier {
public:
    explicit PageClassifier(OCRExtractor& ocr);
    PageType classify(const cv::Mat& page_image);
    bool containsKeyword(const cv::Mat& region, const std::string& keyword);
    static std::string pageTypeToString(PageType type);

private:
    OCRExtractor& ocr_;
    std::string ocrRegion(const cv::Mat& region);
    static bool isLandscape(const cv::Mat& image);
};

#endif // PAGE_CLASSIFIER_H
