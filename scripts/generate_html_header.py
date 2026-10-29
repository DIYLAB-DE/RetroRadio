#!/usr/bin/env python3
"""
Konvertiert webui/index.html -> include/index_html_gz.h (PROGMEM Byte-Array)

Aufruf:
  python3 scripts/generate_html_header.py

Oder als PlatformIO pre-build Script in platformio.ini:
  extra_scripts = pre:scripts/generate_html_header.py
"""

import gzip
import os
import sys

# Unter PlatformIO/SCons steht env nach dem Import zur Verfügung. Daraus lässt
# sich die Projektwurzel zuverlässig ableiten, egal in welchem Unterordner
# dieses Script liegt. Beim direkten Aufruf bleibt _PIO_BUILD False.
_PIO_BUILD = False
try:
    Import("env")
    _PIO_BUILD = True
except Exception:
    pass


def resolve_project_dir():
    """Liefert das Projekt-Wurzelverzeichnis, in dem webui/ und include/ liegen."""
    if _PIO_BUILD:
        return env.subst("$PROJECT_DIR")
    # Standalone: eine Ebene über dem scripts-Ordner, sonst das aktuelle Verzeichnis.
    try:
        here = os.path.dirname(os.path.abspath(__file__))
    except NameError:
        return os.getcwd()
    parent = os.path.dirname(here)
    return parent if os.path.isdir(os.path.join(parent, "webui")) else here


PROJECT_DIR = resolve_project_dir()
INPUT_FILE = os.path.join(PROJECT_DIR, "webui", "index.html")
OUTPUT_FILE = os.path.join(PROJECT_DIR, "include", "index_html_gz.h")


def generate_header(input_path, output_path):
    """Liest HTML, komprimiert immer mit gzip und schreibt C-Header."""

    if not os.path.exists(input_path):
        print(f"[WebUI] Quelldatei nicht gefunden: {input_path}")
        return False

    # Lesen
    with open(input_path, "rb") as f:
        raw = f.read()

    # Komprimieren
    compressed = gzip.compress(raw, compresslevel=9)
    raw_size = len(raw)
    gz_size = len(compressed)
    saving = 100 - (gz_size * 100 // raw_size)

    # Header schreiben
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    with open(output_path, "w") as out:
        out.write("#ifndef INDEX_HTML_GZ_H\n")
        out.write("#define INDEX_HTML_GZ_H\n\n")
        out.write("#include <pgmspace.h>\n\n")
        out.write("/**\n")
        out.write(" * @brief Komprimierte WebUI (index.html) als PROGMEM-Array\n")
        out.write(" *\n")
        out.write(" * Auto-generiert von generate_html_header.py\n")
        out.write(" * NICHT manuell bearbeiten!\n")
        out.write(" *\n")
        out.write(f" * Original:     {raw_size:>7} Bytes\n")
        out.write(f" * Komprimiert:  {gz_size:>7} Bytes ({saving}% Ersparnis)\n")
        out.write(" */\n\n")
        out.write(f"static const size_t INDEX_HTML_GZ_SIZE = {gz_size};\n\n")
        out.write("static const uint8_t INDEX_HTML_GZ[] PROGMEM = {\n")

        for i in range(0, gz_size, 16):
            chunk = compressed[i : i + 16]
            hex_str = ", ".join(f"0x{b:02X}" for b in chunk)
            if i + 16 < gz_size:
                out.write(f"    {hex_str},\n")
            else:
                out.write(f"    {hex_str}\n")

        out.write("};\n\n")
        out.write("#endif // INDEX_HTML_GZ_H\n")

    print(f"[WebUI] {raw_size:,} -> {gz_size:,} Bytes ({saving}% Ersparnis) -> {output_path}")
    return True


# Ausführung
if _PIO_BUILD:
    # Als pre:-Script direkt ausführen, nicht als deferred Action.
    generate_header(INPUT_FILE, OUTPUT_FILE)
elif __name__ == "__main__":
    inp = sys.argv[1] if len(sys.argv) > 1 else INPUT_FILE
    outp = sys.argv[2] if len(sys.argv) > 2 else OUTPUT_FILE
    if generate_header(inp, outp):
        print("[WebUI] Fertig!")
    else:
        sys.exit(1)