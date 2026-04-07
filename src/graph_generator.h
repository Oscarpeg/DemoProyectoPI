#ifndef GRAPH_GENERATOR_H
#define GRAPH_GENERATOR_H

#include <string>
#include <vector>
#include <map>
#include <opencv2/opencv.hpp>
#include "data_structurer.h"
#include "statistical_analyzer.h"

class GraphGenerator {
public:
    // Genera todas las graficas y retorna las rutas de los PNGs
    std::vector<std::string> generateAll(
        const ExtractoCompleto& extracto,
        const AnalysisResult& analysis,
        const std::string& output_dir);

    // Graficas individuales
    std::string generateComposicion(
        const std::map<std::string, double>& composicion,
        const std::string& output_path);

    std::string generateDistribucionRF(
        const std::vector<InstrumentoRentaFija>& instrumentos,
        const std::string& output_path);

    std::string generateRentabilidadesFIC(
        const std::map<std::string, double>& rentabilidades,
        const std::string& output_path);

    std::string generateEvolucionSaldo(
        const std::vector<std::map<std::string, double>>& serie,
        const std::string& output_path);

    std::string generateTablaEstadisticas(
        const AnalysisResult& analysis,
        const std::string& output_path);

private:
    // Utilidades de dibujo
    static std::string formatNumber(double value);
    static void drawTitle(cv::Mat& img, const std::string& title);
    static void drawBar(cv::Mat& img, int x, int y, int width, int height,
                        const cv::Scalar& color);
    static void drawLabel(cv::Mat& img, const std::string& text,
                          int x, int y, double scale = 0.5,
                          const cv::Scalar& color = cv::Scalar(0, 0, 0));

    // Paleta de colores
    static std::vector<cv::Scalar> getPalette();
};

#endif // GRAPH_GENERATOR_H
