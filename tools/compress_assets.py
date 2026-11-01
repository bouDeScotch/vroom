#!/usr/bin/env python3
"""Compresse les circuits et masques de checkpoints de assets_uncompressed/ vers assets/.

Entree  : assets_uncompressed/Tracks/*.png
Sortie  : assets/Tracks/<nom>.trk   (circuit)       pour tous les PNG
          assets/Tracks/<nom>.cpt   (checkpoints)   pour les PNG dont le nom commence par "Checkpoints"

Format commun (little-endian), a garder synchronise avec Track.cpp :
    uint16 largeur, uint16 hauteur
    puis des runs jusqu'a couvrir largeur*hauteur pixels, en ordre ligne par ligne :
        uint8 valeur, uint24 nombre de repetitions   (un run plus long que 2^24-1 est coupe)

Valeurs d'un circuit (.trk) : 0 asphalte, 1 gravier, 2 herbe, 3 mur, 4 depart (pixel magenta).
Valeurs de checkpoints (.cpt) : 0 aucun, n+1 pour le pixel opaque rgb(0,0,n).

Usage : python tools/compress_assets.py [--verify]
"""
import argparse
import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "assets_uncompressed" / "Tracks"
DST = ROOT / "assets" / "Tracks"

MAX_RUN = (1 << 24) - 1

# Meme palette et meme ordre de priorite que Track.cpp (le premier gagne en cas d'egalite) : (code, r, g, b)
PALETTE = [
    (3, 200, 0, 0),      # mur
    (2, 0, 160, 0),      # herbe
    (1, 210, 190, 130),  # gravier
    (0, 128, 128, 128),  # asphalte
]
START_CODE = 4
START_RGB = (255, 0, 255)


def load_rgba(path: Path) -> np.ndarray:
    return np.array(Image.open(path).convert("RGBA"))


def track_codes(rgba: np.ndarray) -> np.ndarray:
    """Un code de terrain par pixel (l'alpha est ignore, comme dans le jeu)."""
    rgb = rgba[:, :, :3].astype(np.int32)
    packed = (rgb[:, :, 0] << 16) | (rgb[:, :, 1] << 8) | rgb[:, :, 2]
    colors, inverse = np.unique(packed.ravel(), return_inverse=True)

    lut = np.empty(len(colors), dtype=np.uint8)
    for i, c in enumerate(colors):
        r, g, b = (int(c) >> 16) & 255, (int(c) >> 8) & 255, int(c) & 255
        if (r, g, b) == START_RGB:
            lut[i] = START_CODE
            continue
        best_code, best_dist = 0, None
        for code, pr, pg, pb in PALETTE:
            d = (r - pr) ** 2 + (g - pg) ** 2 + (b - pb) ** 2
            if best_dist is None or d < best_dist:
                best_code, best_dist = code, d
        lut[i] = best_code
    return lut[inverse].reshape(packed.shape)


def checkpoint_codes(rgba: np.ndarray) -> np.ndarray:
    """0 = aucun, n+1 = pixel opaque rgb(0,0,n). Les bords semi-transparents sont ignores."""
    solid = (rgba[:, :, 3] == 255) & (rgba[:, :, 0] == 0) & (rgba[:, :, 1] == 0)
    codes = np.where(solid, rgba[:, :, 2].astype(np.int32) + 1, 0)
    if codes.max() > 255:
        raise ValueError("trop de checkpoints pour un octet")
    return codes.astype(np.uint8)


def encode(codes: np.ndarray) -> bytes:
    height, width = codes.shape
    if width > 0xFFFF or height > 0xFFFF:
        raise ValueError(f"image trop grande ({width}x{height}), maximum 65535")
    flat = codes.ravel()
    starts = np.concatenate(([0], np.flatnonzero(flat[1:] != flat[:-1]) + 1))
    lengths = np.diff(np.concatenate((starts, [flat.size])))
    out = bytearray(struct.pack("<HH", width, height))
    for value, length in zip(flat[starts], lengths):
        length = int(length)
        while length > 0:
            n = min(length, MAX_RUN)
            out += struct.pack("<B", int(value)) + n.to_bytes(3, "little")
            length -= n
    return bytes(out)


def decode(data: bytes) -> np.ndarray:
    width, height = struct.unpack_from("<HH", data, 0)
    parts, pos = [], 4
    while pos < len(data):
        parts.append(np.full(int.from_bytes(data[pos + 1:pos + 4], "little"), data[pos], dtype=np.uint8))
        pos += 4
    flat = np.concatenate(parts)
    if flat.size != width * height:
        raise ValueError("fichier corrompu : nombre de pixels incorrect")
    return flat.reshape(height, width)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--verify", action="store_true", help="relit chaque fichier produit et compare")
    args = parser.parse_args()

    sources = sorted(SRC.glob("*.png"))
    if not sources:
        print(f"Aucun PNG dans {SRC}", file=sys.stderr)
        return 1
    DST.mkdir(parents=True, exist_ok=True)

    for png in sources:
        is_checkpoints = png.stem.lower().startswith("checkpoints")
        rgba = load_rgba(png)
        codes = checkpoint_codes(rgba) if is_checkpoints else track_codes(rgba)
        data = encode(codes)
        out = DST / (png.stem + (".cpt" if is_checkpoints else ".trk"))
        out.write_bytes(data)

        if args.verify and not np.array_equal(decode(out.read_bytes()), codes):
            print(f"ECHEC de la verification pour {out.name}", file=sys.stderr)
            return 1

        note = ""
        if is_checkpoints:
            note = f", {int((np.unique(codes) > 0).sum())} checkpoints"
        elif not (codes == START_CODE).any():
            note = ", ATTENTION : aucun pixel de depart magenta"
        print(f"{png.name:20s} {png.stat().st_size:>9,} o -> {out.name:16s} {len(data):>9,} o{note}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
