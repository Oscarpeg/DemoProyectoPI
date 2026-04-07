#include "page_classifier.h"
#include <algorithm>
#include <iostream>

PageClassifier::PageClassifier(OCRExtractor& ocr) : ocr_(ocr) {}

bool PageClassifier::isLandscape(const cv::Mat& image) {
    return image.cols > image.rows;
}

std::string PageClassifier::pageTypeToString(PageType type) {
    switch (type) {
        case PageType::RESUMEN: return "RESUMEN";
        case PageType::DETALLE_RENTA_FIJA: return "DETALLE_RENTA_FIJA";
        case PageType::EXTRACTO_FONDOS: return "EXTRACTO_FONDOS";
        case PageType::DESCONOCIDO: return "DESCONOCIDO";
    }
    return "DESCONOCIDO";
}

std::string PageClassifier::ocrRegion(const cv::Mat& region) {
    auto results = ocr_.extractFromRegion(region);
    std::string text;
    for (const auto& r : results) {
        if (!text.empty()) text += " ";
        text += r.text;
    }
    // Normalizar a minusculas
    std::transform(text.begin(), text.end(), text.begin(), ::tolower);
    return text;
}

bool PageClassifier::containsKeyword(const cv::Mat& region, const std::string& keyword) {
    std::string text = ocrRegion(region);
    std::string lower_kw = keyword;
    std::transform(lower_kw.begin(), lower_kw.end(), lower_kw.begin(), ::tolower);
    return text.find(lower_kw) != std::string::npos;
}

PageType PageClassifier::classify(const cv::Mat& page_image) {
    std::cout << "[PageClassifier] Clasificando pagina ("
              << page_image.cols << "x" << page_image.rows << ")..." << std::endl;

    // 1. Heuristica principal: Si es landscape -> DETALLE_RENTA_FIJA (Pagina 3)
    if (isLandscape(page_image)) {
        std::cout << "[PageClassifier] -> DETALLE_RENTA_FIJA (landscape detectado)" << std::endl;
        return PageType::DETALLE_RENTA_FIJA;
    }

    // 2. OCR del header (25% superior)
    int header_height = page_image.rows / 4;
    cv::Mat header = page_image(cv::Rect(0, 0, page_image.cols, header_height));
    std::string header_text = ocrRegion(header);

    std::cout << "[PageClassifier] Header OCR: " << header_text.substr(0, 200) << "..." << std::endl;

    // 3. Si contiene "extracto fondos" / "fondo de inversion" -> EXTRACTO_FONDOS (Pagina 4)
    if (header_text.find("extracto") != std::string::npos &&
        header_text.find("fondo") != std::string::npos) {
        std::cout << "[PageClassifier] -> EXTRACTO_FONDOS (keywords en header)" << std::endl;
        return PageType::EXTRACTO_FONDOS;
    }
    if (header_text.find("fondo") != std::string::npos &&
        header_text.find("inversion") != std::string::npos) {
        std::cout << "[PageClassifier] -> EXTRACTO_FONDOS (fondo+inversion en header)" << std::endl;
        return PageType::EXTRACTO_FONDOS;
    }

    // 4. Si contiene "extracto de inversiones", confirmar RESUMEN con OCR del medio
    if (header_text.find("extracto") != std::string::npos &&
        header_text.find("inversiones") != std::string::npos) {
        // Confirmar con OCR de la parte media
        int mid_y = page_image.rows / 4;
        int mid_h = page_image.rows / 2;
        cv::Mat middle = page_image(cv::Rect(0, mid_y, page_image.cols, mid_h));
        std::string mid_text = ocrRegion(middle);

        if (mid_text.find("nombre") != std::string::npos ||
            mid_text.find("nit") != std::string::npos ||
            mid_text.find("total") != std::string::npos ||
            mid_text.find("activos") != std::string::npos) {
            std::cout << "[PageClassifier] -> RESUMEN (extracto inversiones + datos cliente)" << std::endl;
            return PageType::RESUMEN;
        }
    }

    // 5. Si contiene "defensor" / "consumidor" / "superintendencia" -> DESCONOCIDO (Pagina 2)
    if (header_text.find("defensor") != std::string::npos ||
        header_text.find("consumidor") != std::string::npos ||
        header_text.find("superintendencia") != std::string::npos) {
        std::cout << "[PageClassifier] -> DESCONOCIDO (pagina informativa)" << std::endl;
        return PageType::DESCONOCIDO;
    }

    // 6. Fallback: OCR de mitad superior completa
    int upper_h = page_image.rows / 2;
    cv::Mat upper_half = page_image(cv::Rect(0, 0, page_image.cols, upper_h));
    std::string upper_text = ocrRegion(upper_half);

    if (upper_text.find("fondo") != std::string::npos &&
        upper_text.find("inversion") != std::string::npos) {
        std::cout << "[PageClassifier] -> EXTRACTO_FONDOS (fallback)" << std::endl;
        return PageType::EXTRACTO_FONDOS;
    }

    if (upper_text.find("acciones") != std::string::npos &&
        upper_text.find("valores") != std::string::npos) {
        // Podria ser pagina 1 (tiene el logo de Acciones & Valores)
        if (upper_text.find("nombre") != std::string::npos ||
            upper_text.find("portafolio") != std::string::npos ||
            upper_text.find("total") != std::string::npos) {
            std::cout << "[PageClassifier] -> RESUMEN (fallback acciones+valores)" << std::endl;
            return PageType::RESUMEN;
        }
    }

    std::cout << "[PageClassifier] -> DESCONOCIDO (no se pudo clasificar)" << std::endl;
    return PageType::DESCONOCIDO;
}
