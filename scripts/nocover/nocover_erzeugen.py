#!/usr/bin/env python3
"""Erzeugt include/images/nocover.h, den Cover-Platzhalter des SD-Players.

Ablauf: Vorlage laden (380x380, dunkler Grund, goldene Note mit Equalizer-Balken), Balken leicht
aufhellen, nur den Hintergrund auf das Grau des Coverfelds (0x2104) anheben, das Motiv ins Orange
drehen und saettigen, auf ICON_SIZE verkleinern, mittig in ein WIDTH x HEIGHT grosses Feld setzen
und mit Floyd-Steinberg-Fehlerdiffusion nach RGB565 wandeln. Die Fehlerdiffusion ist entscheidend:
ohne sie zeigt das Panel in weichen Verlaeufen sichtbare Farbstufen.

Aufruf im Projektstamm:  python3 scripts/nocover/nocover_erzeugen.py
Benoetigt Python 3 mit Pillow und NumPy. WIDTH und HEIGHT muessen COVER_W und COVER_H in
include/SDPlayer.h entsprechen.
"""
import os
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SOURCE = os.path.join(HERE, "note_equalizer_vorlage.png")
OUT_H = os.path.normpath(os.path.join(HERE, "..", "..", "include", "images", "nocover.h"))

WIDTH, HEIGHT = 230, 220      # COVER_W x COVER_H
ICON_SIZE = 176               # Kantenlaenge des Motivs im Feld
ICON_Y = 18                   # obere Kante des Motivs, unten bleibt Platz fuer "Kein Cover"
BG = (32, 32, 32)             # 0x2104, Grau des Coverfelds und der Infobox
BAR_GAIN = 1.6                # Aufhellung der Equalizer-Balken (Abstand zum Grund)
LIFT_BAND = 70                # bis zu diesem Abstand vom Grund werden Pixel als Hintergrund angehoben
HUE_SHIFT = -3                # Farbdrehung des Motivs in PIL-HSV-Einheiten (256 = Vollkreis), negativ = ins Orange
SAT_GAIN = 1.22               # Saettigungsfaktor des Motivs


def to565_dither(img):
    """Floyd-Steinberg je Kanal auf 5/6/5 Bit; liefert das RGB565-Feld."""
    a = np.asarray(img.convert("RGB")).astype(np.float64)
    h, w, _ = a.shape
    lv = np.array([31, 63, 31], dtype=np.float64)
    out = np.zeros((h, w, 3), dtype=np.int32)
    for y in range(h):
        row = a[y]
        nxt = a[y + 1] if y + 1 < h else None
        for x in range(w):
            old = row[x]
            q = np.clip(np.rint(old / 255.0 * lv), 0, lv)
            out[y, x] = q
            err = old - q / lv * 255.0
            if x + 1 < w:
                row[x + 1] += err * 7 / 16
            if nxt is not None:
                if x > 0:
                    nxt[x - 1] += err * 3 / 16
                nxt[x] += err * 5 / 16
                if x + 1 < w:
                    nxt[x + 1] += err * 1 / 16
    return (out[:, :, 0] << 11) | (out[:, :, 1] << 5) | out[:, :, 2]


def brighten_bars(a, base):
    """Graue Pixel knapp ueber dem Grund (Equalizer-Balken) vom Grund weg verstaerken."""
    off = a - base
    sat = (a.max(axis=2) - a.min(axis=2)) / np.maximum(a.max(axis=2), 1.0)
    grey = (sat < 0.25) & (off.max(axis=2) > 4) & (off.max(axis=2) < 110)
    a[grey] = base + off[grey] * BAR_GAIN
    return np.clip(a, 0, 255)


def lift_background(a, base):
    """Nur hintergrundnahe Pixel auf BG anheben; liefert Bild und Hintergrundgewicht (1 = Grund)."""
    lift = np.array(BG, dtype=np.float64) - base
    dist = np.abs(a - base).max(axis=2)
    w = np.clip(1.0 - dist / LIFT_BAND, 0.0, 1.0)
    return np.clip(a + lift[None, None, :] * w[..., None], 0, 255), w


def warm(img, w):
    """Motivpixel ins Orange drehen und saettigen, gewichtet mit (1 - Hintergrundgewicht)."""
    hsv = np.asarray(img.convert("HSV")).astype(np.float64)
    m = 1.0 - w
    hsv[:, :, 0] = (hsv[:, :, 0] + HUE_SHIFT * m) % 256
    hsv[:, :, 1] = np.clip(hsv[:, :, 1] * (1 + (SAT_GAIN - 1) * m), 0, 255)
    return Image.fromarray(hsv.astype(np.uint8), "HSV").convert("RGB")


def main():
    src = Image.open(SOURCE).convert("RGB")
    a = np.asarray(src).astype(np.float64)
    base = a[2:6, 2:6].reshape(-1, 3).mean(axis=0)   # Grundfarbe der Vorlage aus der Ecke
    a = brighten_bars(a, base).astype(np.uint8).astype(np.float64)   # Zwischenstand als 8-Bit-Bild, wie bei der abgenommenen Fassung
    base = a[2:6, 2:6].reshape(-1, 3).mean(axis=0)
    a, w = lift_background(a, base)
    motif = warm(Image.fromarray(a.astype(np.uint8)), w)
    icon = motif.resize((ICON_SIZE, ICON_SIZE), Image.LANCZOS)
    canvas = Image.new("RGB", (WIDTH, HEIGHT), BG)
    canvas.paste(icon, ((WIDTH - ICON_SIZE) // 2, ICON_Y))
    v = to565_dither(canvas)
    flat = v.flatten()
    lines = [
        "// Cover-Platzhalter: Note mit Equalizer, %dx%d (COVER_W x COVER_H), RGB565 mit Fehlerdiffusion" % (WIDTH, HEIGHT),
        "// Quelle: ChatGPT-Motivsammlung vom 2026-09-09, Kachel ohne Rahmen und Schriftzug; nur der Hintergrund auf 0x2104 angehoben, Motiv ins Orange gedreht",
        "// Werkzeug: scripts/nocover/nocover_erzeugen.py (Python/PIL)",
        "",
        "const unsigned short nocover[0x%X] PROGMEM = {" % len(flat),
    ]
    for i in range(0, len(flat), 16):
        lines.append(", ".join("0x%04X" % int(x) for x in flat[i:i + 16]) + ",")
    lines += ["};", ""]
    with open(OUT_H, "w", newline="\n") as f:
        f.write("\n".join(lines))
    print("geschrieben:", OUT_H, "(%d Werte, Ecke 0x%04X)" % (len(flat), int(v[0, 0])))


if __name__ == "__main__":
    main()
