#!/usr/bin/env python3
"""
Script auxiliar para OCR usando EasyOCR.
Dos modos de operacion:

1. MODO SERVIDOR (--server): Se mantiene corriendo, recibe rutas por stdin,
   responde JSON por stdout. El modelo se carga UNA sola vez.

2. MODO UNICO (ruta_imagen): Procesa una imagen y termina.
   (Lento porque carga el modelo cada vez - usar solo para pruebas)
"""
import sys
import json
import os

def process_image(reader, image_path, region=None):
    """Procesa una imagen con EasyOCR y retorna resultados como lista de dicts."""
    import cv2

    img = cv2.imread(image_path)
    if img is None:
        return [{"error": f"No se pudo leer: {image_path}"}]

    # Recortar si se especifica region (x,y,w,h)
    if region and len(region) == 4:
        x, y, w, h = region
        img = img[y:y+h, x:x+w]

    results = reader.readtext(img)

    output = []
    for (bbox, text, confidence) in results:
        output.append({
            "text": text,
            "confidence": round(float(confidence), 4),
            "bbox": {
                "top_left": [int(bbox[0][0]), int(bbox[0][1])],
                "bottom_right": [int(bbox[2][0]), int(bbox[2][1])]
            }
        })
    return output


def run_server():
    """Modo servidor: carga modelo una vez, procesa peticiones por stdin."""
    import easyocr

    # Cargar modelo UNA sola vez
    sys.stderr.write("[OCR Server] Cargando modelo EasyOCR (espanol)...\n")
    sys.stderr.flush()
    reader = easyocr.Reader(['es'], gpu=False, verbose=False)

    # Indicar que esta listo
    print(json.dumps({"status": "ready"}), flush=True)
    sys.stderr.write("[OCR Server] Modelo cargado. Esperando peticiones...\n")
    sys.stderr.flush()

    # Procesar peticiones linea por linea
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue

        if line == "EXIT" or line == "QUIT":
            sys.stderr.write("[OCR Server] Cerrando.\n")
            break

        try:
            request = json.loads(line)
        except json.JSONDecodeError:
            # Si no es JSON, asumir que es una ruta de imagen simple
            request = {"image": line}

        image_path = request.get("image", "")
        region = request.get("region", None)  # [x, y, w, h]
        batch = request.get("batch", None)    # lista de {"image":..., "region":...}

        if batch:
            # Modo batch: procesar multiples imagenes de una vez
            results = []
            for item in batch:
                img_path = item.get("image", "")
                img_region = item.get("region", None)
                if os.path.exists(img_path):
                    r = process_image(reader, img_path, img_region)
                    results.append(r)
                else:
                    results.append([{"error": f"No encontrado: {img_path}"}])
            print(json.dumps(results, ensure_ascii=False), flush=True)

        elif image_path and os.path.exists(image_path):
            result = process_image(reader, image_path, region)
            print(json.dumps(result, ensure_ascii=False), flush=True)

        else:
            print(json.dumps([{"error": f"Archivo no encontrado: {image_path}"}]), flush=True)


def run_single(image_path, region=None):
    """Modo unico: carga modelo, procesa una imagen y termina."""
    import easyocr

    reader = easyocr.Reader(['es'], gpu=False, verbose=False)
    result = process_image(reader, image_path, region)
    print(json.dumps(result, ensure_ascii=False))


def main():
    if len(sys.argv) < 2:
        print(json.dumps({"error": "Uso: python ocr_helper.py --server | <ruta_imagen>"}))
        sys.exit(1)

    # Modo servidor
    if sys.argv[1] == "--server":
        run_server()
        return

    # Modo unico (legacy)
    image_path = sys.argv[1]

    if not os.path.exists(image_path):
        print(json.dumps({"error": f"Archivo no encontrado: {image_path}"}))
        sys.exit(1)

    region = None
    if len(sys.argv) >= 4 and sys.argv[2] == "--region":
        try:
            parts = sys.argv[3].split(",")
            region = tuple(int(p) for p in parts)
        except ValueError:
            print(json.dumps({"error": "Formato de region invalido. Usar: x,y,w,h"}))
            sys.exit(1)

    run_single(image_path, region)


if __name__ == "__main__":
    main()
