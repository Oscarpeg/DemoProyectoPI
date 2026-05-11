# ProyectitoResumen — Comprensión Completa del Proyecto

## ¿Qué es este proyecto?

Pipeline en **C++17** que toma un extracto de inversión (PDF, JPG, PNG, foto de celular) de la firma **Acciones & Valores S.A.** (o del template estándar v1 generado por las herramientas del repo) y produce:
- 6 gráficas financieras PNG
- 4 CSVs con los datos estructurados
- 1 JSON conforme al schema `extracto_v1.schema.json`

**Stack:** C++17 + CMake + OpenCV 4 + wxWidgets + Tesseract OCR + nlohmann-json. Todo el procesamiento es 100% C++, sin Python en runtime (Python solo en herramientas de generación de datos de prueba).

**Despliegue:** Docker (recomendado). Sin GPU requerida. CPU-only.

---

## Modos de ejecución (`src/main.cpp`)

| Modo | Cómo se activa | Qué hace |
|---|---|---|
| **Consola** | `./proyecto archivo.pdf` o `./proyecto imagen.jpg` | Pipeline completo: PDF→imagen→OCR→análisis→gráficas→CSV/JSON |
| **--from-json** | `./proyecto --from-json extracto.json` | Salta OCR, solo análisis+gráficas+CSV desde JSON ya extraído |
| **GUI** | `./proyecto` (sin argumentos) | Interfaz wxWidgets con tabs, botones, preview de gráficas |

**Variables de entorno útiles:**
- `PROYECTOPI_DEBUG_PREPROC=1` → guarda etapas intermedias del preprocesamiento de imagen
- `PROYECTOPI_DEBUG_OCR=1` → dump de todos los bloques OCR detectados

---

## Pipeline completo (modo consola, 8 pasos)

```
[1] Detectar tipo de archivo (PDF / imagen)
[2] Si PDF: convertir a PNG via pdftoppm (Poppler) a 300 DPI
[3] Inicializar Tesseract OCR (verifica disponibilidad + idioma "spa")
[4] Por cada página:
    a) Preprocesar imagen (enhanceForOCR)
    b) Intentar StandardParser (template v1). Si matchea → parsear y continuar.
    c) Si no: clasificar página con PageClassifier
    d) Según tipo: RESUMEN / DETALLE_RENTA_FIJA / EXTRACTO_FONDOS / DESCONOCIDO
    e) Extraer datos con DataStructurer
[5] Validar datos extraídos
[6] Análisis estadístico + métricas avanzadas + serie temporal histórica
[7] Generar 6 gráficas PNG con OpenCV
[8] Exportar CSVs y JSON
```

---

## Módulos del código fuente

### `pdf_processor.cpp/.h`
Convierte PDF a imágenes PNG usando `pdftoppm` (subprocess). Soporta PDFs con contraseña (`-upw`). Detecta si Poppler está instalado. Devuelve vector de rutas PNG ordenadas.

### `image_preprocessor.cpp/.h`
**El módulo más complejo.** Pipeline de mejora de imagen para OCR. Todo con OpenCV.

**Función principal:** `enhanceForOCR(input, report)`

**Pasos en orden:**
1. Evaluar nitidez (varianza del Laplaciano). Si < 50 → warning.
2. **Corrección de orientación gruesa** (0/90/180/270°):
   - Primero intenta Tesseract OSD (`--psm 0`) → autoritativo, confianza >= 0.5
   - Fallback: heurística Hough (cuenta líneas horizontales; ganancia mínima 1.5x para aplicar rotación)
3. **Detección de documento** (para fotos de celular):
   - Binarización Otsu → Canny → findContours → convexHull → approxPolyDP (4 vértices)
   - Score de rectangularidad (ángulos ~90°). Si score > 0.6 → corregir perspectiva (`warpPerspective`)
4. **Rotación fina** con HoughLinesP (mediana de ángulos < 30°). Solo aplica si > 0.2°.
5. Grayscale → sharpen si borroso (unsharp mask) → CLAHE → median blur
6. **Upscaling adaptativo**: si lado mínimo < 1500px → escalar a 2500px con Lanczos4 + unsharp
7. **Binarización Sauvola** solo para imágenes pequeñas (pre_min_side < 1500): ventana 25px, k=0.2

**Struct `PreprocessReport`:** documenta todo lo que hizo el pipeline (document_detected, rotation, sharpness, steps_applied, warning).

### `ocr_extractor.cpp/.h`
Invoca Tesseract via CLI subprocess, no por API. Formato de salida: TSV.

**`extractAll(image)`:** Prueba 3 PSM (6, 4, 11) y elige el de mayor score (caracteres alfanuméricos en bloques de longitud ≥ 3 con ≥ 60% alfanumérico). Devuelve el mejor.

**`parseTSV(tsv)`:** Parsea el TSV de Tesseract (12 columnas, level=5 = palabras). Fusiona palabras adyacentes muy juntas (gap < altura/2 en la misma línea) para reconstruir números como "1.000.000,00".

**`extractFromGrid(img, grid)`:** Una sola llamada OCR sobre imagen completa, luego asigna palabras a celdas por intersección de bbox (mucho más rápido que N llamadas por celda).

**Struct `OCRResult`:** `{text, confidence, bbox(cv::Rect)}`.

Config: `lang_="spa"`, `psm_=6`, `oem_=3` (LSTM + legacy).

### `page_classifier.cpp/.h`
Clasifica páginas del PDF de **Acciones & Valores S.A.** (layout legacy):

| Tipo | Criterio de clasificación |
|---|---|
| `RESUMEN` | Portrait + keywords "extracto inversiones" + ("nombre" o "nit" o "total") en mitad |
| `DETALLE_RENTA_FIJA` | **Landscape** (cols > rows) → automático |
| `EXTRACTO_FONDOS` | Keywords "extracto"+"fondo" o "fondo"+"inversion" en header (25% superior) |
| `DESCONOCIDO` | "defensor"/"consumidor"/"superintendencia" → página informativa, se omite |

Dependencia: recibe referencia a `OCRExtractor` para hacer OCR de regiones.

### `table_detector.cpp/.h`
Detecta tablas con bordes visibles usando morfología OpenCV.

**`detectAllTables(input)`:** Binariza con Otsu → detecta líneas H y V con `morphologyEx(MORPH_OPEN)` con kernels rectangulares → OR de líneas → dilata → findContours → para cada contorno grande, llama `buildGrid()`.

**`buildGrid(binary)`:** Encuentra intersecciones de líneas H∩V → clusteriza posiciones X e Y → construye `TableGrid` con `cells[][]` de `cv::Rect`.

**Nota importante:** En el pipeline principal, para `DETALLE_RENTA_FIJA` se usa **`DataStructurer::detectTablesFromOCR()`** (sin morfología) como método primario, y el `TableDetector` morfológico solo como fallback si no hay tablas con bordes.

### `data_structurer.cpp/.h`
**El módulo central de extracción de datos.** Convierte OCR crudo → structs tipados.

**Structs de datos:**
- `ResumenPortafolio`: nombre_cliente, nit, activos{}, total_portafolio, fecha_extracto, etc.
- `InstrumentoRentaFija`: nemotecnico, fecha_emision/vencimiento/compra, tasa_facial/negociacion/valoracion, valor_nominal/mercado, periodicidad
- `SaldoEfectivo`: cuenta, saldo_disponible/canje/bloqueado/total
- `FondoInversion`: nombre_fondo, saldo_anterior/adiciones/retiros/rendimientos/nuevo_saldo, rentabilidades_historicas{}, unidades, valor_unidad_final
- `Transaction` + `CuentaTransacciones`: para extractos bancarios genéricos
- `ExtractoCompleto`: contiene todos los anteriores. Tiene `saveToFile(path)` que serializa a JSON.

**Parseo de números colombianos:**
- `parseColombianNumber("4.382.067.330,00")` → elimina puntos de miles, coma→punto decimal
- `parseColombianPercentage("11,66 % E.A.")` → elimina sufijos EA/MV, normaliza

**`groupIntoLines(ocr_data, y_tolerance=20)`:** Agrupa bloques OCR por centro Y. Cada `VisualLine` tiene `blocks[]` ordenados por X, y métodos `fullText()`, `moneyValue()`, `percentValue()`.

**`moneyValue()`:** Prioridad: bloque con `$` + dígito → número con 2+ puntos (miles) → número con separador (no %) → cualquier número con separador.

**`detectTablesFromOCR(ocr_data)`:** Detecta tablas desde OCR sin morfología. Agrupa en líneas → detecta gaps verticales > 2.5x el promedio → divide en regiones → clasifica cada región como tabla si ≥50% de líneas tienen ≥60% del máximo de bloques y max_blocks ≥ 3. Tipos: "renta_fija", "saldos", "fic_resumen", "unknown".

**`findNearestMoneyRight(ocr_data, label_block, y_tol=40)`:** Emparejamiento espacial para extraer valores. Busca el bloque monetario más cercano a la derecha del label dentro de ±40px vertical.

**Métodos de construcción:**
- `buildResumen(ocr_data)`: parsea por ":" separador + búsqueda de activos por etiqueta + emparejamiento espacial
- `buildRentaFija(table_data)`: mapeo directo de columnas 0-9
- `buildRentaFijaFromLines(ocr_data)`: fallback por líneas visuales, detecta patrones CDT/fechas/tasas
- `buildSaldosEfectivo(table_data)`: 5 columnas (cuenta, disponible, canje, bloqueado, total)
- `buildFondo(ocr_data)`: labels ":" + emparejamiento espacial para movimientos

### `standard_parser.cpp/.h`
Parser para el **Template Estándar v1** (generado por `tools/generate_test_extracts.py`). Layout fijo y conocido de una sola página.

**Detección del template:** Score multi-señal. Busca fuzzy (Levenshtein) al menos 3 de: "Cliente:", "NIT/CC:", "Periodo:", "Direccion:", "Ciudad:", "Asesor:", secciones "RESUMEN PORTAFOLIO", "RENTA FIJA". Si score ≥ 3 → es el template estándar.

**`parseAll(ocr_data, out)`:** Parser semántico. Para renta fija usa regex para extraer fechas (`DD-MM-YYYY`), porcentajes, y montos con `$`. Clasifica por tipo de contenido, no por posición de columna. Robusto a headers garbled (texto blanco sobre fondo oscuro que Tesseract pierde).

Implementa **distancia de Levenshtein** para fuzzy matching de labels cuando OCR confunde caracteres. También reconstruye números fragmentados (espacios entre dígitos → puntos).

### `statistical_analyzer.cpp/.h`
Análisis estadístico y financiero. Sin IA, solo fórmulas matemáticas.

**`analyzeTimeSeries(extractos[])`:** Análisis del último extracto + carga de datos históricos de `data/historico_inicial.csv` + métricas avanzadas.

**`AnalysisResult`:** composicion_porcentaje{}, tasas_valoracion/negociacion/faciales (StatSummary), rendimientos_fic{}, alertas[], serie_temporal[], avanzadas (AdvancedMetrics).

**`AdvancedMetrics` (todas calculadas explícitamente):**
- `yield_ponderado_pct` = Σ(wᵢ × tasa_valoraciónᵢ)
- `duracion_macaulay_anos` = Σ(wᵢ × tᵢ en años), solo instrumentos VIVOS
- `duracion_modificada` = Macaulay / (1 + yield)
- `hhi` = Σ(wᵢ²) × 10000 (Herfindahl-Hirschman)
- `top1/3/5_exposure_pct`
- `vencimientos_buckets{}` por rangos configurables (dias_corto=90, dias_medio=365, dias_largo=1095)
- `sharpe_fic` = (r - rf) / σ
- `max_drawdown_pct` = peor caída desde máximo (como porcentaje negativo)
- `retorno_real_pct` = (1+nominal)/(1+inflacion) - 1 (Fisher)
- `skewness_tasas`, `kurtosis_tasas` (exceso sobre normal), `cv_tasas`, `ci95_lower/upper`

**Helpers estáticos:** `skewness()`, `kurtosis()`, `maxDrawdown()`, `sharpeRatio()`, `realReturnFisher()`, `diasEntreFechasISO()`.

**Alertas automáticas:** outliers en tasas (k-sigma), concentración > 80%, rendimiento FIC negativo, tasa facial < 5%.

### `graph_generator.cpp/.h`
Genera 6 gráficas PNG con **solo OpenCV** (sin matplotlib ni libs externas).

| # | Nombre | Qué muestra |
|---|---|---|
| 01 | `yield_curve.png` | Scatter de tasa_valoracion vs años a vencimiento. Línea conectando puntos ordenados. Regresión solo si n ≥ 3. Excluye instrumentos vencidos. |
| 02 | `maturity_ladder.png` | Barras de monto COP por bucket de vencimiento. Footer: días promedio ponderado. |
| 03 | `pareto_concentracion.png` | Barras (% por holding) ordenadas mayor→menor + línea acumulada. Umbral 80%. HHI en subtítulo. |
| 04 | `boxplot_tasas.png` | Boxplot de tasas de valoración (Q1/Q3/mediana/whiskers/outliers/IC95%). Si n < 5: scatter individual. |
| 05 | `drawdown.png` | Línea de portafolio + área roja entre máximos y valores. Max drawdown en subtítulo. |
| 06 | `dashboard_kpis.png` | 8 tarjetas con KPIs ejecutivos: total portafolio, yield ponderado, duración modificada, HHI, top-3 exposure, Sharpe FIC, max drawdown, retorno real. |

**Colores (BGR OpenCV):** verde (96,174,39), rojo (60,76,231), gris (166,165,149), azul (185,128,41), naranja (18,156,243).

### `csv_exporter.cpp/.h`
Exporta 4 archivos CSV:
- `extracto.csv`: resumen del portafolio (activos, total)
- `renta_fija.csv`: todos los instrumentos RF
- `fondos.csv`: datos de fondos de inversión
- `analisis.csv`: métricas estadísticas (composición %, tasas stats, alertas)

### `extracto_loader.cpp/.h`
Carga un `ExtractoCompleto` desde un JSON (schema v1). Usado por el modo `--from-json`.

### `ui_manager.cpp/.h`
Interfaz gráfica con **wxWidgets**. 6 tabs:
1. **Resumen**: grid con datos del cliente y activos
2. **Renta Fija**: grid con instrumentos
3. **Fondos**: grid con fondos
4. **Gráficas**: listbox de gráficas + preview de imagen
5. **Análisis**: grid con métricas estadísticas
6. **Preview**: preview de la imagen preprocesada

Procesa en un hilo secundario (para no bloquear la UI) y notifica al hilo principal con `wxCommandEvent`.

### `finance_config.cpp/.h`
Configura parámetros financieros desde `data/finance_config.json`. Si no existe el archivo, usa defaults:
- `tasa_libre_riesgo = 9.25%` (IBR/DTF)
- `inflacion_anual = 4.80%` (IPC)
- `benchmark_rendto = 11%`
- `hhi_bajo = 1500`, `hhi_alto = 2500`
- Buckets: corto=90d, medio=365d, largo=1095d

---

## Datos y archivos de soporte

### `data/historico_inicial.csv`
Serie temporal histórica de portafolios (columnas: total_portafolio, renta_fija, fic, efectivo). Se carga para calcular drawdown, crecimiento y Sharpe sobre la serie completa.

### `schema/extracto_v1.schema.json`
JSON Schema del contrato de datos. Define estructura de `ExtractoCompleto` para validación externa.

### `pdfs/examples/`
10 PDFs de ejemplo (extracto_01_carlos.pdf ... extracto_10_juan.pdf) + fotos de celular.

### `tools/`
- `generate_test_extracts.py`: genera PDFs sintéticos del Template Estándar v1 con datos aleatorios
- `generate_image_variants.py`: genera variantes de imagen (rotaciones, ruido, compresión) para pruebas

---

## Flujo de datos: qué struct sale de cada módulo

```
PDF/imagen
   ↓ PDFProcessor → vector<string> (rutas PNG)
   ↓ ImagePreprocessor → cv::Mat (imagen mejorada) + PreprocessReport
   ↓ OCRExtractor → vector<OCRResult> {text, confidence, bbox}
   ↓ StandardParser O (PageClassifier + DataStructurer) → ExtractoCompleto
   ↓ StatisticalAnalyzer → AnalysisResult (incluye AdvancedMetrics)
   ↓ GraphGenerator → vector<string> (rutas PNG de gráficas)
   ↓ CSVExporter → vector<string> (rutas CSV)
   ↓ ExtractoCompleto::saveToFile() → JSON
```

---

## Dos parsers: Legacy vs Estándar

El proyecto soporta **dos formatos de entrada**:

**Legacy (Acciones & Valores S.A.):**
- PDF de múltiples páginas (portrait + landscape)
- Página 1: Resumen (portrait)
- Página 2: Defensor consumidor (se omite)
- Página 3: Tablas con bordes (landscape) — Saldos, Renta Fija
- Página 4: Extracto de Fondos (portrait)
- Parsing: `PageClassifier` + `DataStructurer` + `TableDetector`

**Estándar v1 (generado por tools/):**
- Single-page, layout fijo y conocido
- Detección: si OCR contiene marcadores suficientes
- Parsing: `StandardParser` con regex semánticos y Levenshtein fuzzy

El main intenta `StandardParser` primero; si falla, cae al pipeline legacy.

---

## Lo que NO tiene el proyecto (estado actual)

- **Cero IA/ML real:** No hay ningún modelo entrenado. Todo es reglas, morfología OpenCV, y OCR clásico (Tesseract).
- `PageClassifier` → keyword matching, no clasificador ML
- `TableDetector` → morfología geométrica, no deep learning
- `OCRExtractor` → Tesseract clásico, no TrOCR ni PaddleOCR

---

## Dónde tocar para agregar IA (contexto para futuros cambios)

Para cumplir la rúbrica de IA del proyecto, el punto de entrada más limpio es:

1. **`TableDetector` / `detectAllTables()`:** Reemplazar o augmentar con un modelo ONNX cargado via `cv::dnn::readNetFromONNX()` (OpenCV ya es dependencia). YOLOv8-nano (6 MB, CPU-only) es el candidato natural — detecta regiones de tabla como bounding boxes. Integrar en `src/table_detector.cpp/.h`.

2. **`PageClassifier::classify()`:** Actualmente hace keyword matching. Se podría reemplazar con un clasificador entrenado (SVM, Random Forest, o pequeña CNN) sobre features de texto/imagen. Integrar como método alternativo.

3. **El modelo ONNX se carga así:**
```cpp
cv::dnn::Net net = cv::dnn::readNetFromONNX("models/table_detector.onnx");
cv::Mat blob = cv::dnn::blobFromImage(img, 1/255.0, cv::Size(640,640));
net.setInput(blob);
cv::Mat out = net.forward();
```

4. **Entrenamiento:** Dataset = los 10 PDFs de `pdfs/examples/` renderizados + variantes de `tools/generate_image_variants.py`. Anotar bounding boxes de regiones de tabla con LabelImg. Fine-tune desde COCO preentrenado.

5. **Métricas:** mAP@0.5, mAP@0.5:0.95, IoU por imagen. Probar con foto de WhatsApp ya en `pdfs/examples/`.
