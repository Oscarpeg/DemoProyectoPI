#ifndef PDF_PROCESSOR_H
#define PDF_PROCESSOR_H

#include <string>
#include <vector>

// Clase para convertir archivos PDF a imagenes PNG usando pdftoppm (Poppler)
class PDFProcessor {
public:
    // Establece la contrasena del PDF (user password)
    void setPassword(const std::string& password);

    // Convierte PDF a imagenes PNG (una por pagina) usando pdftoppm
    // Retorna vector con las rutas de las imagenes generadas
    std::vector<std::string> convertToImages(const std::string& pdf_path,
                                              const std::string& output_dir,
                                              int dpi = 300);

    // Retorna numero de paginas del PDF
    int getPageCount(const std::string& pdf_path);

    // Verifica si pdftoppm esta instalado
    static bool isPopplerAvailable();

    // Verifica si un PDF requiere contrasena
    static bool needsPassword(const std::string& pdf_path);

private:
    std::string password_;
};

#endif // PDF_PROCESSOR_H
