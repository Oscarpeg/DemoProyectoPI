#ifndef STATISTICAL_ANALYZER_H
#define STATISTICAL_ANALYZER_H

#include <string>
#include <vector>
#include <map>
#include "data_structurer.h"

struct StatSummary {
    double mean = 0.0;
    double std_dev = 0.0;
    double median = 0.0;
    double min_val = 0.0;
    double max_val = 0.0;
    int count = 0;
};

struct Alert {
    std::string type;       // "WARNING", "INFO", "CRITICAL"
    std::string message;
};

struct AnalysisResult {
    // Composicion del portafolio (%)
    std::map<std::string, double> composicion_porcentaje;

    // Estadisticas de tasas de renta fija
    StatSummary tasas_valoracion;
    StatSummary tasas_negociacion;
    StatSummary tasas_faciales;
    StatSummary valores_mercado;

    // Rendimiento FIC
    std::map<std::string, double> rendimientos_fic;

    // Alertas
    std::vector<Alert> alertas;

    // Serie temporal
    std::vector<std::map<std::string, double>> serie_temporal;

    nlohmann::json to_json() const;
};

class StatisticalAnalyzer {
public:
    static double mean(const std::vector<double>& data);
    static double stdDev(const std::vector<double>& data);
    static double median(std::vector<double> data);
    static double minVal(const std::vector<double>& data);
    static double maxVal(const std::vector<double>& data);
    static StatSummary summarize(const std::vector<double>& data);
    static std::vector<int> findOutliers(const std::vector<double>& data, double k = 2.0);

    AnalysisResult analyze(const ExtractoCompleto& extracto);
    AnalysisResult analyzeTimeSeries(const std::vector<ExtractoCompleto>& extractos);

    static std::vector<std::map<std::string, std::string>> loadHistoricalData(
        const std::string& csv_path);
};

#endif // STATISTICAL_ANALYZER_H
