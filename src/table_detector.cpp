#include "table_detector.h"
#include <algorithm>
#include <iostream>
#include <cmath>

cv::Mat TableDetector::detectHorizontalLines(const cv::Mat& binary, int min_length) {
    cv::Mat inverted;
    cv::bitwise_not(binary, inverted);
    if (min_length <= 0) min_length = binary.cols / 4;

    cv::Mat h_kernel = cv::getStructuringElement(
        cv::MORPH_RECT, cv::Size(min_length, 1));
    cv::Mat h_lines;
    cv::morphologyEx(inverted, h_lines, cv::MORPH_OPEN, h_kernel);
    return h_lines;
}

cv::Mat TableDetector::detectVerticalLines(const cv::Mat& binary, int min_length) {
    cv::Mat inverted;
    cv::bitwise_not(binary, inverted);
    if (min_length <= 0) min_length = binary.rows / 6;

    cv::Mat v_kernel = cv::getStructuringElement(
        cv::MORPH_RECT, cv::Size(1, min_length));
    cv::Mat v_lines;
    cv::morphologyEx(inverted, v_lines, cv::MORPH_OPEN, v_kernel);
    return v_lines;
}

std::vector<cv::Point> TableDetector::findIntersections(
    const cv::Mat& h_lines, const cv::Mat& v_lines) {

    cv::Mat intersections;
    cv::bitwise_and(h_lines, v_lines, intersections);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::dilate(intersections, intersections, kernel);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(intersections, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<cv::Point> points;
    for (const auto& contour : contours) {
        cv::Moments m = cv::moments(contour);
        if (m.m00 > 0) {
            int cx = static_cast<int>(m.m10 / m.m00);
            int cy = static_cast<int>(m.m01 / m.m00);
            points.emplace_back(cx, cy);
        }
    }
    return points;
}

std::vector<int> TableDetector::clusterPositions(
    std::vector<int>& positions, int threshold) {
    if (positions.empty()) return {};
    std::sort(positions.begin(), positions.end());
    std::vector<int> clustered;
    int current_sum = positions[0];
    int current_count = 1;

    for (size_t i = 1; i < positions.size(); ++i) {
        if (positions[i] - positions[i - 1] <= threshold) {
            current_sum += positions[i];
            current_count++;
        } else {
            clustered.push_back(current_sum / current_count);
            current_sum = positions[i];
            current_count = 1;
        }
    }
    clustered.push_back(current_sum / current_count);
    return clustered;
}

TableGrid TableDetector::buildGrid(const cv::Mat& binary) {
    std::cout << "[TableDetector] Detectando estructura de tabla..." << std::endl;
    TableGrid grid;

    cv::Mat h_lines = detectHorizontalLines(binary);
    cv::Mat v_lines = detectVerticalLines(binary);
    std::vector<cv::Point> intersections = findIntersections(h_lines, v_lines);

    if (intersections.size() < 4) {
        std::cout << "[TableDetector] Insuficientes intersecciones ("
                  << intersections.size() << "). No se detecto tabla." << std::endl;
        return grid;
    }

    std::cout << "[TableDetector] " << intersections.size()
              << " intersecciones encontradas" << std::endl;

    std::vector<int> x_positions, y_positions;
    for (const auto& pt : intersections) {
        x_positions.push_back(pt.x);
        y_positions.push_back(pt.y);
    }

    grid.col_positions = clusterPositions(x_positions, 15);
    grid.row_positions = clusterPositions(y_positions, 10);

    grid.rows = static_cast<int>(grid.row_positions.size()) - 1;
    grid.cols = static_cast<int>(grid.col_positions.size()) - 1;

    std::cout << "[TableDetector] Grid: " << grid.rows
              << " filas x " << grid.cols << " columnas" << std::endl;

    if (!grid.col_positions.empty() && !grid.row_positions.empty()) {
        int x = grid.col_positions.front();
        int y = grid.row_positions.front();
        int w = grid.col_positions.back() - x;
        int h = grid.row_positions.back() - y;
        grid.bounds = cv::Rect(x, y, w, h);
    }

    grid.cells.clear();
    for (size_t r = 0; r + 1 < grid.row_positions.size(); ++r) {
        std::vector<cv::Rect> row_cells;
        for (size_t c = 0; c + 1 < grid.col_positions.size(); ++c) {
            int x = grid.col_positions[c];
            int y = grid.row_positions[r];
            int w = grid.col_positions[c + 1] - x;
            int h = grid.row_positions[r + 1] - y;
            int margin = 3;
            x += margin; y += margin; w -= 2 * margin; h -= 2 * margin;
            if (w > 0 && h > 0) row_cells.emplace_back(x, y, w, h);
        }
        if (!row_cells.empty()) grid.cells.push_back(row_cells);
    }
    return grid;
}

std::vector<cv::Mat> TableDetector::extractCells(
    const cv::Mat& img, const TableGrid& grid) {
    std::vector<cv::Mat> cells;
    for (const auto& row : grid.cells) {
        for (const auto& cell_rect : row) {
            cv::Rect safe_rect = cell_rect & cv::Rect(0, 0, img.cols, img.rows);
            if (safe_rect.width > 0 && safe_rect.height > 0)
                cells.push_back(img(safe_rect).clone());
        }
    }
    return cells;
}

std::vector<TableGrid> TableDetector::detectAllTables(const cv::Mat& input) {
    std::cout << "[TableDetector] Detectando todas las tablas en la pagina..." << std::endl;
    std::vector<TableGrid> tables;

    // Re-binarizar con OTSU desde la imagen de entrada para preservar lineas de tabla
    // La binarizacion adaptativa (usada para OCR) puede destruir lineas finas
    cv::Mat binary;
    if (input.channels() > 1) {
        cv::Mat gray;
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
        cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);
    } else {
        // Si ya es 1 canal, verificar si parece binarizada (solo 0 y 255)
        // Si no, re-binarizar con OTSU
        double minVal, maxVal;
        cv::minMaxLoc(input, &minVal, &maxVal);
        if (minVal == 0 && maxVal == 255) {
            binary = input.clone();
        } else {
            cv::threshold(input, binary, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);
        }
    }

    // Usar min_length mas cortos para detectar tablas que no ocupan todo el ancho
    int h_min = binary.cols / 8;  // antes era cols/4, muy largo para tablas parciales
    int v_min = binary.rows / 10; // antes era rows/6, muy largo para tablas cortas

    std::cout << "[TableDetector] Imagen: " << binary.cols << "x" << binary.rows
              << ", min_h=" << h_min << ", min_v=" << v_min << std::endl;

    cv::Mat h_lines = detectHorizontalLines(binary, h_min);
    cv::Mat v_lines = detectVerticalLines(binary, v_min);

    cv::Mat all_lines;
    cv::bitwise_or(h_lines, v_lines, all_lines);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    cv::dilate(all_lines, all_lines, kernel, cv::Point(-1, -1), 3);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(all_lines, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (const auto& contour : contours) {
        cv::Rect bounds = cv::boundingRect(contour);
        if (bounds.width < binary.cols / 4 || bounds.height < 50) continue;

        cv::Rect safe_bounds = bounds & cv::Rect(0, 0, binary.cols, binary.rows);
        cv::Mat table_region = binary(safe_bounds);

        TableGrid grid = buildGrid(table_region);

        if (!grid.cells.empty()) {
            grid.bounds.x += bounds.x;
            grid.bounds.y += bounds.y;
            for (auto& row : grid.cells) {
                for (auto& cell : row) {
                    cell.x += bounds.x;
                    cell.y += bounds.y;
                }
            }
            for (auto& pos : grid.row_positions) pos += bounds.y;
            for (auto& pos : grid.col_positions) pos += bounds.x;
            tables.push_back(grid);
        }
    }

    std::cout << "[TableDetector] " << tables.size() << " tablas detectadas" << std::endl;
    return tables;
}
