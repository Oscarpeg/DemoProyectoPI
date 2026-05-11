#!/usr/bin/env python3
"""
Paso 1.6 — Validar dataset antes de entrenar.
Uso: python3 ai/validate_dataset.py
"""
import sys
from pathlib import Path

DATASET  = Path("ai/dataset/all")
IMG_EXTS = {".jpg", ".jpeg", ".png"}

# Clases válidas según dataset.yaml
VALID_CLASSES = {
    0: "tabla_renta",      1: "tabla_portafolio", 2: "titulo_renta",
    3: "tabla_fondo",      4: "titulo_fondo",      5: "titulo_portafolio",
    6: "titulo_saldos",    7: "tabla_saldos",      8: "tabla_fondo_vista",
    9: "titulo_fondo_vista", 10: "titulo_fondo_sumar", 11: "tabla_fondo_sumar"
}

images_dir = DATASET / "images"
labels_dir = DATASET / "labels"

errors   = []
warnings = []

imgs = {p.stem: p for p in images_dir.iterdir() if p.suffix.lower() in IMG_EXTS}
lbls = {p.stem: p for p in labels_dir.glob("*.txt")}

# 1. Huérfanos
for stem in set(imgs) - set(lbls):
    warnings.append(f"Imagen sin label (se asume sin tabla): {stem}")
for stem in set(lbls) - set(imgs):
    errors.append(f"Label sin imagen: {stem}")

# 2. Contenido de cada label
n_con_annot  = 0
n_sin_annot  = 0
class_counts = {k: 0 for k in VALID_CLASSES}

for stem, txt in lbls.items():
    lines = [l.strip() for l in txt.read_text().splitlines() if l.strip()]
    if not lines:
        n_sin_annot += 1
        continue
    n_con_annot += 1
    for i, line in enumerate(lines, 1):
        parts = line.split()
        if len(parts) != 5:
            errors.append(f"{txt.name} línea {i}: esperaba 5 campos, tiene {len(parts)} → '{line}'")
            continue
        try:
            cls  = int(parts[0])
            vals = [float(p) for p in parts[1:]]
        except ValueError:
            errors.append(f"{txt.name} línea {i}: valores no numéricos → '{line}'")
            continue
        if cls not in VALID_CLASSES:
            errors.append(f"{txt.name} línea {i}: clase {cls} no válida (rango 0-11)")
        else:
            class_counts[cls] += 1
        if any(v < 0 or v > 1 for v in vals):
            errors.append(f"{txt.name} línea {i}: coordenadas fuera de [0,1] → {vals}")
        cx, cy, w, h = vals
        if w <= 0 or h <= 0:
            errors.append(f"{txt.name} línea {i}: w o h es 0 o negativo → w={w} h={h}")

# Imprimir
print(f"\nDataset: {len(imgs)} imágenes | {len(lbls)} labels")
print(f"  Con anotaciones: {n_con_annot}")
print(f"  Sin anotaciones: {n_sin_annot} (páginas sin elementos)")
print(f"\n  Anotaciones por clase:")
for cls_id, name in VALID_CLASSES.items():
    count = class_counts[cls_id]
    bar   = "█" * min(count, 30)
    print(f"    {cls_id:2d} {name:22s}: {count:4d}  {bar}")

if warnings:
    print(f"\n{len(warnings)} AVISOS:")
    for w in warnings:
        print(f"  ⚠  {w}")

if errors:
    print(f"\n{len(errors)} ERRORES — corregir antes de entrenar:")
    for e in errors:
        print(f"  ✗  {e}")
    sys.exit(1)
else:
    print("\n✓ Dataset válido — listo para el split.")
