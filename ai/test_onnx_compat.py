#!/usr/bin/env python3
"""
Paso 0 — Validar compatibilidad OpenCV + YOLOv8 ONNX.

Descarga yolov8n.onnx (~12 MB), lo carga con cv2.dnn,
corre una inferencia dummy y verifica el shape de salida.

Resultado:
  OK   → usar YOLOv8n en el entrenamiento, activar cv::transpose en C++
  FAIL → cambiar a YOLOv5nu en todos los scripts, omitir transpose
"""
import sys
import os
import urllib.request

import cv2
import numpy as np

ONNX_PATH = "yolov8n_test.onnx"

def download_model():
    if os.path.exists(ONNX_PATH):
        print(f"Modelo ya disponible: {ONNX_PATH}")
        return
    print("Descargando yolov8n.pt y exportando a ONNX (primera vez ~30 seg)...")
    from ultralytics import YOLO
    model = YOLO("yolov8n.pt")
    model.export(format="onnx", opset=11, simplify=True, imgsz=640)
    # ultralytics guarda el .onnx junto al .pt
    import glob
    candidates = glob.glob(str(os.path.expanduser("~/.config/Ultralytics/**/*.onnx"),),
                           ) + glob.glob("yolov8n.onnx")
    exported = "yolov8n.onnx"
    if os.path.exists(exported):
        os.rename(exported, ONNX_PATH)
    else:
        raise FileNotFoundError("No se encontro el .onnx exportado")
    print(f"  Exportado y guardado en {ONNX_PATH}")

def test_compat():
    print(f"\nOpenCV version: {cv2.__version__}")

    # 1. Cargar modelo
    print("Cargando modelo con cv2.dnn.readNetFromONNX...")
    try:
        net = cv2.dnn.readNetFromONNX(ONNX_PATH)
    except Exception as e:
        print(f"FALLO al cargar: {e}")
        return False

    net.setPreferableBackend(cv2.dnn.DNN_BACKEND_OPENCV)
    net.setPreferableTarget(cv2.dnn.DNN_TARGET_CPU)

    # 2. Inferencia con imagen dummy
    print("Corriendo inferencia dummy (640x640)...")
    dummy_img = np.zeros((640, 640, 3), dtype=np.uint8)
    blob = cv2.dnn.blobFromImage(dummy_img, 1.0/255.0, (640, 640),
                                  swapRB=True, crop=False)
    net.setInput(blob)
    try:
        out = net.forward()
    except Exception as e:
        print(f"FALLO en forward(): {e}")
        return False

    # 3. Analizar shape de salida
    print(f"Output shape: {out.shape}")

    # YOLOv8 ONNX: (1, 4+nc, 8400)  — las coords/clases van en dim[1]
    #   COCO (80 clases)  → (1, 84, 8400)
    #   Nuestro modelo (1 clase) → (1,  5, 8400)  necesita transpose en C++
    # YOLOv5 ONNX: (1, 8400, 5+nc) — los anchors van en dim[1]
    #   COCO (80 clases)  → (1, 8400, 85)
    #   Nuestro modelo (1 clase) → (1, 8400,  6)  directo en C++
    if len(out.shape) == 3:
        _, d1, d2 = out.shape
        if d1 < d2:
            # dim1 es (4+nc) y dim2 son los anchors (8400)
            # → YOLOv8 layout → necesita transpose en C++
            nc = d1 - 4
            print(f"Layout detectado: YOLOv8  [1, {d1}, {d2}]  (nc={nc} clases)")
            print(">>> En C++: cv::transpose(out, out) ES NECESARIO")
            layout = "v8"
        else:
            # dim1 son los anchors y dim2 es (4+nc)
            # → YOLOv5 layout → directo en C++
            nc = d2 - 5  # YOLOv5 tiene conf + nc
            print(f"Layout detectado: YOLOv5  [1, {d1}, {d2}]  (nc={nc} clases)")
            print(">>> En C++: cv::transpose NO es necesario")
            layout = "v5"
    else:
        print(f"Layout DESCONOCIDO: {out.shape}  — revisar manualmente")
        layout = "unknown"

    return layout

def main():
    try:
        download_model()
    except Exception as e:
        print(f"Error descargando modelo: {e}")
        print("Verifica conexion a internet y vuelve a intentar.")
        sys.exit(1)

    result = test_compat()

    print("\n" + "="*50)
    if result == "v8":
        print("RESULTADO: OK — YOLOv8n compatible con OpenCV", cv2.__version__)
        print("Usar en entrenamiento : yolov8n.pt")
        print("Usar en C++ (transpose): SI")
    elif result == "v5":
        print("RESULTADO: OK — Solo compatible con layout YOLOv5")
        print("Usar en entrenamiento : yolov5nu.pt  (cambiar en train_colab.ipynb)")
        print("Usar en C++ (transpose): NO")
    else:
        print("RESULTADO: FALLO — cv2.dnn no puede correr este modelo.")
        print("Opciones:")
        print("  1. Compilar OpenCV >= 4.7 desde fuente")
        print("  2. Usar YOLOv5nu.pt y verificar de nuevo")
        print("  3. Usar ONNX Runtime en C++ en lugar de cv2.dnn")

    print("="*50)

    # Limpiar
    if os.path.exists(ONNX_PATH):
        os.remove(ONNX_PATH)
        print(f"\nArchivo temporal {ONNX_PATH} eliminado.")

    sys.exit(0 if result in ("v8", "v5") else 1)

if __name__ == "__main__":
    main()
