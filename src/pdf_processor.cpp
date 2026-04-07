#include "pdf_processor.h"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <sstream>
#include <filesystem>
#include <array>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

// Ejecuta un comando del sistema y captura su salida (stdout + stderr)
static std::string execCommand(const std::string& cmd, bool throw_on_error = true) {
    // Redirigir stderr a stdout para capturar mensajes de error
    std::string full_cmd = cmd + " 2>&1";
    std::array<char, 256> buffer;
    std::string result;
    FILE* pipe = popen(full_cmd.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Error al ejecutar comando: " + cmd);
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result += buffer.data();
    }
    int ret = pclose(pipe);
    if (ret != 0 && throw_on_error) {
        std::string error_msg = "Comando fallo con codigo " + std::to_string(ret) + ": " + cmd;
        if (!result.empty()) {
            error_msg += "\nSalida: " + result;
        }
        throw std::runtime_error(error_msg);
    }
    return result;
}

void PDFProcessor::setPassword(const std::string& password) {
    password_ = password;
}

bool PDFProcessor::isPopplerAvailable() {
    try {
        execCommand("which pdftoppm", true);
        return true;
    } catch (...) {
        return false;
    }
}

bool PDFProcessor::needsPassword(const std::string& pdf_path) {
    std::string cmd = "pdfinfo '" + pdf_path + "'";
    std::string output = execCommand(cmd, false);
    if (output.find("Incorrect password") != std::string::npos ||
        output.find("password") != std::string::npos) {
        return true;
    }
    return false;
}

int PDFProcessor::getPageCount(const std::string& pdf_path) {
    if (!fs::exists(pdf_path)) {
        throw std::runtime_error("Archivo PDF no encontrado: " + pdf_path);
    }

    std::ostringstream cmd;
    cmd << "pdfinfo";
    if (!password_.empty()) {
        cmd << " -upw '" << password_ << "'";
    }
    cmd << " '" << pdf_path << "'";

    std::string output;
    try {
        output = execCommand(cmd.str());
    } catch (...) {
        return -1;
    }

    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("Pages:") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                std::string num_str = line.substr(pos + 1);
                num_str.erase(0, num_str.find_first_not_of(" \t"));
                num_str.erase(num_str.find_last_not_of(" \t\r\n") + 1);
                try {
                    return std::stoi(num_str);
                } catch (...) {
                    return -1;
                }
            }
        }
    }
    return -1;
}

std::vector<std::string> PDFProcessor::convertToImages(
    const std::string& pdf_path,
    const std::string& output_dir,
    int dpi) {

    if (!fs::exists(pdf_path)) {
        throw std::runtime_error("Archivo PDF no encontrado: " + pdf_path);
    }

    if (!isPopplerAvailable()) {
        throw std::runtime_error(
            "pdftoppm no esta instalado. Instalar con:\n"
            "  Linux: sudo apt install poppler-utils");
    }

    // Usar rutas absolutas para evitar problemas
    std::string abs_output_dir = fs::absolute(output_dir).string();
    std::string abs_pdf_path = fs::absolute(pdf_path).string();

    fs::create_directories(abs_output_dir);

    // Construir prefijo de salida (reemplazar espacios para evitar problemas)
    std::string base_name = fs::path(abs_pdf_path).stem().string();
    for (char& c : base_name) {
        if (c == ' ' || c == '(' || c == ')' || c == '&') {
            c = '_';
        }
    }
    std::string output_prefix = abs_output_dir + "/" + base_name;

    // Ejecutar pdftoppm con contrasena si esta configurada
    std::ostringstream cmd;
    cmd << "pdftoppm"
        << " -png"
        << " -r " << dpi;

    if (!password_.empty()) {
        cmd << " -upw '" << password_ << "'";
    }

    cmd << " '" << abs_pdf_path << "'"
        << " '" << output_prefix << "'";

    std::cout << "[PDFProcessor] Ejecutando: pdftoppm -png -r " << dpi;
    if (!password_.empty()) std::cout << " -upw ****";
    std::cout << " '" << abs_pdf_path << "' '" << output_prefix << "'" << std::endl;

    try {
        execCommand(cmd.str());
    } catch (const std::exception& e) {
        std::string err = e.what();
        if (err.find("Incorrect password") != std::string::npos ||
            err.find("password") != std::string::npos) {
            throw std::runtime_error(
                "El PDF esta protegido con contrasena. "
                "Por favor ingrese la contrasena correcta.");
        }
        throw std::runtime_error("Error al convertir PDF: " + err);
    }

    // Recolectar archivos PNG generados
    std::vector<std::string> image_paths;
    for (const auto& entry : fs::directory_iterator(abs_output_dir)) {
        std::string filename = entry.path().filename().string();
        if (filename.find(base_name) != std::string::npos &&
            entry.path().extension() == ".png") {
            image_paths.push_back(entry.path().string());
        }
    }

    std::sort(image_paths.begin(), image_paths.end());

    std::cout << "[PDFProcessor] " << image_paths.size()
              << " paginas convertidas a PNG" << std::endl;

    return image_paths;
}
