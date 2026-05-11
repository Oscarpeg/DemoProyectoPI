#!/bin/bash
# Ejecutar desde la raíz del proyecto después de terminar de anotar.
# Junta imágenes y labels de oscar/ y santiago/ en all/.

set -e
cd "$(dirname "$0")/.."

DATASET="ai/dataset"
mkdir -p "$DATASET/all/images" "$DATASET/all/labels"

echo "Juntando imágenes y labels..."

for persona in oscar santiago; do
    imgs="$DATASET/$persona/images"
    lbls="$DATASET/$persona/labels"

    if [ ! -d "$imgs" ]; then
        echo "  [WARN] No existe $imgs — saltar"
        continue
    fi

    n_imgs=$(ls "$imgs" | grep -v classes.txt | wc -l)
    n_lbls=$(ls "$lbls" 2>/dev/null | wc -l)
    echo "  $persona: $n_imgs imágenes, $n_lbls labels"

    # Copiar imágenes
    find "$imgs" -maxdepth 1 \( -name "*.jpg" -o -name "*.jpeg" -o -name "*.png" \) \
        -exec cp {} "$DATASET/all/images/" \;

    # Copiar labels (solo .txt)
    if [ -d "$lbls" ]; then
        find "$lbls" -maxdepth 1 -name "*.txt" \
            -exec cp {} "$DATASET/all/labels/" \;
    fi
done

echo ""
echo "Resultado en ai/dataset/all/:"
echo "  Imágenes: $(ls $DATASET/all/images/ | wc -l)"
echo "  Labels:   $(ls $DATASET/all/labels/ | wc -l)"
