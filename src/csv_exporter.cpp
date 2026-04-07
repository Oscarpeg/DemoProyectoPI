#include "csv_exporter.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

std::string CSVExporter::escapeCSV(const std::string& field) {
    bool needs_quotes = false;
    for (char c : field) {
        if (c == ',' || c == '"' || c == '\n' || c == '\r') {
            needs_quotes = true;
            break;
        }
    }
    if (!needs_quotes) return field;

    std::string escaped = "\"";
    for (char c : field) {
        if (c == '"') escaped += "\"\"";
        else escaped += c;
    }
    escaped += "\"";
    return escaped;
}

std::string CSVExporter::doubleToStr(double value, int precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

bool CSVExporter::exportExtracto(const ExtractoCompleto& extracto, const std::string& path) {
    std::cout << "[CSVExporter] Exportando extracto a: " << path << std::endl;

    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "[CSVExporter] Error abriendo: " << path << std::endl;
        return false;
    }

    file << "Campo,Valor\n";
    file << escapeCSV("Fecha Extracto") << "," << escapeCSV(extracto.resumen.fecha_extracto) << "\n";
    file << escapeCSV("Nombre Cliente") << "," << escapeCSV(extracto.resumen.nombre_cliente) << "\n";
    file << escapeCSV("NIT") << "," << escapeCSV(extracto.resumen.nit) << "\n";
    file << escapeCSV("Direccion") << "," << escapeCSV(extracto.resumen.direccion) << "\n";
    file << escapeCSV("Ciudad") << "," << escapeCSV(extracto.resumen.ciudad) << "\n";
    file << escapeCSV("Telefono") << "," << escapeCSV(extracto.resumen.telefono) << "\n";
    file << escapeCSV("Asesor") << "," << escapeCSV(extracto.resumen.asesor) << "\n";

    file << "\nActivo,Valor\n";
    for (const auto& [key, val] : extracto.resumen.activos) {
        file << escapeCSV(key) << "," << doubleToStr(val) << "\n";
    }
    file << escapeCSV("Total Activos") << "," << doubleToStr(extracto.resumen.total_activos) << "\n";
    file << escapeCSV("Total Pasivos") << "," << doubleToStr(extracto.resumen.total_pasivos) << "\n";
    file << escapeCSV("Total Portafolio") << "," << doubleToStr(extracto.resumen.total_portafolio) << "\n";

    file.close();
    return true;
}

bool CSVExporter::exportRentaFija(const std::vector<InstrumentoRentaFija>& instrumentos,
                                   const std::string& path) {
    std::cout << "[CSVExporter] Exportando renta fija a: " << path << std::endl;

    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << "Nemotecnico,Fecha Emision,Fecha Vencimiento,Fecha Compra,"
         << "Tasa Facial (%),Periodicidad,Valor Nominal,"
         << "Tasa Negociacion (%),Tasa Valoracion (%),Valor Mercado\n";

    for (const auto& inst : instrumentos) {
        file << escapeCSV(inst.nemotecnico) << ","
             << escapeCSV(inst.fecha_emision) << ","
             << escapeCSV(inst.fecha_vencimiento) << ","
             << escapeCSV(inst.fecha_compra) << ","
             << doubleToStr(inst.tasa_facial) << ","
             << escapeCSV(inst.periodicidad) << ","
             << doubleToStr(inst.valor_nominal) << ","
             << doubleToStr(inst.tasa_negociacion) << ","
             << doubleToStr(inst.tasa_valoracion) << ","
             << doubleToStr(inst.valor_mercado) << "\n";
    }

    file.close();
    return true;
}

bool CSVExporter::exportFondos(const std::vector<FondoInversion>& fondos,
                                const std::string& path) {
    std::cout << "[CSVExporter] Exportando fondos a: " << path << std::endl;

    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << "Nombre Fondo,Codigo,Inversionista,Identificacion,"
         << "Saldo Anterior,Adiciones,Retiros,Rendimientos,Nuevo Saldo,Unidades,"
         << "Valor Unidad Final\n";

    for (const auto& fondo : fondos) {
        file << escapeCSV(fondo.nombre_fondo) << ","
             << escapeCSV(fondo.codigo) << ","
             << escapeCSV(fondo.nombre_inversionista) << ","
             << escapeCSV(fondo.identificacion) << ","
             << doubleToStr(fondo.saldo_anterior) << ","
             << doubleToStr(fondo.adiciones) << ","
             << doubleToStr(fondo.retiros) << ","
             << doubleToStr(fondo.rendimientos) << ","
             << doubleToStr(fondo.nuevo_saldo) << ","
             << doubleToStr(fondo.unidades, 6) << ","
             << doubleToStr(fondo.valor_unidad_final) << "\n";
    }

    // Rentabilidades historicas como seccion separada
    for (const auto& fondo : fondos) {
        if (!fondo.rentabilidades_historicas.empty()) {
            file << "\nRentabilidades Historicas - " << escapeCSV(fondo.nombre_fondo) << "\n";
            file << "Periodo,Rentabilidad (%)\n";
            for (const auto& [periodo, val] : fondo.rentabilidades_historicas) {
                file << escapeCSV(periodo) << "," << doubleToStr(val) << "\n";
            }
        }
    }

    file.close();
    return true;
}

bool CSVExporter::exportAnalisis(const AnalysisResult& analysis, const std::string& path) {
    std::cout << "[CSVExporter] Exportando analisis a: " << path << std::endl;

    std::ofstream file(path);
    if (!file.is_open()) return false;

    // Composicion
    file << "Composicion del Portafolio\n";
    file << "Activo,Porcentaje (%)\n";
    for (const auto& [key, val] : analysis.composicion_porcentaje) {
        file << escapeCSV(key) << "," << doubleToStr(val) << "\n";
    }

    // Estadisticas
    file << "\nEstadisticas de Tasas\n";
    file << "Metrica,Media,Desv.Est,Mediana,Min,Max,N\n";

    auto writeStat = [&](const std::string& name, const StatSummary& s) {
        file << escapeCSV(name) << ","
             << doubleToStr(s.mean) << ","
             << doubleToStr(s.std_dev) << ","
             << doubleToStr(s.median) << ","
             << doubleToStr(s.min_val) << ","
             << doubleToStr(s.max_val) << ","
             << s.count << "\n";
    };

    writeStat("Tasa Valoracion", analysis.tasas_valoracion);
    writeStat("Tasa Negociacion", analysis.tasas_negociacion);
    writeStat("Tasa Facial", analysis.tasas_faciales);
    writeStat("Valor Mercado", analysis.valores_mercado);

    // Alertas
    file << "\nAlertas\n";
    file << "Tipo,Mensaje\n";
    for (const auto& a : analysis.alertas) {
        file << escapeCSV(a.type) << "," << escapeCSV(a.message) << "\n";
    }

    file.close();
    return true;
}

std::vector<std::string> CSVExporter::exportAll(
    const ExtractoCompleto& extracto,
    const AnalysisResult& analysis,
    const std::string& output_dir) {

    std::cout << "[CSVExporter] Exportando todos los CSV a: " << output_dir << std::endl;
    std::vector<std::string> paths;

    std::string p1 = output_dir + "/extracto.csv";
    if (exportExtracto(extracto, p1)) paths.push_back(p1);

    std::string p2 = output_dir + "/renta_fija.csv";
    if (exportRentaFija(extracto.renta_fija, p2)) paths.push_back(p2);

    std::string p3 = output_dir + "/fondos.csv";
    if (exportFondos(extracto.fondos, p3)) paths.push_back(p3);

    std::string p4 = output_dir + "/analisis.csv";
    if (exportAnalisis(analysis, p4)) paths.push_back(p4);

    std::cout << "[CSVExporter] " << paths.size() << " archivos CSV exportados" << std::endl;
    return paths;
}
