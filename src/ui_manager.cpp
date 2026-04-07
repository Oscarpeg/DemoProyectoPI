#include "ui_manager.h"
#include "pdf_processor.h"
#include "image_preprocessor.h"
#include "page_classifier.h"
#include "table_detector.h"
#include "ocr_extractor.h"
#include "graph_generator.h"
#include "csv_exporter.h"

#include <wx/textdlg.h>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

wxIMPLEMENT_APP_NO_MAIN(InvestmentApp);

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_BUTTON(ID_SELECT_PDF, MainFrame::OnSelectPDF)
    EVT_BUTTON(ID_PROCESS, MainFrame::OnProcess)
    EVT_BUTTON(ID_EXPORT_CSV, MainFrame::OnExportCSV)
    EVT_LISTBOX(ID_SELECT_GRAPH, MainFrame::OnSelectGraph)
    EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
    EVT_MENU(wxID_EXIT, MainFrame::OnQuit)
    EVT_COMMAND(ID_PROCESS_COMPLETE, wxEVT_COMMAND_BUTTON_CLICKED, MainFrame::OnProcessComplete)
wxEND_EVENT_TABLE()

bool InvestmentApp::OnInit() {
    wxInitAllImageHandlers();  // CRITICO: necesario para cargar PNG/JPG en wxStaticBitmap
    auto* frame = new MainFrame();
    frame->Show(true);
    return true;
}

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "Sistema de Analisis de Extractos de Inversion",
              wxDefaultPosition, wxSize(1100, 750)),
      processing_(false) {
    createMenuBar();
    createLayout();
    Centre();
    lbl_status_->SetLabel("Listo. Seleccione un archivo PDF.");
}

void MainFrame::createMenuBar() {
    auto* menuBar = new wxMenuBar();

    auto* menuArchivo = new wxMenu();
    menuArchivo->Append(ID_SELECT_PDF, "&Abrir PDF...\tCtrl+O");
    menuArchivo->Append(ID_PROCESS, "&Procesar\tCtrl+P");
    menuArchivo->Append(ID_EXPORT_CSV, "&Exportar CSV\tCtrl+E");
    menuArchivo->AppendSeparator();
    menuArchivo->Append(wxID_EXIT, "&Salir\tCtrl+Q");

    auto* menuAyuda = new wxMenu();
    menuAyuda->Append(wxID_ABOUT, "&Acerca de...");

    menuBar->Append(menuArchivo, "&Archivo");
    menuBar->Append(menuAyuda, "&Ayuda");
    SetMenuBar(menuBar);
}

void MainFrame::createLayout() {
    auto* main_panel = new wxPanel(this);
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);

    // Barra superior de botones
    auto* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_seleccionar_pdf_ = new wxButton(main_panel, ID_SELECT_PDF, "Seleccionar PDF");
    btn_procesar_ = new wxButton(main_panel, ID_PROCESS, "Procesar");
    btn_exportar_csv_ = new wxButton(main_panel, ID_EXPORT_CSV, "Exportar CSV");
    btn_procesar_->Enable(false);
    btn_exportar_csv_->Enable(false);

    btn_sizer->Add(btn_seleccionar_pdf_, 0, wxALL, 5);
    btn_sizer->Add(btn_procesar_, 0, wxALL, 5);
    btn_sizer->Add(btn_exportar_csv_, 0, wxALL, 5);
    main_sizer->Add(btn_sizer, 0, wxEXPAND);

    // Barra de progreso
    progress_bar_ = new wxGauge(main_panel, wxID_ANY, 100);
    main_sizer->Add(progress_bar_, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

    // Status label
    lbl_status_ = new wxStaticText(main_panel, wxID_ANY, "");
    main_sizer->Add(lbl_status_, 0, wxEXPAND | wxALL, 5);

    // Notebook con tabs
    notebook_ = new wxNotebook(main_panel, wxID_ANY);
    notebook_->AddPage(createResumenTab(notebook_), "Resumen");
    notebook_->AddPage(createRentaFijaTab(notebook_), "Renta Fija");
    notebook_->AddPage(createFondosTab(notebook_), "Fondos");
    notebook_->AddPage(createGraficasTab(notebook_), "Graficas");
    notebook_->AddPage(createAnalisisTab(notebook_), "Analisis");
    main_sizer->Add(notebook_, 1, wxEXPAND | wxALL, 5);

    // Log panel
    txt_log_ = new wxTextCtrl(main_panel, wxID_ANY, "", wxDefaultPosition,
                               wxSize(-1, 120), wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL);
    main_sizer->Add(txt_log_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

    main_panel->SetSizer(main_sizer);
}

wxPanel* MainFrame::createResumenTab(wxNotebook* parent) {
    auto* panel = new wxPanel(parent);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    grid_resumen_ = new wxGrid(panel, wxID_ANY);
    grid_resumen_->CreateGrid(15, 2);
    grid_resumen_->SetColLabelValue(0, "Campo");
    grid_resumen_->SetColLabelValue(1, "Valor");
    grid_resumen_->SetColSize(0, 250);
    grid_resumen_->SetColSize(1, 400);
    grid_resumen_->EnableEditing(false);
    sizer->Add(grid_resumen_, 1, wxEXPAND | wxALL, 5);
    panel->SetSizer(sizer);
    return panel;
}

wxPanel* MainFrame::createRentaFijaTab(wxNotebook* parent) {
    auto* panel = new wxPanel(parent);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    grid_renta_fija_ = new wxGrid(panel, wxID_ANY);
    grid_renta_fija_->CreateGrid(0, 10);
    std::vector<std::string> cols = {
        "Nemotecnico", "Emision", "Vencimiento", "Compra",
        "Tasa Facial", "Periodicidad", "Nominal",
        "Tasa Negoc.", "Tasa Valor.", "Valor Mercado"
    };
    for (int i = 0; i < 10; ++i) {
        grid_renta_fija_->SetColLabelValue(i, cols[i]);
        grid_renta_fija_->SetColSize(i, 100);
    }
    grid_renta_fija_->EnableEditing(false);
    sizer->Add(grid_renta_fija_, 1, wxEXPAND | wxALL, 5);
    panel->SetSizer(sizer);
    return panel;
}

wxPanel* MainFrame::createFondosTab(wxNotebook* parent) {
    auto* panel = new wxPanel(parent);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    grid_fondos_ = new wxGrid(panel, wxID_ANY);
    grid_fondos_->CreateGrid(15, 2);
    grid_fondos_->SetColLabelValue(0, "Campo");
    grid_fondos_->SetColLabelValue(1, "Valor");
    grid_fondos_->SetColSize(0, 250);
    grid_fondos_->SetColSize(1, 400);
    grid_fondos_->EnableEditing(false);
    sizer->Add(grid_fondos_, 1, wxEXPAND | wxALL, 5);
    panel->SetSizer(sizer);
    return panel;
}

wxPanel* MainFrame::createGraficasTab(wxNotebook* parent) {
    auto* panel = new wxPanel(parent);
    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    list_graficas_ = new wxListBox(panel, ID_SELECT_GRAPH, wxDefaultPosition, wxSize(200, -1));
    sizer->Add(list_graficas_, 0, wxEXPAND | wxALL, 5);

    scroll_grafica_ = new wxScrolledWindow(panel, wxID_ANY);
    scroll_grafica_->SetScrollRate(5, 5);
    auto* scroll_sizer = new wxBoxSizer(wxVERTICAL);
    panel_grafica_ = new wxStaticBitmap(scroll_grafica_, wxID_ANY, wxNullBitmap);
    scroll_sizer->Add(panel_grafica_, 0, wxALL, 5);
    scroll_grafica_->SetSizer(scroll_sizer);

    sizer->Add(scroll_grafica_, 1, wxEXPAND | wxALL, 5);
    panel->SetSizer(sizer);
    return panel;
}

wxPanel* MainFrame::createAnalisisTab(wxNotebook* parent) {
    auto* panel = new wxPanel(parent);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    grid_analisis_ = new wxGrid(panel, wxID_ANY);
    grid_analisis_->CreateGrid(20, 2);
    grid_analisis_->SetColLabelValue(0, "Metrica");
    grid_analisis_->SetColLabelValue(1, "Valor");
    grid_analisis_->SetColSize(0, 350);
    grid_analisis_->SetColSize(1, 300);
    grid_analisis_->EnableEditing(false);
    sizer->Add(grid_analisis_, 1, wxEXPAND | wxALL, 5);
    panel->SetSizer(sizer);
    return panel;
}

void MainFrame::OnSelectPDF(wxCommandEvent&) {
    wxFileDialog dlg(this, "Seleccionar extracto PDF", "", "",
                     "Archivos PDF (*.pdf)|*.pdf", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_CANCEL) return;

    current_pdf_path_ = dlg.GetPath().ToStdString();
    logMessage("PDF seleccionado: " + current_pdf_path_);

    // Verificar si necesita contrasena
    if (PDFProcessor::needsPassword(current_pdf_path_)) {
        wxPasswordEntryDialog pwd_dlg(this,
            "El PDF esta protegido. Ingrese la contrasena:",
            "Contrasena PDF");
        if (pwd_dlg.ShowModal() == wxID_OK) {
            current_pdf_password_ = pwd_dlg.GetValue().ToStdString();
            logMessage("Contrasena configurada.");
        } else {
            logMessage("Contrasena no proporcionada.");
            return;
        }
    }

    btn_procesar_->Enable(true);
    lbl_status_->SetLabel("PDF cargado. Presione Procesar.");
}

void MainFrame::OnProcess(wxCommandEvent&) {
    if (processing_) {
        logMessage("Ya se esta procesando un documento.");
        return;
    }
    if (current_pdf_path_.empty()) {
        wxMessageBox("Primero seleccione un archivo PDF.", "Error", wxICON_ERROR);
        return;
    }

    processing_ = true;
    btn_procesar_->Enable(false);
    btn_exportar_csv_->Enable(false);
    progress_bar_->SetValue(0);
    lbl_status_->SetLabel("Procesando...");
    logMessage("Iniciando procesamiento del extracto...");

    // Lanzar en hilo separado
    std::thread([this]() {
        processExtracto();
    }).detach();
}

void MainFrame::processExtracto() {
    try {
        // Copiar variables por VALOR para evitar crashes cross-thread
        std::string pdf_path = current_pdf_path_;
        std::string pdf_password = current_pdf_password_;

        // 1. PDF -> imagenes
        CallAfter([this]() {
            logMessage("[1/7] Convirtiendo PDF a imagenes...");
            setProgress(5);
        });

        PDFProcessor pdf;
        if (!pdf_password.empty()) {
            pdf.setPassword(pdf_password);
        }
        auto images = pdf.convertToImages(pdf_path, "output/images", 300);

        if (images.empty()) {
            CallAfter([this]() {
                logMessage("ERROR: No se generaron imagenes del PDF.");
                lbl_status_->SetLabel("Error en conversion.");
                processing_ = false;
                btn_procesar_->Enable(true);
            });
            return;
        }

        int num_pages = static_cast<int>(images.size());
        std::string pages_msg = std::to_string(num_pages) + " paginas convertidas.";
        CallAfter([this, pages_msg]() {
            logMessage(pages_msg);
            setProgress(15);
        });

        // 2. Iniciar OCR server
        CallAfter([this]() {
            logMessage("[2/7] Iniciando servidor OCR...");
            setProgress(20);
        });

        OCRExtractor ocr;
        if (!ocr.startServer()) {
            CallAfter([this]() {
                logMessage("ERROR: No se pudo iniciar el servidor OCR.");
                lbl_status_->SetLabel("Error OCR.");
                processing_ = false;
                btn_procesar_->Enable(true);
            });
            return;
        }

        ImagePreprocessor preproc;
        PageClassifier classifier(ocr);
        TableDetector table_det;
        DataStructurer structurer;
        ExtractoCompleto extracto;

        // 3. Procesar cada pagina
        for (int i = 0; i < num_pages; ++i) {
            int page_num = i + 1;
            std::string page_msg = "[3/7] Procesando pagina " + std::to_string(page_num) +
                                   " de " + std::to_string(num_pages) + "...";
            int progress = 25 + (i * 35 / num_pages);
            CallAfter([this, page_msg, progress]() {
                logMessage(page_msg);
                setProgress(progress);
            });

            cv::Mat img = cv::imread(images[i]);
            if (img.empty()) continue;

            cv::Mat preprocessed = preproc.fullPreprocess(img);
            PageType type = classifier.classify(img);

            std::string type_str = PageClassifier::pageTypeToString(type);
            std::string class_msg = "  Pagina " + std::to_string(page_num) + " -> " + type_str;
            CallAfter([this, class_msg]() { logMessage(class_msg); });

            switch (type) {
                case PageType::RESUMEN: {
                    auto ocr_data = ocr.extractAll(img);
                    extracto.resumen = structurer.buildResumen(ocr_data);
                    break;
                }
                case PageType::DETALLE_RENTA_FIJA: {
                    // Extraer OCR de pagina completa
                    auto ocr_data = ocr.extractAll(img);

                    // Detectar tablas desde bounding boxes del OCR
                    auto ocr_tables = structurer.detectTablesFromOCR(ocr_data, img.cols);
                    std::string tbl_msg = "  " + std::to_string(ocr_tables.size()) +
                                          " tablas detectadas (OCR)";
                    CallAfter([this, tbl_msg]() { logMessage(tbl_msg); });

                    for (size_t t = 0; t < ocr_tables.size(); ++t) {
                        const auto& tbl = ocr_tables[t];
                        std::string grid_msg = "  Tabla " + std::to_string(t + 1) +
                                               ": tipo=" + tbl.type +
                                               ", " + std::to_string(tbl.rows.size()) + " filas";
                        CallAfter([this, grid_msg]() { logMessage(grid_msg); });

                        auto grid = structurer.ocrTableToGrid(tbl);

                        if (tbl.type == "renta_fija") {
                            auto rf = structurer.buildRentaFija(grid);
                            if (!rf.empty()) {
                                extracto.renta_fija.insert(extracto.renta_fija.end(),
                                                            rf.begin(), rf.end());
                            }
                        } else if (tbl.type == "saldos") {
                            auto saldos = structurer.buildSaldosEfectivo(grid);
                            extracto.saldos_efectivo.insert(extracto.saldos_efectivo.end(),
                                                             saldos.begin(), saldos.end());
                        }
                    }

                    // FALLBACK: si no se encontraron instrumentos RF, usar lineas visuales
                    if (extracto.renta_fija.empty()) {
                        CallAfter([this]() {
                            logMessage("  Fallback: extrayendo RF desde lineas visuales...");
                        });
                        auto rf = structurer.buildRentaFijaFromLines(ocr_data);
                        extracto.renta_fija.insert(extracto.renta_fija.end(),
                                                    rf.begin(), rf.end());
                    }
                    break;
                }
                case PageType::EXTRACTO_FONDOS: {
                    auto ocr_data = ocr.extractAll(img);
                    auto fondo = structurer.buildFondo(ocr_data);
                    extracto.fondos.push_back(fondo);
                    break;
                }
                case PageType::DESCONOCIDO:
                default: {
                    std::string skip_msg = "  Pagina " + std::to_string(page_num) + " omitida (informativa)";
                    CallAfter([this, skip_msg]() { logMessage(skip_msg); });
                    break;
                }
            }
        }

        // 4. Validar
        CallAfter([this]() {
            logMessage("[4/7] Validando datos extraidos...");
            setProgress(65);
        });

        structurer.validate(extracto);
        auto errors = structurer.getValidationErrors();
        for (const auto& err : errors) {
            std::string warn = "  ADVERTENCIA: " + err;
            CallAfter([this, warn]() { logMessage(warn); });
        }

        // 5. Analisis estadistico
        CallAfter([this]() {
            logMessage("[5/7] Ejecutando analisis estadistico...");
            setProgress(75);
        });

        StatisticalAnalyzer analyzer;

        // Cargar datos historicos
        auto historical = StatisticalAnalyzer::loadHistoricalData("data/historico_inicial.csv");
        std::vector<ExtractoCompleto> serie;
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

        // 6. Generar graficas
        CallAfter([this]() {
            logMessage("[6/7] Generando graficas...");
            setProgress(85);
        });

        std::string abs_graphs = fs::absolute("output/graphs").string();
        fs::create_directories(abs_graphs);
        GraphGenerator grapher;
        auto graph_paths = grapher.generateAll(extracto, analysis, abs_graphs);

        // 7. Guardar JSON
        CallAfter([this]() {
            logMessage("[7/7] Guardando resultados...");
            setProgress(95);
        });

        std::string abs_json = fs::absolute("output/json").string();
        fs::create_directories(abs_json);
        extracto.saveToFile(abs_json + "/extracto.json");

        // Actualizar estado compartido (copiar por valor)
        ExtractoCompleto final_extracto = extracto;
        AnalysisResult final_analysis = analysis;
        std::vector<std::string> final_graphs = graph_paths;

        CallAfter([this, final_extracto, final_analysis, final_graphs]() {
            current_extracto_ = final_extracto;
            current_analysis_ = final_analysis;
            graph_paths_ = final_graphs;

            updateResumenDisplay(current_extracto_);
            updateRentaFijaDisplay(current_extracto_);
            updateFondosDisplay(current_extracto_);
            updateAnalisisDisplay(current_analysis_);

            // Actualizar lista de graficas
            list_graficas_->Clear();
            for (const auto& path : graph_paths_) {
                std::string name = fs::path(path).filename().string();
                list_graficas_->Append(name);
            }
            if (!graph_paths_.empty()) {
                list_graficas_->SetSelection(0);
                showGraph(graph_paths_[0]);
            }

            setProgress(100);
            lbl_status_->SetLabel("Procesamiento completado.");
            logMessage("Procesamiento completado exitosamente.");
            processing_ = false;
            btn_procesar_->Enable(true);
            btn_exportar_csv_->Enable(true);
        });

    } catch (const std::exception& e) {
        std::string err_msg = std::string("ERROR: ") + e.what();
        CallAfter([this, err_msg]() {
            logMessage(err_msg);
            lbl_status_->SetLabel("Error durante el procesamiento.");
            processing_ = false;
            btn_procesar_->Enable(true);
        });
    }
}

void MainFrame::OnExportCSV(wxCommandEvent&) {
    wxDirDialog dlg(this, "Seleccionar directorio para exportar CSV");
    if (dlg.ShowModal() == wxID_CANCEL) return;

    std::string dir = dlg.GetPath().ToStdString();
    CSVExporter exporter;
    auto paths = exporter.exportAll(current_extracto_, current_analysis_, dir);

    logMessage("CSV exportados: " + std::to_string(paths.size()) + " archivos en " + dir);
    wxMessageBox("Se exportaron " + std::to_string(paths.size()) + " archivos CSV.",
                 "Exportacion completada", wxICON_INFORMATION);
}

void MainFrame::OnSelectGraph(wxCommandEvent&) {
    int sel = list_graficas_->GetSelection();
    if (sel >= 0 && sel < static_cast<int>(graph_paths_.size())) {
        showGraph(graph_paths_[sel]);
    }
}

void MainFrame::OnAbout(wxCommandEvent&) {
    wxMessageBox(
        "Sistema Inteligente de Extraccion y Analisis\n"
        "de Extractos de Inversion\n\n"
        "Version 1.0\n"
        "Procesamiento de PDFs de Acciones & Valores S.A.\n\n"
        "Tecnologias: C++17, OpenCV, EasyOCR, wxWidgets",
        "Acerca de", wxICON_INFORMATION);
}

void MainFrame::OnQuit(wxCommandEvent&) {
    Close(true);
}

void MainFrame::OnProcessComplete(wxCommandEvent&) {
    // Handled inline via CallAfter
}

void MainFrame::updateResumenDisplay(const ExtractoCompleto& extracto) {
    const auto& r = extracto.resumen;
    int row = 0;

    auto setRow = [&](const std::string& campo, const std::string& valor) {
        if (row >= grid_resumen_->GetNumberRows()) {
            grid_resumen_->AppendRows(1);
        }
        grid_resumen_->SetCellValue(row, 0, campo);
        grid_resumen_->SetCellValue(row, 1, valor);
        row++;
    };

    setRow("Fecha Extracto", r.fecha_extracto);
    setRow("Nombre Cliente", r.nombre_cliente);
    setRow("NIT", r.nit);
    setRow("Direccion", r.direccion);
    setRow("Ciudad", r.ciudad);
    setRow("Telefono", r.telefono);
    setRow("Asesor", r.asesor);

    for (const auto& [key, val] : r.activos) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << val;
        setRow(key, "$" + oss.str());
    }
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << r.total_activos;
        setRow("Total Activos", "$" + oss.str());
    }
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << r.total_pasivos;
        setRow("Total Pasivos", "$" + oss.str());
    }
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << r.total_portafolio;
        setRow("TOTAL PORTAFOLIO", "$" + oss.str());
    }

    grid_resumen_->AutoSize();
}

void MainFrame::updateRentaFijaDisplay(const ExtractoCompleto& extracto) {
    int current_rows = grid_renta_fija_->GetNumberRows();
    if (current_rows > 0) grid_renta_fija_->DeleteRows(0, current_rows);

    for (const auto& inst : extracto.renta_fija) {
        grid_renta_fija_->AppendRows(1);
        int row = grid_renta_fija_->GetNumberRows() - 1;

        grid_renta_fija_->SetCellValue(row, 0, inst.nemotecnico);
        grid_renta_fija_->SetCellValue(row, 1, inst.fecha_emision);
        grid_renta_fija_->SetCellValue(row, 2, inst.fecha_vencimiento);
        grid_renta_fija_->SetCellValue(row, 3, inst.fecha_compra);

        auto fmt = [](double v) -> std::string {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << v;
            return oss.str();
        };

        grid_renta_fija_->SetCellValue(row, 4, fmt(inst.tasa_facial) + "%");
        grid_renta_fija_->SetCellValue(row, 5, inst.periodicidad);
        grid_renta_fija_->SetCellValue(row, 6, fmt(inst.valor_nominal));
        grid_renta_fija_->SetCellValue(row, 7, fmt(inst.tasa_negociacion) + "%");
        grid_renta_fija_->SetCellValue(row, 8, fmt(inst.tasa_valoracion) + "%");
        grid_renta_fija_->SetCellValue(row, 9, fmt(inst.valor_mercado));
    }
    grid_renta_fija_->AutoSize();
}

void MainFrame::updateFondosDisplay(const ExtractoCompleto& extracto) {
    int row = 0;
    auto setRow = [&](const std::string& campo, const std::string& valor) {
        if (row >= grid_fondos_->GetNumberRows()) {
            grid_fondos_->AppendRows(1);
        }
        grid_fondos_->SetCellValue(row, 0, campo);
        grid_fondos_->SetCellValue(row, 1, valor);
        row++;
    };

    for (const auto& fondo : extracto.fondos) {
        setRow("Nombre Fondo", fondo.nombre_fondo);
        setRow("Codigo", fondo.codigo);
        setRow("Inversionista", fondo.nombre_inversionista);
        setRow("Identificacion", fondo.identificacion);

        auto fmt = [](double v) -> std::string {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << v;
            return oss.str();
        };

        setRow("Saldo Anterior", "$" + fmt(fondo.saldo_anterior));
        setRow("Adiciones", "$" + fmt(fondo.adiciones));
        setRow("Retiros", "$" + fmt(fondo.retiros));
        setRow("Rendimientos", "$" + fmt(fondo.rendimientos));
        setRow("Nuevo Saldo", "$" + fmt(fondo.nuevo_saldo));
        setRow("Unidades", fmt(fondo.unidades));
        setRow("Valor Unidad", "$" + fmt(fondo.valor_unidad_final));

        for (const auto& [periodo, val] : fondo.rentabilidades_historicas) {
            setRow("Rentab. " + periodo, fmt(val) + "%");
        }
        setRow("---", "---");
    }
    grid_fondos_->AutoSize();
}

void MainFrame::updateAnalisisDisplay(const AnalysisResult& analysis) {
    int row = 0;
    auto setRow = [&](const std::string& campo, const std::string& valor) {
        if (row >= grid_analisis_->GetNumberRows()) {
            grid_analisis_->AppendRows(1);
        }
        grid_analisis_->SetCellValue(row, 0, campo);
        grid_analisis_->SetCellValue(row, 1, valor);
        row++;
    };

    setRow("=== COMPOSICION ===", "");
    for (const auto& [key, val] : analysis.composicion_porcentaje) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << val << "%";
        setRow(key, oss.str());
    }

    auto fmtStat = [](const std::string& name, const StatSummary& s) -> std::string {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2)
            << "Media=" << s.mean << ", Desv=" << s.std_dev
            << ", Med=" << s.median << " [" << s.min_val << " - " << s.max_val << "]";
        return oss.str();
    };

    setRow("=== TASAS ===", "");
    setRow("Tasa Valoracion", fmtStat("", analysis.tasas_valoracion));
    setRow("Tasa Negociacion", fmtStat("", analysis.tasas_negociacion));
    setRow("Tasa Facial", fmtStat("", analysis.tasas_faciales));

    setRow("=== ALERTAS ===", "");
    for (const auto& a : analysis.alertas) {
        setRow("[" + a.type + "]", a.message);
    }

    grid_analisis_->AutoSize();
}

void MainFrame::showGraph(const std::string& image_path) {
    if (!fs::exists(image_path)) return;

    wxImage wx_img;
    if (wx_img.LoadFile(image_path)) {
        wxBitmap bmp(wx_img);
        panel_grafica_->SetBitmap(bmp);
        scroll_grafica_->SetVirtualSize(bmp.GetWidth(), bmp.GetHeight());
        scroll_grafica_->FitInside();
    }
}

void MainFrame::setProgress(int percentage) {
    progress_bar_->SetValue(percentage);
}

void MainFrame::logMessage(const std::string& msg) {
    txt_log_->AppendText(msg + "\n");
}
