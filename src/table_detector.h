#ifndef TABLE_DETECTOR_H
#define TABLE_DETECTOR_H

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

struct TableGrid {
    cv::Rect bounds;
    std::vector<int> row_positions;
    std::vector<int> col_positions;
    std::vector<std::vector<cv::Rect>> cells;
    int rows = 0;
    int cols = 0;
};

// Resultado de una detección AI. Incluye la región, la clase y la confianza.
// class_id sigue el mapeo del dataset:
//   0=tabla_renta      1=tabla_portafolio  2=titulo_renta
//   3=tabla_fondo      4=titulo_fondo      5=titulo_portafolio
//   6=titulo_saldos    7=tabla_saldos      8=tabla_fondo_vista
//   9=titulo_fondo_vista  10=titulo_fondo_sumar  11=tabla_fondo_sumar
struct AIDetection {
    cv::Rect rect;
    int      class_id;
    float    confidence;
};

class TableDetector {
public:
    // ── Detección morfológica (método original) ──────────────────────────
    cv::Mat detectHorizontalLines(const cv::Mat& binary, int min_length = 0);
    cv::Mat detectVerticalLines(const cv::Mat& binary, int min_length = 0);
    std::vector<cv::Point> findIntersections(const cv::Mat& h_lines, const cv::Mat& v_lines);
    TableGrid buildGrid(const cv::Mat& binary);
    std::vector<cv::Mat> extractCells(const cv::Mat& img, const TableGrid& grid);
    std::vector<TableGrid> detectAllTables(const cv::Mat& input);

    // ── Detección con modelo YOLOv8n ONNX (método nuevo) ─────────────────
    // Devuelve todas las detecciones con su class_id. El caller filtra
    // por class_id segun el tipo de pagina que esta procesando.
    // Devuelve vector vacío si el archivo no existe o si el modelo falla.
    std::vector<AIDetection> detectWithAI(const cv::Mat& input,
                                           const std::string& model_path,
                                           float conf_threshold = 0.4f);

private:
    std::vector<int> clusterPositions(std::vector<int>& positions, int threshold = 10);
};

#endif // TABLE_DETECTOR_H
