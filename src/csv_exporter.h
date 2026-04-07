#ifndef CSV_EXPORTER_H
#define CSV_EXPORTER_H

#include <string>
#include <vector>
#include "data_structurer.h"
#include "statistical_analyzer.h"

class CSVExporter {
public:
    bool exportExtracto(const ExtractoCompleto& extracto, const std::string& path);
    bool exportRentaFija(const std::vector<InstrumentoRentaFija>& instrumentos,
                         const std::string& path);
    bool exportFondos(const std::vector<FondoInversion>& fondos, const std::string& path);
    bool exportAnalisis(const AnalysisResult& analysis, const std::string& path);

    // Exportar todos los CSVs a un directorio
    std::vector<std::string> exportAll(const ExtractoCompleto& extracto,
                                        const AnalysisResult& analysis,
                                        const std::string& output_dir);

private:
    static std::string escapeCSV(const std::string& field);
    static std::string doubleToStr(double value, int precision = 2);
};

#endif // CSV_EXPORTER_H
