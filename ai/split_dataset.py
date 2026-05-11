#!/usr/bin/env python3
"""
Paso 1.8 — Split 70/15/15 del dataset anotado.

Lee de:   ai/dataset/all/images/  y  ai/dataset/all/labels/
Genera:   ai/dataset/images/{train,val,test}
          ai/dataset/labels/{train,val,test}
          ai/dataset/dataset.yaml

Las fotos reales de celular van SIEMPRE al test set (son el caso mas dificil
y necesitamos medir el rendimiento exactamente en esas condiciones).

Uso:
    python3 ai/split_dataset.py
"""
import random
import shutil
from pathlib import Path

random.seed(42)

# ── Rutas ──────────────────────────────────────────────────────────────────
DATASET    = Path("ai/dataset")
ALL_IMGS   = DATASET / "all" / "images"
ALL_LABELS = DATASET / "all" / "labels"
IMG_EXTS   = {".jpg", ".jpeg", ".png"}

# Fotos reales de celular → siempre al test set
CELULAR_PREFIXES = ("foto", "Foto", "impresion", "Impresion", "whatsapp", "WhatsApp")

# ── Verificar que exista el directorio fuente ───────────────────────────────
if not ALL_IMGS.exists():
    print("ERROR: ai/dataset/all/images/ no existe.")
    print("Ejecuta primero:  bash ai/merge_labels.sh")
    raise SystemExit(1)

# ── Separar fotos de celular del resto ─────────────────────────────────────
all_imgs = sorted(p for p in ALL_IMGS.iterdir() if p.suffix.lower() in IMG_EXTS)

celular = [p for p in all_imgs if p.name.startswith(CELULAR_PREFIXES)]
resto   = [p for p in all_imgs if p not in celular]

random.shuffle(resto)

n       = len(resto)
n_train = int(n * 0.70)
n_val   = int(n * 0.15)

train_imgs = resto[:n_train]
val_imgs   = resto[n_train:n_train + n_val]
test_imgs  = resto[n_train + n_val:] + celular   # fotos de celular siempre en test

splits = {"train": train_imgs, "val": val_imgs, "test": test_imgs}

print(f"Total imágenes: {len(all_imgs)}")
print(f"  Fotos celular (→ test forzado): {len(celular)}")
print(f"  Resto: {len(resto)}")
print()
print(f"Split:")
print(f"  train: {len(train_imgs)}")
print(f"  val:   {len(val_imgs)}")
print(f"  test:  {len(test_imgs)}  (incluye {len(celular)} fotos de celular)")
print()

# ── Crear carpetas y copiar ─────────────────────────────────────────────────
for split, imgs in splits.items():
    img_dir = DATASET / "images" / split
    lbl_dir = DATASET / "labels" / split
    img_dir.mkdir(parents=True, exist_ok=True)
    lbl_dir.mkdir(parents=True, exist_ok=True)

    for img_path in imgs:
        # Imagen
        shutil.copy(img_path, img_dir / img_path.name)

        # Label correspondiente (puede no existir si la página no tiene tabla)
        lbl_path = ALL_LABELS / (img_path.stem + ".txt")
        if lbl_path.exists():
            shutil.copy(lbl_path, lbl_dir / lbl_path.name)
        else:
            # Crear .txt vacío (imagen sin tabla → clase negativa válida)
            (lbl_dir / (img_path.stem + ".txt")).write_text("")

    print(f"  {split}: {len(imgs)} imágenes copiadas → {img_dir}")

# ── dataset.yaml: solo actualizar paths, preservar nc y names ──────────────
yaml_path = DATASET / "dataset.yaml"
if not yaml_path.exists():
    # Solo crear si no existe (para no pisar el nc y names ya configurados)
    yaml_path.write_text(
        "path: .\n"
        "train: images/train\n"
        "val:   images/val\n"
        "test:  images/test\n"
        "\n"
        "# Completar nc y names segun el dataset anotado\n"
        "nc: 12\n"
    )
    print(f"\ndataset.yaml creado en {yaml_path}")
else:
    print(f"\ndataset.yaml existente preservado en {yaml_path}")
print("\n✓ Split completo. Siguiente paso:")
print("  zip -r dataset.zip ai/dataset/images ai/dataset/labels ai/dataset/dataset.yaml")
