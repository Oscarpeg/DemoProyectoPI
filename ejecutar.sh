#!/bin/bash
# ============================================================
# ejecutar.sh - Script para ejecutar ProyectoPI con Docker
# ============================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Colores
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN} ProyectoPI - Sistema de Extractos${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# Crear carpetas si no existen
mkdir -p pdfs resultados

# Verificar Docker
if ! command -v docker &> /dev/null; then
    echo -e "${RED}ERROR: Docker no está instalado.${NC}"
    echo "Instalar con: sudo apt install docker.io docker-compose"
    exit 1
fi

# Construir imagen si no existe
if ! docker images | grep -q "proyectopi"; then
    echo -e "${YELLOW}[1/2] Construyendo imagen Docker (primera vez, ~10-15 min)...${NC}"
    docker compose build
    echo -e "${GREEN}Imagen construida exitosamente.${NC}"
else
    echo -e "${GREEN}[1/2] Imagen Docker encontrada.${NC}"
fi

echo ""
echo -e "${YELLOW}¿Qué deseas hacer?${NC}"
echo "  1) Procesar un PDF (modo consola)"
echo "  2) Abrir interfaz gráfica (GUI)"
echo "  3) Reconstruir imagen Docker"
echo ""
read -p "Opción [1/2/3]: " opcion

case $opcion in
    1)
        # Listar PDFs disponibles
        echo ""
        echo "PDFs en la carpeta ./pdfs/:"
        ls pdfs/*.pdf 2>/dev/null || echo "  (vacía - coloca tus PDFs en la carpeta 'pdfs/')"
        echo ""
        read -p "Ruta del PDF (o nombre si está en pdfs/): " pdf_path

        # Si solo puso el nombre, buscar en pdfs/
        if [[ ! "$pdf_path" == /* ]] && [[ -f "pdfs/$pdf_path" ]]; then
            pdf_path="pdfs/$pdf_path"
        fi

        if [[ ! -f "$pdf_path" ]]; then
            echo -e "${RED}ERROR: Archivo no encontrado: $pdf_path${NC}"
            exit 1
        fi

        # Copiar PDF a carpeta pdfs/ si no está ahí
        pdf_name=$(basename "$pdf_path")
        if [[ ! -f "pdfs/$pdf_name" ]]; then
            cp "$pdf_path" "pdfs/$pdf_name"
        fi

        read -p "¿El PDF tiene contraseña? (s/N): " tiene_pw
        PW_FLAG=""
        if [[ "$tiene_pw" == "s" || "$tiene_pw" == "S" ]]; then
            read -sp "Contraseña: " password
            echo ""
            PW_FLAG="--pw $password"
        fi

        echo ""
        echo -e "${YELLOW}[2/2] Procesando PDF...${NC}"
        docker compose run --rm proyecto "./ProyectoPI" "pdfs/$pdf_name" $PW_FLAG

        echo ""
        echo -e "${GREEN}========================================${NC}"
        echo -e "${GREEN} Resultados en: ./resultados/${NC}"
        echo -e "${GREEN}========================================${NC}"
        ls -la resultados/ 2>/dev/null
        ;;

    2)
        echo ""
        echo -e "${YELLOW}[2/2] Abriendo interfaz gráfica...${NC}"
        # Permitir conexiones X11 locales
        xhost +local:docker 2>/dev/null || true
        docker compose up proyecto-gui
        ;;

    3)
        echo -e "${YELLOW}Reconstruyendo imagen...${NC}"
        docker compose build --no-cache
        echo -e "${GREEN}Imagen reconstruida.${NC}"
        ;;

    *)
        echo -e "${RED}Opción no válida.${NC}"
        exit 1
        ;;
esac
