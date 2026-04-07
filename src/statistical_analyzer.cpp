#include "statistical_analyzer.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <iostream>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

double StatisticalAnalyzer::mean(const std::vector<double>& data) {
    if (data.empty()) return 0.0;
    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    return sum / static_cast<double>(data.size());
}

double StatisticalAnalyzer::stdDev(const std::vector<double>& data) {
    if (data.size() < 2) return 0.0;
    double m = mean(data);
    double sum_sq = 0.0;
    for (double v : data) {
        sum_sq += (v - m) * (v - m);
    }
    return std::sqrt(sum_sq / static_cast<double>(data.size() - 1));
}

double StatisticalAnalyzer::median(std::vector<double> data) {
    if (data.empty()) return 0.0;
    std::sort(data.begin(), data.end());
    size_t n = data.size();
    if (n % 2 == 0) {
        return (data[n / 2 - 1] + data[n / 2]) / 2.0;
    }
    return data[n / 2];
}

double StatisticalAnalyzer::minVal(const std::vector<double>& data) {
    if (data.empty()) return 0.0;
    return *std::min_element(data.begin(), data.end());
}

double StatisticalAnalyzer::maxVal(const std::vector<double>& data) {
    if (data.empty()) return 0.0;
    return *std::max_element(data.begin(), data.end());
}

StatSummary StatisticalAnalyzer::summarize(const std::vector<double>& data) {
    StatSummary s;
    s.count = static_cast<int>(data.size());
    if (data.empty()) return s;
    s.mean = mean(data);
    s.std_dev = stdDev(data);
    s.median = median(data);
    s.min_val = minVal(data);
    s.max_val = maxVal(data);
    return s;
}

std::vector<int> StatisticalAnalyzer::findOutliers(const std::vector<double>& data, double k) {
    std::vector<int> outliers;
    if (data.size() < 3) return outliers;

    double m = mean(data);
    double sd = stdDev(data);
    if (sd < 1e-9) return outliers;

    for (size_t i = 0; i < data.size(); ++i) {
        if (std::abs(data[i] - m) > k * sd) {
            outliers.push_back(static_cast<int>(i));
        }
    }
    return outliers;
}

AnalysisResult StatisticalAnalyzer::analyze(const ExtractoCompleto& extracto) {
    std::cout << "[StatisticalAnalyzer] Analizando extracto..." << std::endl;
    AnalysisResult result;

    // ---- Composicion porcentual del portafolio ----
    double total = extracto.resumen.total_portafolio;
    if (total > 0) {
        for (const auto& [key, val] : extracto.resumen.activos) {
            result.composicion_porcentaje[key] = (val / total) * 100.0;
        }
    }

    // ---- Estadisticas de Renta Fija ----
    std::vector<double> tasas_val, tasas_neg, tasas_fac, valores_merc;
    for (const auto& inst : extracto.renta_fija) {
        if (inst.tasa_valoracion > 0) tasas_val.push_back(inst.tasa_valoracion);
        if (inst.tasa_negociacion > 0) tasas_neg.push_back(inst.tasa_negociacion);
        if (inst.tasa_facial > 0) tasas_fac.push_back(inst.tasa_facial);
        if (inst.valor_mercado > 0) valores_merc.push_back(inst.valor_mercado);
    }

    result.tasas_valoracion = summarize(tasas_val);
    result.tasas_negociacion = summarize(tasas_neg);
    result.tasas_faciales = summarize(tasas_fac);
    result.valores_mercado = summarize(valores_merc);

    // ---- Rendimientos FIC ----
    for (const auto& fondo : extracto.fondos) {
        for (const auto& [periodo, valor] : fondo.rentabilidades_historicas) {
            result.rendimientos_fic[periodo] = valor;
        }
    }

    // ---- Alertas ----
    // Outliers en tasas de valoracion
    auto outlier_idx = findOutliers(tasas_val);
    for (int idx : outlier_idx) {
        if (idx < static_cast<int>(extracto.renta_fija.size())) {
            result.alertas.push_back({
                "WARNING",
                "Tasa de valoracion atipica en " + extracto.renta_fija[idx].nemotecnico +
                ": " + std::to_string(extracto.renta_fija[idx].tasa_valoracion) + "%"
            });
        }
    }

    // Concentracion excesiva
    for (const auto& [key, pct] : result.composicion_porcentaje) {
        if (pct > 80.0) {
            result.alertas.push_back({
                "WARNING",
                "Alta concentracion en " + key + ": " + std::to_string(pct) + "%"
            });
        }
    }

    // Verificar si FIC tiene rendimiento negativo
    for (const auto& fondo : extracto.fondos) {
        if (fondo.rendimientos < 0) {
            result.alertas.push_back({
                "CRITICAL",
                "Rendimiento negativo en fondo: " + fondo.nombre_fondo
            });
        }
    }

    // Instrumentos con tasa facial muy baja
    for (const auto& inst : extracto.renta_fija) {
        if (inst.tasa_facial > 0 && inst.tasa_facial < 5.0) {
            result.alertas.push_back({
                "INFO",
                "Tasa facial baja en " + inst.nemotecnico +
                ": " + std::to_string(inst.tasa_facial) + "%"
            });
        }
    }

    std::cout << "[StatisticalAnalyzer] Analisis completado. "
              << result.alertas.size() << " alertas generadas." << std::endl;

    return result;
}

AnalysisResult StatisticalAnalyzer::analyzeTimeSeries(
    const std::vector<ExtractoCompleto>& extractos) {

    std::cout << "[StatisticalAnalyzer] Analizando serie temporal de "
              << extractos.size() << " extractos..." << std::endl;

    AnalysisResult result;
    if (extractos.empty()) return result;

    // Analizar el ultimo extracto como base
    result = analyze(extractos.back());

    // Construir serie temporal
    for (const auto& ext : extractos) {
        std::map<std::string, double> point;
        point["total_portafolio"] = ext.resumen.total_portafolio;
        point["total_activos"] = ext.resumen.total_activos;

        for (const auto& [key, val] : ext.resumen.activos) {
            point[key] = val;
        }

        result.serie_temporal.push_back(point);
    }

    // Tendencia del portafolio
    if (extractos.size() >= 2) {
        double first = extractos.front().resumen.total_portafolio;
        double last = extractos.back().resumen.total_portafolio;
        if (first > 0) {
            double growth = ((last - first) / first) * 100.0;
            result.alertas.push_back({
                "INFO",
                "Crecimiento del portafolio: " + std::to_string(growth) +
                "% en " + std::to_string(extractos.size()) + " periodos"
            });
        }
    }

    return result;
}

std::vector<std::map<std::string, std::string>> StatisticalAnalyzer::loadHistoricalData(
    const std::string& csv_path) {

    std::cout << "[StatisticalAnalyzer] Cargando datos historicos de: " << csv_path << std::endl;
    std::vector<std::map<std::string, std::string>> records;

    std::ifstream file(csv_path);
    if (!file.is_open()) {
        std::cerr << "[StatisticalAnalyzer] No se pudo abrir: " << csv_path << std::endl;
        return records;
    }

    std::string line;
    std::vector<std::string> headers;

    // Leer headers
    if (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string header;
        while (std::getline(iss, header, ',')) {
            // Trim
            while (!header.empty() && (header.front() == ' ' || header.front() == '\t'))
                header.erase(header.begin());
            while (!header.empty() && (header.back() == ' ' || header.back() == '\t' ||
                                        header.back() == '\r' || header.back() == '\n'))
                header.pop_back();
            headers.push_back(header);
        }
    }

    // Leer datos
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        // Eliminar \r si existe
        if (!line.empty() && line.back() == '\r') line.pop_back();

        std::istringstream iss(line);
        std::string field;
        std::map<std::string, std::string> record;
        size_t col = 0;

        while (std::getline(iss, field, ',') && col < headers.size()) {
            // Trim
            while (!field.empty() && field.front() == ' ') field.erase(field.begin());
            while (!field.empty() && field.back() == ' ') field.pop_back();
            record[headers[col]] = field;
            col++;
        }

        if (!record.empty()) {
            records.push_back(record);
        }
    }

    std::cout << "[StatisticalAnalyzer] " << records.size()
              << " registros historicos cargados" << std::endl;

    return records;
}

json AnalysisResult::to_json() const {
    json j;
    j["composicion_porcentaje"] = composicion_porcentaje;

    auto stat_to_json = [](const StatSummary& s) -> json {
        return {
            {"mean", s.mean}, {"std_dev", s.std_dev}, {"median", s.median},
            {"min", s.min_val}, {"max", s.max_val}, {"count", s.count}
        };
    };

    j["tasas_valoracion"] = stat_to_json(tasas_valoracion);
    j["tasas_negociacion"] = stat_to_json(tasas_negociacion);
    j["tasas_faciales"] = stat_to_json(tasas_faciales);
    j["valores_mercado"] = stat_to_json(valores_mercado);
    j["rendimientos_fic"] = rendimientos_fic;

    json alertas_arr = json::array();
    for (const auto& a : alertas) {
        alertas_arr.push_back({{"type", a.type}, {"message", a.message}});
    }
    j["alertas"] = alertas_arr;

    return j;
}
