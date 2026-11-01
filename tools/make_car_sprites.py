#!/usr/bin/env python3
"""Genere des sprites provisoires pour les voitures du catalogue a partir de assets/Cars/Car1.png.

Chaque variante change la couleur de la carrosserie et des bandes, et ajuste la largeur du sprite
au rapport largeur/longueur de la voiture. A remplacer par de vrais sprites : il suffit d'ecraser
le PNG (nez vers le haut, meme principe que Car1.png), le jeu n'a rien d'autre a savoir.

Usage : python tools/make_car_sprites.py
"""
from pathlib import Path

import numpy as np
from PIL import Image

CARS = Path(__file__).resolve().parent.parent / "assets" / "Cars"
SOURCE = CARS / "Car1.png"

# Rapport largeur/longueur de la WRX (1.83 / 4.67), reference de Car1.png
REF_RATIO = 1.83 / 4.67

# nom du fichier : (couleur carrosserie, couleur des bandes, largeur voiture m, longueur voiture m)
VARIANTS = {
    "Twingo.png":    ((245, 205, 40),  (200, 30, 30),   1.63, 3.43),
    "Skyline.png":   ((20, 80, 200),   (230, 230, 235), 1.785, 4.60),
    "Renegade.png":  ((60, 110, 70),   (220, 220, 220), 1.805, 4.236),
    "GT3RS.png":     ((235, 95, 20),   (240, 240, 240), 1.90, 4.572),
    "Chiron.png":    ((30, 60, 160),   (20, 20, 20),    2.038, 4.544),
}


def shade(color, factor):
    return tuple(int(c * factor) for c in color)


def recolor(rgba: np.ndarray, pairs) -> np.ndarray:
    """Remplace chaque couleur d'origine par la nouvelle, avec un fondu pour les bords anti-crenele."""
    out = rgba.astype(np.float32)
    rgb = out[:, :, :3]
    for src, dst in pairs:
        dist = np.linalg.norm(rgb - np.array(src, dtype=np.float32), axis=2)
        weight = np.clip(1.0 - dist / 60.0, 0.0, 1.0)[:, :, None]
        out[:, :, :3] += weight * (np.array(dst, dtype=np.float32) - np.array(src, dtype=np.float32))
    return np.clip(out, 0, 255).astype(np.uint8)


def main() -> None:
    img = Image.open(SOURCE).convert("RGBA")
    arr = np.array(img)
    h, w = arr.shape[:2]
    def sample(fx, fy):
        return tuple(int(v) for v in arr[int(h * fy), int(w * fx)][:3])

    body_from = sample(0.50, 0.55)
    stripe_from = sample(0.27, 0.55)
    wing_from = sample(0.50, 0.03)        # becquets avant et arriere, plus sombres que la carrosserie
    light_from = sample(0.50, 0.085)      # moitie haute de la bulle (phare), plus sombre que les bandes

    for name, (body, stripe, width_m, length_m) in VARIANTS.items():
        pairs = [(body_from, body), (stripe_from, stripe), (wing_from, shade(body, 0.45)), (light_from, shade(stripe, 0.8))]
        variant = Image.fromarray(recolor(arr, pairs))
        new_w = round(w * (width_m / length_m) / REF_RATIO)
        variant = variant.resize((new_w, h), Image.LANCZOS)
        variant.save(CARS / name)
        print(f"{name:14s} {new_w}x{h}")


if __name__ == "__main__":
    main()
