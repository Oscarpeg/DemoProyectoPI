#include "ocr_extractor.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <cstring>
#include <csignal>

// Para pipes bidireccionales (POSIX/Linux)
#include <unistd.h>
#include <sys/wait.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

OCRExtractor::OCRExtractor()
    : temp_dir_("output/images/temp_ocr"),
      python_script_path_("python/ocr_helper.py") {
    fs::create_directories(temp_dir_);
}

OCRExtractor::OCRExtractor(const std::string& python_script_path)
    : temp_dir_("output/images/temp_ocr"),
      python_script_path_(python_script_path) {
    fs::create_directories(temp_dir_);
}

OCRExtractor::~OCRExtractor() {
    stopServer();
}

bool OCRExtractor::startServer() {
    if (server_running_) return true;

    std::cout << "[OCRExtractor] Iniciando servidor OCR persistente..." << std::endl;
    std::cout << "[OCRExtractor] (El modelo se carga una sola vez, por favor espere)" << std::endl;

    int pipe_to_child[2];
    int pipe_from_child[2];

    if (pipe(pipe_to_child) < 0 || pipe(pipe_from_child) < 0) {
        std::cerr << "[OCRExtractor] Error creando pipes" << std::endl;
        return false;
    }

    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << "[OCRExtractor] Error en fork()" << std::endl;
        return false;
    }

    if (pid == 0) {
        // === Proceso hijo: ejecutar Python ===
        close(pipe_to_child[1]);
        close(pipe_from_child[0]);

        dup2(pipe_to_child[0], STDIN_FILENO);
        close(pipe_to_child[0]);

        dup2(pipe_from_child[1], STDOUT_FILENO);
        close(pipe_from_child[1]);

        execlp("python3", "python3", python_script_path_.c_str(), "--server", nullptr);
        execlp("python", "python", python_script_path_.c_str(), "--server", nullptr);

        std::cerr << "[OCRExtractor] Error: no se pudo ejecutar Python" << std::endl;
        _exit(1);
    }

    // === Proceso padre ===
    close(pipe_to_child[0]);
    close(pipe_from_child[1]);

    server_pid_ = pid;
    server_stdin_ = fdopen(pipe_to_child[1], "w");
    server_stdout_ = fdopen(pipe_from_child[0], "r");

    if (!server_stdin_ || !server_stdout_) {
        std::cerr << "[OCRExtractor] Error abriendo file descriptors" << std::endl;
        stopServer();
        return false;
    }

    // Esperar mensaje "ready" del servidor
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), server_stdout_) != nullptr) {
        std::string response(buffer);
        try {
            auto j = json::parse(response);
            if (j.contains("status") && j["status"] == "ready") {
                server_running_ = true;
                std::cout << "[OCRExtractor] Servidor OCR listo!" << std::endl;
                return true;
            }
        } catch (...) {
            std::cerr << "[OCRExtractor] Respuesta inesperada: " << response << std::endl;
        }
    }

    std::cerr << "[OCRExtractor] El servidor no respondio correctamente" << std::endl;
    stopServer();
    return false;
}

void OCRExtractor::stopServer() {
    if (server_stdin_) {
        fprintf(server_stdin_, "EXIT\n");
        fflush(server_stdin_);
        fclose(server_stdin_);
        server_stdin_ = nullptr;
    }
    if (server_stdout_) {
        fclose(server_stdout_);
        server_stdout_ = nullptr;
    }
    if (server_pid_ > 0) {
        int status;
        waitpid(server_pid_, &status, WNOHANG);
        kill(server_pid_, SIGTERM);
        waitpid(server_pid_, &status, 0);
        server_pid_ = -1;
    }
    server_running_ = false;
}

bool OCRExtractor::isServerRunning() const {
    return server_running_;
}

std::string OCRExtractor::sendRequest(const std::string& json_request) {
    if (!server_running_) {
        if (!startServer()) return "[]";
    }

    fprintf(server_stdin_, "%s\n", json_request.c_str());
    fflush(server_stdin_);

    char buffer[65536];
    if (fgets(buffer, sizeof(buffer), server_stdout_) != nullptr) {
        return std::string(buffer);
    }

    std::cerr << "[OCRExtractor] No se recibio respuesta del servidor" << std::endl;
    return "[]";
}

std::vector<OCRResult> OCRExtractor::requestOCR(const std::string& image_path) {
    json request;
    request["image"] = image_path;
    std::string response = sendRequest(request.dump());
    return parseOCRResponse(response);
}

std::vector<std::vector<OCRResult>> OCRExtractor::requestBatchOCR(
    const std::vector<std::string>& image_paths) {

    json request;
    json batch = json::array();
    for (const auto& path : image_paths) {
        batch.push_back({{"image", path}});
    }
    request["batch"] = batch;

    std::string response = sendRequest(request.dump());

    std::vector<std::vector<OCRResult>> all_results;
    try {
        auto parsed = json::parse(response);
        if (parsed.is_array()) {
            for (const auto& item : parsed) {
                std::string item_str = item.dump();
                all_results.push_back(parseOCRResponse(item_str));
            }
        }
    } catch (const json::parse_error& e) {
        std::cerr << "[OCRExtractor] Error parseando batch: " << e.what() << std::endl;
    }

    return all_results;
}

std::vector<OCRResult> OCRExtractor::parseOCRResponse(const std::string& json_str) {
    std::vector<OCRResult> results;

    try {
        auto parsed = json::parse(json_str);

        if (parsed.is_object() && parsed.contains("error")) {
            std::cerr << "[OCRExtractor] Error OCR: "
                      << parsed["error"].get<std::string>() << std::endl;
            return results;
        }

        if (!parsed.is_array()) return results;

        for (const auto& item : parsed) {
            if (item.is_object() && item.contains("error")) continue;

            OCRResult r;
            r.text = item.value("text", "");
            r.confidence = item.value("confidence", 0.0);

            if (item.contains("bbox")) {
                const auto& bbox = item["bbox"];
                int x1 = bbox["top_left"][0].get<int>();
                int y1 = bbox["top_left"][1].get<int>();
                int x2 = bbox["bottom_right"][0].get<int>();
                int y2 = bbox["bottom_right"][1].get<int>();
                r.bbox = cv::Rect(x1, y1, x2 - x1, y2 - y1);
            }

            results.push_back(r);
        }
    } catch (const json::parse_error& e) {
        std::cerr << "[OCRExtractor] Error parseando JSON: " << e.what() << std::endl;
    }

    return results;
}

std::string OCRExtractor::saveTempImage(const cv::Mat& img, const std::string& prefix) {
    std::string path = temp_dir_ + "/" + prefix + "_" + std::to_string(temp_counter_++) + ".png";
    cv::imwrite(path, img);
    return path;
}

OCRResult OCRExtractor::extractFromCell(const cv::Mat& cell_img) {
    std::string temp_path = saveTempImage(cell_img, "cell");
    auto results = requestOCR(temp_path);
    fs::remove(temp_path);

    OCRResult combined;
    combined.confidence = 0.0;

    for (const auto& r : results) {
        if (!combined.text.empty()) combined.text += " ";
        combined.text += r.text;
        combined.confidence += r.confidence;
    }

    if (!results.empty()) {
        combined.confidence /= static_cast<double>(results.size());
        combined.bbox = results[0].bbox;
    }

    return combined;
}

std::vector<std::vector<OCRResult>> OCRExtractor::extractFromGrid(
    const cv::Mat& img, const TableGrid& grid) {

    std::cout << "[OCRExtractor] Extrayendo texto de grid ("
              << grid.cells.size() << " filas)..." << std::endl;

    std::vector<std::string> cell_paths;
    std::vector<std::pair<int, int>> cell_coords;

    for (size_t r = 0; r < grid.cells.size(); ++r) {
        for (size_t c = 0; c < grid.cells[r].size(); ++c) {
            cv::Rect cell_rect = grid.cells[r][c] & cv::Rect(0, 0, img.cols, img.rows);
            if (cell_rect.width <= 0 || cell_rect.height <= 0) {
                cell_paths.push_back("");
                cell_coords.push_back({static_cast<int>(r), static_cast<int>(c)});
                continue;
            }
            cv::Mat cell_img = img(cell_rect);
            std::string path = saveTempImage(cell_img, "grid");
            cell_paths.push_back(path);
            cell_coords.push_back({static_cast<int>(r), static_cast<int>(c)});
        }
    }

    std::vector<std::string> valid_paths;
    std::vector<int> valid_indices;
    for (size_t i = 0; i < cell_paths.size(); ++i) {
        if (!cell_paths[i].empty()) {
            valid_paths.push_back(cell_paths[i]);
            valid_indices.push_back(static_cast<int>(i));
        }
    }

    std::cout << "[OCRExtractor] Procesando " << valid_paths.size()
              << " celdas en batch..." << std::endl;

    const size_t BATCH_SIZE = 20;
    std::vector<std::vector<OCRResult>> batch_results_all;

    for (size_t start = 0; start < valid_paths.size(); start += BATCH_SIZE) {
        size_t end = std::min(start + BATCH_SIZE, valid_paths.size());
        std::vector<std::string> batch_paths(valid_paths.begin() + start,
                                              valid_paths.begin() + end);
        auto batch_res = requestBatchOCR(batch_paths);
        batch_results_all.insert(batch_results_all.end(),
                                  batch_res.begin(), batch_res.end());

        std::cout << "[OCRExtractor] Batch " << (start / BATCH_SIZE + 1)
                  << " completado (" << end << "/" << valid_paths.size() << ")" << std::endl;
    }

    int num_rows = static_cast<int>(grid.cells.size());
    std::vector<std::vector<OCRResult>> table_results(num_rows);
    for (int r = 0; r < num_rows; ++r) {
        table_results[r].resize(grid.cells[r].size());
    }

    int batch_idx = 0;
    for (size_t i = 0; i < cell_coords.size(); ++i) {
        int r = cell_coords[i].first;
        int c = cell_coords[i].second;

        if (cell_paths[i].empty()) {
            table_results[r][c].text = "";
            table_results[r][c].confidence = 0.0;
            continue;
        }

        if (batch_idx < static_cast<int>(batch_results_all.size())) {
            OCRResult combined;
            combined.confidence = 0.0;
            for (const auto& res : batch_results_all[batch_idx]) {
                if (!combined.text.empty()) combined.text += " ";
                combined.text += res.text;
                combined.confidence += res.confidence;
            }
            if (!batch_results_all[batch_idx].empty()) {
                combined.confidence /= static_cast<double>(batch_results_all[batch_idx].size());
            }
            combined.bbox = grid.cells[r][c];
            table_results[r][c] = combined;
        }
        batch_idx++;
    }

    for (const auto& path : cell_paths) {
        if (!path.empty()) fs::remove(path);
    }

    std::cout << "[OCRExtractor] Extraccion de grid completada." << std::endl;
    return table_results;
}

std::vector<OCRResult> OCRExtractor::extractFromRegion(const cv::Mat& region) {
    std::string temp_path = saveTempImage(region, "region");
    auto results = requestOCR(temp_path);
    fs::remove(temp_path);
    return results;
}

std::vector<OCRResult> OCRExtractor::extractAll(const cv::Mat& image) {
    std::cout << "[OCRExtractor] Extrayendo texto de imagen completa..." << std::endl;
    std::string temp_path = saveTempImage(image, "full");
    auto results = requestOCR(temp_path);
    fs::remove(temp_path);
    std::cout << "[OCRExtractor] " << results.size() << " bloques de texto extraidos" << std::endl;
    return results;
}
