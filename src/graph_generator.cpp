#include "graph_generator.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

std::vector<cv::Scalar> GraphGenerator::getPalette() {
    return {
        cv::Scalar(41, 128, 185),   // Azul
        cv::Scalar(39, 174, 96),    // Verde
        cv::Scalar(231, 76, 60),    // Rojo
        cv::Scalar(243, 156, 18),   // Naranja
        cv::Scalar(142, 68, 173),   // Purpura
        cv::Scalar(52, 152, 219),   // Azul claro
        cv::Scalar(46, 204, 113),   // Verde claro
        cv::Scalar(230, 126, 34),   // Naranja oscuro
        cv::Scalar(149, 165, 166),  // Gris
        cv::Scalar(52, 73, 94),     // Azul oscuro
    };
}

std::string GraphGenerator::formatNumber(double value) {
    std::ostringstream oss;
    if (std::abs(value) >= 1e9) {
        oss << std::fixed << std::setprecision(2) << (value / 1e9) << "B";
    } else if (std::abs(value) >= 1e6) {
        oss << std::fixed << std::setprecision(2) << (value / 1e6) << "M";
    } else if (std::abs(value) >= 1e3) {
        oss << std::fixed << std::setprecision(1) << (value / 1e3) << "K";
    } else {
        oss << std::fixed << std::setprecision(2) << value;
    }
    return oss.str();
}

void GraphGenerator::drawTitle(cv::Mat& img, const std::string& title) {
    int baseline = 0;
    cv::Size text_size = cv::getTextSize(title, cv::FONT_HERSHEY_SIMPLEX, 0.8, 2, &baseline);
    int x = (img.cols - text_size.width) / 2;
    cv::putText(img, title, cv::Point(x, 35), cv::FONT_HERSHEY_SIMPLEX,
                0.8, cv::Scalar(30, 30, 30), 2);
}

void GraphGenerator::drawBar(cv::Mat& img, int x, int y, int width, int height,
                              const cv::Scalar& color) {
    cv::rectangle(img, cv::Point(x, y), cv::Point(x + width, y + height), color, cv::FILLED);
    cv::rectangle(img, cv::Point(x, y), cv::Point(x + width, y + height),
                  cv::Scalar(0, 0, 0), 1);
}

void GraphGenerator::drawLabel(cv::Mat& img, const std::string& text,
                                int x, int y, double scale, const cv::Scalar& color) {
    cv::putText(img, text, cv::Point(x, y), cv::FONT_HERSHEY_SIMPLEX,
                scale, color, 1, cv::LINE_AA);
}

// ============================================================
// 1. Composicion del portafolio (barras horizontales)
// ============================================================
std::string GraphGenerator::generateComposicion(
    const std::map<std::string, double>& composicion,
    const std::string& output_path) {

    std::cout << "[GraphGenerator] Generando grafica de composicion..." << std::endl;

    int width = 800, height = 100 + static_cast<int>(composicion.size()) * 60;
    cv::Mat img(height, width, CV_8UC3, cv::Scalar(255, 255, 255));
    drawTitle(img, "Composicion del Portafolio (%)");

    auto palette = getPalette();
    int y = 70;
    int max_bar_width = 500;
    int label_x = 20;
    int bar_x = 200;
    int idx = 0;

    // Encontrar max para escalar
    double max_val = 0;
    for (const auto& [key, val] : composicion) {
        if (val > max_val) max_val = val;
    }
    if (max_val <= 0) max_val = 100;

    for (const auto& [key, val] : composicion) {
        cv::Scalar color = palette[idx % palette.size()];

        // Etiqueta
        drawLabel(img, key, label_x, y + 25, 0.45);

        // Barra
        int bar_width = static_cast<int>((val / max_val) * max_bar_width);
        if (bar_width < 2) bar_width = 2;
        drawBar(img, bar_x, y + 5, bar_width, 30, color);

        // Valor
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << val << "%";
        drawLabel(img, oss.str(), bar_x + bar_width + 10, y + 25, 0.4);

        y += 50;
        idx++;
    }

    cv::imwrite(output_path, img);
    std::cout << "[GraphGenerator] Composicion guardada: " << output_path << std::endl;
    return output_path;
}

// ============================================================
// 2. Distribucion Renta Fija por nemotecnico (barras verticales)
// ============================================================
std::string GraphGenerator::generateDistribucionRF(
    const std::vector<InstrumentoRentaFija>& instrumentos,
    const std::string& output_path) {

    std::cout << "[GraphGenerator] Generando distribucion de renta fija..." << std::endl;

    int n = static_cast<int>(instrumentos.size());
    int width = std::max(600, 120 * n + 100);
    int height = 500;
    cv::Mat img(height, width, CV_8UC3, cv::Scalar(255, 255, 255));
    drawTitle(img, "Distribucion Renta Fija - Valor de Mercado");

    auto palette = getPalette();

    // Encontrar max valor
    double max_val = 0;
    for (const auto& inst : instrumentos) {
        if (inst.valor_mercado > max_val) max_val = inst.valor_mercado;
    }
    if (max_val <= 0) max_val = 1;

    int chart_bottom = height - 80;
    int chart_top = 70;
    int chart_height = chart_bottom - chart_top;
    int bar_width = std::min(80, (width - 100) / std::max(1, n) - 10);
    int start_x = 80;

    // Eje Y
    cv::line(img, cv::Point(start_x - 10, chart_top),
             cv::Point(start_x - 10, chart_bottom), cv::Scalar(100, 100, 100), 1);
    // Eje X
    cv::line(img, cv::Point(start_x - 10, chart_bottom),
             cv::Point(width - 20, chart_bottom), cv::Scalar(100, 100, 100), 1);

    // Lineas de referencia Y
    for (int i = 0; i <= 4; ++i) {
        int y = chart_bottom - (chart_height * i / 4);
        double val = max_val * i / 4.0;
        cv::line(img, cv::Point(start_x - 10, y), cv::Point(width - 20, y),
                 cv::Scalar(220, 220, 220), 1);
        drawLabel(img, formatNumber(val), 5, y + 5, 0.35, cv::Scalar(100, 100, 100));
    }

    for (int i = 0; i < n; ++i) {
        int x = start_x + i * (bar_width + 10);
        int bar_h = static_cast<int>((instrumentos[i].valor_mercado / max_val) * chart_height);
        if (bar_h < 2) bar_h = 2;
        int y = chart_bottom - bar_h;

        cv::Scalar color = palette[i % palette.size()];
        drawBar(img, x, y, bar_width, bar_h, color);

        // Valor sobre la barra
        drawLabel(img, formatNumber(instrumentos[i].valor_mercado),
                  x, y - 5, 0.35);

        // Nemotecnico debajo
        // Rotar texto truncado
        std::string nemo = instrumentos[i].nemotecnico;
        if (nemo.size() > 12) nemo = nemo.substr(0, 12);
        drawLabel(img, nemo, x, chart_bottom + 20, 0.3);
    }

    cv::imwrite(output_path, img);
    std::cout << "[GraphGenerator] Distribucion RF guardada: " << output_path << std::endl;
    return output_path;
}

// ============================================================
// 3. Rentabilidades FIC (barras)
// ============================================================
std::string GraphGenerator::generateRentabilidadesFIC(
    const std::map<std::string, double>& rentabilidades,
    const std::string& output_path) {

    std::cout << "[GraphGenerator] Generando rentabilidades FIC..." << std::endl;

    int n = static_cast<int>(rentabilidades.size());
    int width = std::max(600, 100 * n + 100);
    int height = 450;
    cv::Mat img(height, width, CV_8UC3, cv::Scalar(255, 255, 255));
    drawTitle(img, "Rentabilidades Historicas FIC (%)");

    auto palette = getPalette();

    double max_val = 0, min_val = 0;
    for (const auto& [key, val] : rentabilidades) {
        if (val > max_val) max_val = val;
        if (val < min_val) min_val = val;
    }
    double range = max_val - min_val;
    if (range <= 0) range = 10;

    int chart_bottom = height - 80;
    int chart_top = 70;
    int chart_height = chart_bottom - chart_top;
    int zero_y = chart_bottom;
    if (min_val < 0) {
        zero_y = chart_bottom - static_cast<int>((-min_val / range) * chart_height);
    }

    int bar_width = std::min(70, (width - 100) / std::max(1, n) - 10);
    int start_x = 60;
    int idx = 0;

    // Linea del cero
    cv::line(img, cv::Point(start_x - 10, zero_y),
             cv::Point(width - 20, zero_y), cv::Scalar(0, 0, 0), 1);

    for (const auto& [key, val] : rentabilidades) {
        int x = start_x + idx * (bar_width + 15);
        int bar_h = static_cast<int>((std::abs(val) / range) * chart_height);
        if (bar_h < 2) bar_h = 2;

        cv::Scalar color = palette[idx % palette.size()];

        if (val >= 0) {
            int y = zero_y - bar_h;
            drawBar(img, x, y, bar_width, bar_h, color);
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << val << "%";
            drawLabel(img, oss.str(), x, y - 5, 0.35);
        } else {
            drawBar(img, x, zero_y, bar_width, bar_h, color);
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << val << "%";
            drawLabel(img, oss.str(), x, zero_y + bar_h + 15, 0.35);
        }

        // Etiqueta
        std::string label = key;
        if (label.size() > 12) label = label.substr(0, 12);
        drawLabel(img, label, x, chart_bottom + 20, 0.3);

        idx++;
    }

    cv::imwrite(output_path, img);
    std::cout << "[GraphGenerator] Rentabilidades FIC guardada: " << output_path << std::endl;
    return output_path;
}

// ============================================================
// 4. Evolucion saldo (linea con area rellena)
// ============================================================
std::string GraphGenerator::generateEvolucionSaldo(
    const std::vector<std::map<std::string, double>>& serie,
    const std::string& output_path) {

    std::cout << "[GraphGenerator] Generando evolucion del saldo..." << std::endl;

    int width = 800, height = 450;
    cv::Mat img(height, width, CV_8UC3, cv::Scalar(255, 255, 255));
    drawTitle(img, "Evolucion del Portafolio");

    if (serie.empty()) {
        drawLabel(img, "Sin datos historicos", width / 2 - 80, height / 2, 0.6);
        cv::imwrite(output_path, img);
        return output_path;
    }

    // Extraer valores de total_portafolio
    std::vector<double> values;
    for (const auto& point : serie) {
        auto it = point.find("total_portafolio");
        if (it != point.end()) {
            values.push_back(it->second);
        }
    }

    if (values.empty()) {
        drawLabel(img, "Sin datos de portafolio", width / 2 - 100, height / 2, 0.6);
        cv::imwrite(output_path, img);
        return output_path;
    }

    double min_v = *std::min_element(values.begin(), values.end());
    double max_v = *std::max_element(values.begin(), values.end());
    double range = max_v - min_v;
    if (range < 1) range = max_v * 0.1;
    min_v -= range * 0.1;
    max_v += range * 0.1;
    range = max_v - min_v;

    int chart_left = 100, chart_right = width - 40;
    int chart_top = 70, chart_bottom = height - 60;
    int chart_w = chart_right - chart_left;
    int chart_h = chart_bottom - chart_top;

    // Ejes
    cv::line(img, cv::Point(chart_left, chart_top),
             cv::Point(chart_left, chart_bottom), cv::Scalar(100, 100, 100), 1);
    cv::line(img, cv::Point(chart_left, chart_bottom),
             cv::Point(chart_right, chart_bottom), cv::Scalar(100, 100, 100), 1);

    // Lineas de referencia Y
    for (int i = 0; i <= 4; ++i) {
        int y = chart_bottom - (chart_h * i / 4);
        double val = min_v + range * i / 4.0;
        cv::line(img, cv::Point(chart_left, y), cv::Point(chart_right, y),
                 cv::Scalar(230, 230, 230), 1);
        drawLabel(img, formatNumber(val), 5, y + 5, 0.35, cv::Scalar(100, 100, 100));
    }

    // Puntos y area
    std::vector<cv::Point> points;
    int n = static_cast<int>(values.size());
    for (int i = 0; i < n; ++i) {
        int x = chart_left + (i * chart_w) / std::max(1, n - 1);
        int y = chart_bottom - static_cast<int>(((values[i] - min_v) / range) * chart_h);
        points.emplace_back(x, y);
    }

    // Area rellena
    std::vector<cv::Point> area_points = points;
    area_points.emplace_back(points.back().x, chart_bottom);
    area_points.emplace_back(points.front().x, chart_bottom);
    cv::fillPoly(img, std::vector<std::vector<cv::Point>>{area_points},
                 cv::Scalar(200, 220, 240));

    // Linea
    for (size_t i = 1; i < points.size(); ++i) {
        cv::line(img, points[i - 1], points[i], cv::Scalar(41, 128, 185), 2, cv::LINE_AA);
    }

    // Puntos
    for (const auto& pt : points) {
        cv::circle(img, pt, 4, cv::Scalar(41, 128, 185), cv::FILLED, cv::LINE_AA);
        cv::circle(img, pt, 4, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
    }

    // Etiquetas X
    for (int i = 0; i < n; ++i) {
        int x = chart_left + (i * chart_w) / std::max(1, n - 1);
        drawLabel(img, std::to_string(i + 1), x - 3, chart_bottom + 20, 0.35);
    }

    cv::imwrite(output_path, img);
    std::cout << "[GraphGenerator] Evolucion guardada: " << output_path << std::endl;
    return output_path;
}

// ============================================================
// 5. Tabla de estadisticas (renderizada como imagen)
// ============================================================
std::string GraphGenerator::generateTablaEstadisticas(
    const AnalysisResult& analysis,
    const std::string& output_path) {

    std::cout << "[GraphGenerator] Generando tabla de estadisticas..." << std::endl;

    int width = 750, row_h = 30;
    int num_rows = 7; // header + 4 tasas + 2 spacer
    int height = 80 + num_rows * row_h + static_cast<int>(analysis.alertas.size()) * 25 + 60;
    cv::Mat img(height, width, CV_8UC3, cv::Scalar(255, 255, 255));
    drawTitle(img, "Resumen Estadistico");

    int y = 60;
    int cols[] = {20, 120, 220, 320, 420, 520, 620};

    // Header
    cv::rectangle(img, cv::Point(10, y), cv::Point(width - 10, y + row_h),
                  cv::Scalar(41, 128, 185), cv::FILLED);
    std::vector<std::string> headers = {"Metrica", "Media", "Desv.Est", "Mediana", "Min", "Max", "N"};
    for (int i = 0; i < 7; ++i) {
        drawLabel(img, headers[i], cols[i], y + 20, 0.4, cv::Scalar(255, 255, 255));
    }
    y += row_h;

    auto drawStatRow = [&](const std::string& name, const StatSummary& s, const cv::Scalar& bg) {
        cv::rectangle(img, cv::Point(10, y), cv::Point(width - 10, y + row_h), bg, cv::FILLED);
        drawLabel(img, name, cols[0], y + 20, 0.38);

        auto fmt = [](double v) -> std::string {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << v;
            return oss.str();
        };

        drawLabel(img, fmt(s.mean), cols[1], y + 20, 0.38);
        drawLabel(img, fmt(s.std_dev), cols[2], y + 20, 0.38);
        drawLabel(img, fmt(s.median), cols[3], y + 20, 0.38);
        drawLabel(img, fmt(s.min_val), cols[4], y + 20, 0.38);
        drawLabel(img, fmt(s.max_val), cols[5], y + 20, 0.38);
        drawLabel(img, std::to_string(s.count), cols[6], y + 20, 0.38);
        y += row_h;
    };

    cv::Scalar row1(245, 245, 245);
    cv::Scalar row2(255, 255, 255);

    drawStatRow("Tasa Valor.", analysis.tasas_valoracion, row1);
    drawStatRow("Tasa Negoc.", analysis.tasas_negociacion, row2);
    drawStatRow("Tasa Facial", analysis.tasas_faciales, row1);
    drawStatRow("Val. Mercado", analysis.valores_mercado, row2);

    // Alertas
    y += 20;
    drawLabel(img, "Alertas:", 20, y + 15, 0.5, cv::Scalar(231, 76, 60));
    y += 25;

    for (const auto& alerta : analysis.alertas) {
        cv::Scalar color;
        if (alerta.type == "CRITICAL") color = cv::Scalar(0, 0, 200);
        else if (alerta.type == "WARNING") color = cv::Scalar(0, 140, 230);
        else color = cv::Scalar(100, 100, 100);

        std::string text = "[" + alerta.type + "] " + alerta.message;
        if (text.size() > 90) text = text.substr(0, 90) + "...";
        drawLabel(img, text, 30, y + 15, 0.35, color);
        y += 22;
    }

    cv::imwrite(output_path, img);
    std::cout << "[GraphGenerator] Tabla estadisticas guardada: " << output_path << std::endl;
    return output_path;
}

// ============================================================
// Generar todas las graficas
// ============================================================
std::vector<std::string> GraphGenerator::generateAll(
    const ExtractoCompleto& extracto,
    const AnalysisResult& analysis,
    const std::string& output_dir) {

    std::cout << "[GraphGenerator] Generando todas las graficas en: " << output_dir << std::endl;
    std::vector<std::string> paths;

    // 1. Composicion
    if (!analysis.composicion_porcentaje.empty()) {
        paths.push_back(generateComposicion(
            analysis.composicion_porcentaje,
            output_dir + "/01_composicion_portafolio.png"));
    }

    // 2. Distribucion RF
    if (!extracto.renta_fija.empty()) {
        paths.push_back(generateDistribucionRF(
            extracto.renta_fija,
            output_dir + "/02_distribucion_renta_fija.png"));
    }

    // 3. Rentabilidades FIC
    if (!analysis.rendimientos_fic.empty()) {
        paths.push_back(generateRentabilidadesFIC(
            analysis.rendimientos_fic,
            output_dir + "/03_rentabilidades_fic.png"));
    }

    // 4. Evolucion saldo
    if (!analysis.serie_temporal.empty()) {
        paths.push_back(generateEvolucionSaldo(
            analysis.serie_temporal,
            output_dir + "/04_evolucion_saldo.png"));
    }

    // 5. Tabla estadisticas
    paths.push_back(generateTablaEstadisticas(
        analysis, output_dir + "/05_tabla_estadisticas.png"));

    std::cout << "[GraphGenerator] " << paths.size() << " graficas generadas" << std::endl;
    return paths;
}
