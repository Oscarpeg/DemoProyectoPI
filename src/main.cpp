#include <iostream>
#include <string>
#include <filesystem>
#include <iomanip>

#include "pdf_processor.h"
#include "image_preprocessor.h"
#include "ocr_extractor.h"
#include "page_classifier.h"
#include "table_detector.h"
#include "data_structurer.h"
#include "statistical_analyzer.h"
#include "graph_generator.h"
#include "csv_exporter.h"
#include "ui_manager.h"

#include <wx/wx.h>

namespace fs = std::filesystem;

// ============================================================
// Modo consola: pipeline completo sin GUI
// ============================================================
int runConsole(const std::string& pdf_path, const std::string& password) {
    std::cout << "========================================" << std::endl;
    std::cout << " Sistema de Analisis de Extractos" << std::endl;
    std::cout << " Modo Consola" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // 1. Verificar dependencias
    if (!PDFProcessor::isPopplerAvailable()) {
        std::cerr << "ERROR: pdftoppm no esta instalado." << std::endl;
        std::cerr << "Instalar con: sudo apt install poppler-utils" << std::endl;
        return 1;
    }

    if (!fs::exists(pdf_path)) {
        std::cerr << "ERROR: Archivo no encontrado: " << pdf_path << std::endl;
        return 1;
    }

    std::cout << "[1/8] Archivo PDF: " << pdf_path << std::endl;

    // 2. Verificar contrasena
    std::string pw = password;
    if (pw.empty() && PDFProcessor::needsPassword(pdf_path)) {
        std::cout << "El PDF esta protegido. Ingrese la contrasena: ";
        std::getline(std::cin, pw);
    }

    // 3. Convertir PDF a imagenes
    std::cout << "\n[2/8] Convirtiendo PDF a imagenes..." << std::endl;
    PDFProcessor pdf;
    if (!pw.empty()) {
        pdf.setPassword(pw);
    }

    fs::create_directories("output/images");
    auto images = pdf.convertToImages(pdf_path, "output/images", 300);

    if (images.empty()) {
        std::cerr << "ERROR: No se pudieron generar imagenes del PDF." << std::endl;
        return 1;
    }
    std::cout << "  " << images.size() << " paginas convertidas." << std::endl;

    // 4. Iniciar servidor OCR
    std::cout << "\n[3/8] Iniciando servidor OCR..." << std::endl;
    OCRExtractor ocr;
    if (!ocr.startServer()) {
        std::cerr << "ERROR: No se pudo iniciar el servidor OCR." << std::endl;
        std::cerr << "Verificar que Python3 y EasyOCR estan instalados." << std::endl;
        return 1;
    }

    // 5. Procesar cada pagina
    std::cout << "\n[4/8] Procesando paginas..." << std::endl;
    ImagePreprocessor preproc;
    PageClassifier classifier(ocr);
    TableDetector table_det;
    DataStructurer structurer;
    ExtractoCompleto extracto;

    for (size_t i = 0; i < images.size(); ++i) {
        int page_num = static_cast<int>(i + 1);
        std::cout << "\n--- Pagina " << page_num << " ---" << std::endl;

        cv::Mat img = cv::imread(images[i]);
        if (img.empty()) {
            std::cerr << "  ERROR: No se pudo leer imagen: " << images[i] << std::endl;
            continue;
        }

        std::cout << "  Dimensiones: " << img.cols << "x" << img.rows << std::endl;

        // Preprocesar
        cv::Mat preprocessed = preproc.fullPreprocess(img);

        // Clasificar
        PageType type = classifier.classify(img);
        std::cout << "  Tipo: " << PageClassifier::pageTypeToString(type) << std::endl;

        switch (type) {
            case PageType::RESUMEN: {
                std::cout << "  Extrayendo resumen del portafolio..." << std::endl;
                auto ocr_data = ocr.extractAll(img);
                extracto.resumen = structurer.buildResumen(ocr_data);
                std::cout << "  Cliente: " << extracto.resumen.nombre_cliente << std::endl;
                std::cout << "  Total Portafolio: $" << std::fixed << std::setprecision(2)
                          << extracto.resumen.total_portafolio << std::endl;
                break;
            }

            case PageType::DETALLE_RENTA_FIJA: {
                std::cout << "  Extrayendo OCR de pagina completa..." << std::endl;
                auto ocr_data = ocr.extractAll(img);

                // Detectar tablas desde bounding boxes del OCR (no morfologia)
                std::cout << "  Detectando tablas desde datos OCR..." << std::endl;
                auto ocr_tables = structurer.detectTablesFromOCR(ocr_data, img.cols);

                for (size_t t = 0; t < ocr_tables.size(); ++t) {
                    const auto& tbl = ocr_tables[t];
                    std::cout << "  Tabla " << (t + 1) << ": tipo=" << tbl.type
                              << ", " << tbl.rows.size() << " filas" << std::endl;

                    auto grid = structurer.ocrTableToGrid(tbl);

                    if (tbl.type == "renta_fija") {
                        auto rf = structurer.buildRentaFija(grid);
                        if (!rf.empty()) {
                            extracto.renta_fija.insert(extracto.renta_fija.end(),
                                                        rf.begin(), rf.end());
                            std::cout << "  -> " << rf.size() << " instrumentos RF" << std::endl;
                        }
                    } else if (tbl.type == "saldos") {
                        auto saldos = structurer.buildSaldosEfectivo(grid);
                        extracto.saldos_efectivo.insert(extracto.saldos_efectivo.end(),
                                                         saldos.begin(), saldos.end());
                        std::cout << "  -> " << saldos.size() << " saldos" << std::endl;
                    }
                    // fic_resumen y unknown se procesan con el OCR completo si es necesario
                }

                // FALLBACK: si no se encontraron instrumentos RF, usar lineas visuales
                if (extracto.renta_fija.empty()) {
                    std::cout << "  Fallback: extrayendo RF desde lineas visuales..." << std::endl;
                    auto rf = structurer.buildRentaFijaFromLines(ocr_data);
                    extracto.renta_fija.insert(extracto.renta_fija.end(),
                                                rf.begin(), rf.end());
                    std::cout << "  Fallback: " << rf.size() << " instrumentos extraidos" << std::endl;
                }
                break;
            }

            case PageType::EXTRACTO_FONDOS: {
                std::cout << "  Extrayendo datos de fondos..." << std::endl;
                auto ocr_data = ocr.extractAll(img);
                auto fondo = structurer.buildFondo(ocr_data);
                extracto.fondos.push_back(fondo);
                std::cout << "  Fondo: " << fondo.nombre_fondo << std::endl;
                std::cout << "  Nuevo Saldo: $" << std::fixed << std::setprecision(2)
                          << fondo.nuevo_saldo << std::endl;
                break;
            }

            case PageType::DESCONOCIDO:
            default:
                std::cout << "  (Pagina omitida - informativa)" << std::endl;
                break;
        }
    }

    // 6. Validar
    std::cout << "\n[5/8] Validando datos extraidos..." << std::endl;
    structurer.validate(extracto);
    auto errors = structurer.getValidationErrors();
    if (!errors.empty()) {
        for (const auto& err : errors) {
            std::cout << "  ADVERTENCIA: " << err << std::endl;
        }
    } else {
        std::cout << "  Validacion exitosa." << std::endl;
    }

    // 7. Analisis estadistico
    std::cout << "\n[6/8] Ejecutando analisis estadistico..." << std::endl;
    StatisticalAnalyzer analyzer;

    // Cargar datos historicos
    std::vector<ExtractoCompleto> serie;
    auto historical = StatisticalAnalyzer::loadHistoricalData("data/historico_inicial.csv");
    for (const auto& record : historical) {
        ExtractoCompleto hist_ext;
        auto it = record.find("total_portafolio");
        if (it != record.end()) {
            try { hist_ext.resumen.total_portafolio = std::stod(it->second); } catch (...) {}
        }
        it = record.find("renta_fija");
        if (it != record.end()) {
            try { hist_ext.resumen.activos["Renta Fija"] = std::stod(it->second); } catch (...) {}
        }
        it = record.find("fic");
        if (it != record.end()) {
            try { hist_ext.resumen.activos["FIC"] = std::stod(it->second); } catch (...) {}
        }
        it = record.find("efectivo");
        if (it != record.end()) {
            try { hist_ext.resumen.activos["Saldos en Efectivo"] = std::stod(it->second); } catch (...) {}
        }
        serie.push_back(hist_ext);
    }
    serie.push_back(extracto);

    AnalysisResult analysis = analyzer.analyzeTimeSeries(serie);

    // Imprimir resultados del analisis
    std::cout << "\n  Composicion del portafolio:" << std::endl;
    for (const auto& [key, val] : analysis.composicion_porcentaje) {
        std::cout << "    " << key << ": " << std::fixed << std::setprecision(1)
                  << val << "%" << std::endl;
    }

    std::cout << "\n  Tasas de valoracion: Media=" << std::fixed << std::setprecision(2)
              << analysis.tasas_valoracion.mean << "%, Desv="
              << analysis.tasas_valoracion.std_dev << "%" << std::endl;

    if (!analysis.alertas.empty()) {
        std::cout << "\n  Alertas:" << std::endl;
        for (const auto& a : analysis.alertas) {
            std::cout << "    [" << a.type << "] " << a.message << std::endl;
        }
    }

    // 8. Generar graficas
    std::cout << "\n[7/8] Generando graficas..." << std::endl;
    fs::create_directories("output/graphs");
    GraphGenerator grapher;
    auto graph_paths = grapher.generateAll(extracto, analysis, "output/graphs");
    std::cout << "  " << graph_paths.size() << " graficas generadas." << std::endl;

    // 9. Exportar CSV
    std::cout << "\n[8/8] Exportando CSV y JSON..." << std::endl;
    fs::create_directories("output/csv");
    fs::create_directories("output/json");
    CSVExporter exporter;
    auto csv_paths = exporter.exportAll(extracto, analysis, "output/csv");
    std::cout << "  " << csv_paths.size() << " archivos CSV exportados." << std::endl;

    extracto.saveToFile("output/json/extracto.json");

    // Resumen final
    std::cout << "\n========================================" << std::endl;
    std::cout << " PROCESAMIENTO COMPLETADO" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << " Cliente: " << extracto.resumen.nombre_cliente << std::endl;
    std::cout << " Total Portafolio: $" << std::fixed << std::setprecision(2)
              << extracto.resumen.total_portafolio << std::endl;
    std::cout << " Instrumentos RF: " << extracto.renta_fija.size() << std::endl;
    std::cout << " Fondos: " << extracto.fondos.size() << std::endl;
    std::cout << " Graficas: output/graphs/" << std::endl;
    std::cout << " CSV: output/csv/" << std::endl;
    std::cout << " JSON: output/json/extracto.json" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}

// ============================================================
// main: Dual mode (GUI o consola)
// ============================================================
int main(int argc, char* argv[]) {
    // Modo consola si se pasa un archivo PDF como argumento
    if (argc >= 2) {
        std::string arg1 = argv[1];

        // Verificar si es un archivo PDF (no un flag de wx)
        if (arg1.size() > 4 && arg1.substr(arg1.size() - 4) == ".pdf") {
            std::string password;
            // Buscar flag --pw
            for (int i = 2; i < argc - 1; ++i) {
                if (std::string(argv[i]) == "--pw") {
                    password = argv[i + 1];
                    break;
                }
            }
            return runConsole(arg1, password);
        }
    }

    // Modo GUI (wxWidgets)
    wxApp::SetInstance(new InvestmentApp());
    return wxEntry(argc, argv);
}
