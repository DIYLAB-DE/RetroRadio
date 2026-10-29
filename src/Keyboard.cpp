#include "Keyboard.h"

#include <cmath>
#include <cstring>
#include <limits>

#include "I18n.h"
#include "Logger.h"
#include "StringUtils.h"
#include "Theme.h"

// Fonts
#include "fonts/ComfortaaBold18.h"

// ===================================================================
// Layout-Definitionen
// ===================================================================

// Hilfsmakros für kompaktere Tastendefinition Datum-Werte: MC_DATUM=4 (zentriert), ML_DATUM=0 (links), MR_DATUM=8 (rechts)
#define K(l, a, w) {l, a, w, nullptr, 0, MC_DATUM}           // Standard (zentriert)
#define K_C(l, a, w, c) {l, a, w, nullptr, c, MC_DATUM}      // Mit individueller Farbe
#define K_L(l, a, w) {l, a, w, nullptr, 0, ML_DATUM}         // Links ausgerichtet
#define K_R(l, a, w) {l, a, w, nullptr, 0, MR_DATUM}         // Rechts ausgerichtet
#define K_ICON(l, a, w, i) {l, a, w, i, 0, MC_DATUM}         // Mit Icon
#define K_ICON_C(l, a, w, i, c) {l, a, w, i, c, MC_DATUM}    // Mit Icon und Farbe
#define K_END {nullptr, nullptr, 0.0f, nullptr, 0, MC_DATUM} // Ende der Zeile

/**
 * Layout: Kleinbuchstaben
 */
const KeyDef Keyboard::LAYOUT_LOWERCASE[Keyboard::Layout::KEYBOARD_ROWS][Keyboard::Layout::MAX_COLS] = {
    // 1. Zeile
    {
        K("^", "^", 1.0f),
        K("1", "1", 1.0f),
        K("2", "2", 1.0f),
        K("3", "3", 1.0f),
        K("4", "4", 1.0f),
        K("5", "5", 1.0f),
        K("6", "6", 1.0f),
        K("7", "7", 1.0f),
        K("8", "8", 1.0f),
        K("9", "9", 1.0f),
        K("0", "0", 1.0f),
        K("ß", "ß", 1.0f),
        K("´", "´", 1.0f),
        K("^BACKSPACE:20^", "BACKSPACE", 1.0f),
        K_END},
    // 2. Zeile
    {
        K("", "", 1.5f),
        K("q", "q", 1.0f),
        K("w", "w", 1.0f),
        K("e", "e", 1.0f),
        K("r", "r", 1.0f),
        K("t", "t", 1.0f),
        K("z", "z", 1.0f),
        K("u", "u", 1.0f),
        K("i", "i", 1.0f),
        K("o", "o", 1.0f),
        K("p", "p", 1.0f),
        K("ü", "ü", 1.0f),
        K("+", "+", 1.0f),
        K_END},
    // 3. Zeile
    {
        K_L("^CAPSLOCK:24^", "CAPSLOCK", 1.75f),
        K("a", "a", 1.0f),
        K("s", "s", 1.0f),
        K("d", "d", 1.0f),
        K("f", "f", 1.0f),
        K("g", "g", 1.0f),
        K("h", "h", 1.0f),
        K("j", "j", 1.0f),
        K("k", "k", 1.0f),
        K("l", "l", 1.0f),
        K("ö", "ö", 1.0f),
        K("ä", "ä", 1.0f),
        K("#", "#", 1.0f),
        K_END},
    // 4. Zeile
    {
        K_L("^SHIFT:24^", "SHIFT", 1.5f),
        K("<", "<", 1.0f),
        K("y", "y", 1.0f),
        K("x", "x", 1.0f),
        K("c", "c", 1.0f),
        K("v", "v", 1.0f),
        K("b", "b", 1.0f),
        K("n", "n", 1.0f),
        K("m", "m", 1.0f),
        K(",", ",", 1.0f),
        K(".", ".", 1.0f),
        K("-", "-", 1.0f),
        K_R("^SHIFT:24^", "SHIFT", 1.5f),
        K_END},
    // 5. Zeile
    {
        K("!€1", "SYMBOLS", 1.5f),
        K("@", "@", 1.0f),
        K("", "SPACE", 6.0f),
        K(".de", ".de", 1.5f),
        K("^CLEFT:24^", "LEFT", 1.0f),
        K("^CRIGHT:24^", "RIGHT", 1.0f),
        K("OK", "OK", 2.0f),
        K_END}};

/**
 * Layout: Großbuchstaben
 */
const KeyDef Keyboard::LAYOUT_UPPERCASE[Keyboard::Layout::KEYBOARD_ROWS][Keyboard::Layout::MAX_COLS] = {
    // 1. Zeile
    {
        K("°", "°", 1.0f),
        K("!", "!", 1.0f),
        K("\\", "\\", 1.0f),
        K("§", "§", 1.0f),
        K("$", "$", 1.0f),
        K("%", "%", 1.0f),
        K("&", "&", 1.0f),
        K("/", "/", 1.0f),
        K("(", "(", 1.0f),
        K(")", ")", 1.0f),
        K("=", "=", 1.0f),
        K("?", "?", 1.0f),
        K("`", "`", 1.0f),
        K("^BACKSPACE:20^", "BACKSPACE", 1.0f),
        K_END},
    // 2. Zeile
    {
        K("", "", 1.5f),
        K("Q", "Q", 1.0f),
        K("W", "W", 1.0f),
        K("E", "E", 1.0f),
        K("R", "R", 1.0f),
        K("T", "T", 1.0f),
        K("Z", "Z", 1.0f),
        K("U", "U", 1.0f),
        K("I", "I", 1.0f),
        K("O", "O", 1.0f),
        K("P", "P", 1.0f),
        K("Ü", "Ü", 1.0f),
        K("*", "*", 1.0f),
        K_END},
    // 3. Zeile
    {
        K_L("^CAPSLOCK:24^", "CAPSLOCK", 1.75f),
        K("A", "A", 1.0f),
        K("S", "S", 1.0f),
        K("D", "D", 1.0f),
        K("F", "F", 1.0f),
        K("G", "G", 1.0f),
        K("H", "H", 1.0f),
        K("J", "J", 1.0f),
        K("K", "K", 1.0f),
        K("L", "L", 1.0f),
        K("Ö", "Ö", 1.0f),
        K("Ä", "Ä", 1.0f),
        K("'", "'", 1.0f),
        K_END},
    // 4. Zeile
    {
        K_L("^SHIFT:24^", "SHIFT", 1.5f),
        K(">", ">", 1.0f),
        K("Y", "Y", 1.0f),
        K("X", "X", 1.0f),
        K("C", "C", 1.0f),
        K("V", "V", 1.0f),
        K("B", "B", 1.0f),
        K("N", "N", 1.0f),
        K("M", "M", 1.0f),
        K(";", ";", 1.0f),
        K(":", ":", 1.0f),
        K("_", "_", 1.0f),
        K_R("^SHIFT:24^", "SHIFT", 1.5f),
        K_END},
    // 5. Zeile
    {
        K("!€1", "SYMBOLS", 1.5f),
        K("@", "@", 1.0f),
        K("", "SPACE", 6.0f),
        K(".de", ".de", 1.5f),
        K("^CLEFT:24^", "LEFT", 1.0f),
        K("^CRIGHT:24^", "RIGHT", 1.0f),
        K("OK", "OK", 2.0f),
        K_END}};

/**
 * Layout: Symbole und Sonderzeichen (Kleinbuchstaben-Variante)
 */
const KeyDef Keyboard::LAYOUT_SYMBOLS_LOWERCASE[Keyboard::Layout::KEYBOARD_ROWS][Keyboard::Layout::MAX_COLS] = {
    // 1. Zeile
    {
        K("^", "^", 1.0f),
        K("1", "1", 1.0f),
        K_C("²", "²", 1.0f, TFT_GOLD),
        K_C("³", "³", 1.0f, TFT_GOLD),
        K("4", "4", 1.0f),
        K("5", "5", 1.0f),
        K("6", "6", 1.0f),
        K_C("{", "{", 1.0f, TFT_GOLD),
        K_C("[", "[", 1.0f, TFT_GOLD),
        K_C("]", "]", 1.0f, TFT_GOLD),
        K_C("}", "}", 1.0f, TFT_GOLD),
        K_C("\\", "\\", 1.0f, TFT_GOLD),
        K("´", "´", 1.0f),
        K("^BACKSPACE:20^", "BACKSPACE", 1.0f),
        K_END},
    // 2. Zeile
    {
        K("", "", 1.5f),
        K_C("@", "@", 1.0f, TFT_GOLD),
        K("w", "w", 1.0f),
        K_C("€", "€", 1.0f, TFT_GOLD),
        K("r", "r", 1.0f),
        K("t", "t", 1.0f),
        K("z", "z", 1.0f),
        K("u", "u", 1.0f),
        K("i", "i", 1.0f),
        K("o", "o", 1.0f),
        K("p", "p", 1.0f),
        K("ü", "ü", 1.0f),
        K_C("~", "~", 1.0f, TFT_GOLD),
        K_END},
    // 3. Zeile
    {
        K_L("^CAPSLOCK:24^", "CAPSLOCK", 1.75f),
        K("a", "a", 1.0f),
        K("s", "s", 1.0f),
        K("d", "d", 1.0f),
        K("f", "f", 1.0f),
        K("g", "g", 1.0f),
        K("h", "h", 1.0f),
        K("j", "j", 1.0f),
        K("k", "k", 1.0f),
        K("l", "l", 1.0f),
        K("ö", "ö", 1.0f),
        K("ä", "ä", 1.0f),
        K("#", "#", 1.0f),
        K_END},
    // 4. Zeile
    {
        K_L("^SHIFT:24^", "SHIFT", 1.5f),
        K_C("|", "|", 1.0f, TFT_GOLD),
        K("y", "y", 1.0f),
        K("x", "x", 1.0f),
        K("c", "c", 1.0f),
        K("v", "v", 1.0f),
        K("b", "b", 1.0f),
        K("n", "n", 1.0f),
        K("m", "m", 1.0f),
        K(",", ",", 1.0f),
        K(".", ".", 1.0f),
        K("-", "-", 1.0f),
        K_R("^SHIFT:24^", "SHIFT", 1.5f),
        K_END},
    // 5. Zeile
    {
        K("ABC", "LETTERS", 1.5f),
        K("@", "@", 1.0f),
        K("", "SPACE", 6.0f),
        K(".de", ".de", 1.5f),
        K("^CLEFT:24^", "LEFT", 1.0f),
        K("^CRIGHT:24^", "RIGHT", 1.0f),
        K("OK", "OK", 2.0f),
        K_END}};

/**
 * Layout: Symbole und Sonderzeichen (Großbuchstaben-Variante)
 */
const KeyDef Keyboard::LAYOUT_SYMBOLS_UPPERCASE[Keyboard::Layout::KEYBOARD_ROWS][Keyboard::Layout::MAX_COLS] = {
    // 1. Zeile
    {
        K("°", "°", 1.0f),
        K("!", "!", 1.0f),
        K_C("²", "²", 1.0f, TFT_GOLD),
        K_C("³", "³", 1.0f, TFT_GOLD),
        K("$", "$", 1.0f),
        K("%", "%", 1.0f),
        K("&", "&", 1.0f),
        K_C("{", "{", 1.0f, TFT_GOLD),
        K_C("[", "[", 1.0f, TFT_GOLD),
        K_C("]", "]", 1.0f, TFT_GOLD),
        K_C("}", "}", 1.0f, TFT_GOLD),
        K_C("\\", "\\", 1.0f, TFT_GOLD),
        K("`", "`", 1.0f),
        K("^BACKSPACE:20^", "BACKSPACE", 1.0f),
        K_END},
    // 2. Zeile
    {
        K("", "", 1.5f),
        K_C("@", "@", 1.0f, TFT_GOLD),
        K("W", "W", 1.0f),
        K_C("€", "€", 1.0f, TFT_GOLD),
        K("R", "R", 1.0f),
        K("T", "T", 1.0f),
        K("Z", "Z", 1.0f),
        K("U", "U", 1.0f),
        K("I", "I", 1.0f),
        K("O", "O", 1.0f),
        K("P", "P", 1.0f),
        K("Ü", "Ü", 1.0f),
        K_C("~", "~", 1.0f, TFT_GOLD),
        K_END},
    // 3. Zeile
    {
        K_L("^CAPSLOCK:24^", "CAPSLOCK", 1.75f),
        K("A", "A", 1.0f),
        K("S", "S", 1.0f),
        K("D", "D", 1.0f),
        K("F", "F", 1.0f),
        K("G", "G", 1.0f),
        K("H", "H", 1.0f),
        K("J", "J", 1.0f),
        K("K", "K", 1.0f),
        K("L", "L", 1.0f),
        K("Ö", "Ö", 1.0f),
        K("Ä", "Ä", 1.0f),
        K("'", "'", 1.0f),
        K_END},
    // 4. Zeile
    {
        K_L("^SHIFT:24^", "SHIFT", 1.5f),
        K_C("|", "|", 1.0f, TFT_GOLD),
        K("Y", "Y", 1.0f),
        K("X", "X", 1.0f),
        K("C", "C", 1.0f),
        K("V", "V", 1.0f),
        K("B", "B", 1.0f),
        K("N", "N", 1.0f),
        K("M", "M", 1.0f),
        K(";", ";", 1.0f),
        K(":", ":", 1.0f),
        K("_", "_", 1.0f),
        K_R("^SHIFT:24^", "SHIFT", 1.5f),
        K_END},
    // 5. Zeile
    {
        K("ABC", "LETTERS", 1.5f),
        K("@", "@", 1.0f),
        K("", "SPACE", 6.0f),
        K(".de", ".de", 1.5f),
        K("^CLEFT:24^", "LEFT", 1.0f),
        K("^CRIGHT:24^", "RIGHT", 1.0f),
        K("OK", "OK", 2.0f),
        K_END}};

/**
 * Layout: Kleinbuchstaben (Englisch, QWERTY)
 */
const KeyDef Keyboard::LAYOUT_EN_LOWERCASE[Keyboard::Layout::KEYBOARD_ROWS][Keyboard::Layout::MAX_COLS] = {
    // 1. Zeile
    {
        K("`", "`", 1.0f),
        K("1", "1", 1.0f),
        K("2", "2", 1.0f),
        K("3", "3", 1.0f),
        K("4", "4", 1.0f),
        K("5", "5", 1.0f),
        K("6", "6", 1.0f),
        K("7", "7", 1.0f),
        K("8", "8", 1.0f),
        K("9", "9", 1.0f),
        K("0", "0", 1.0f),
        K("-", "-", 1.0f),
        K("=", "=", 1.0f),
        K("^BACKSPACE:20^", "BACKSPACE", 1.0f),
        K_END},
    // 2. Zeile
    {
        K("", "", 1.5f),
        K("q", "q", 1.0f),
        K("w", "w", 1.0f),
        K("e", "e", 1.0f),
        K("r", "r", 1.0f),
        K("t", "t", 1.0f),
        K("y", "y", 1.0f),
        K("u", "u", 1.0f),
        K("i", "i", 1.0f),
        K("o", "o", 1.0f),
        K("p", "p", 1.0f),
        K("ü", "ü", 1.0f),
        K("]", "]", 1.0f),
        K_END},
    // 3. Zeile
    {
        K_L("^CAPSLOCK:24^", "CAPSLOCK", 1.75f),
        K("a", "a", 1.0f),
        K("s", "s", 1.0f),
        K("d", "d", 1.0f),
        K("f", "f", 1.0f),
        K("g", "g", 1.0f),
        K("h", "h", 1.0f),
        K("j", "j", 1.0f),
        K("k", "k", 1.0f),
        K("l", "l", 1.0f),
        K("ö", "ö", 1.0f),
        K("ä", "ä", 1.0f),
        K("\\", "\\", 1.0f),
        K_END},
    // 4. Zeile
    {
        K_L("^SHIFT:24^", "SHIFT", 2.5f),
        K("z", "z", 1.0f),
        K("x", "x", 1.0f),
        K("c", "c", 1.0f),
        K("v", "v", 1.0f),
        K("b", "b", 1.0f),
        K("n", "n", 1.0f),
        K("m", "m", 1.0f),
        K(",", ",", 1.0f),
        K(".", ".", 1.0f),
        K("-", "-", 1.0f),
        K_R("^SHIFT:24^", "SHIFT", 1.5f),
        K_END},
    // 5. Zeile
    {
        K("!€1", "SYMBOLS", 1.5f),
        K("@", "@", 1.0f),
        K("", "SPACE", 6.0f),
        K(".com", ".com", 1.5f),
        K("^CLEFT:24^", "LEFT", 1.0f),
        K("^CRIGHT:24^", "RIGHT", 1.0f),
        K("OK", "OK", 2.0f),
        K_END}};

/**
 * Layout: Großbuchstaben (Englisch, QWERTY)
 */
const KeyDef Keyboard::LAYOUT_EN_UPPERCASE[Keyboard::Layout::KEYBOARD_ROWS][Keyboard::Layout::MAX_COLS] = {
    // 1. Zeile
    {
        K("~", "~", 1.0f),
        K("!", "!", 1.0f),
        K("\"", "\"", 1.0f),
        K("£", "£", 1.0f),
        K("$", "$", 1.0f),
        K("%", "%", 1.0f),
        K("^", "^", 1.0f),
        K("&", "&", 1.0f),
        K("*", "*", 1.0f),
        K("(", "(", 1.0f),
        K(")", ")", 1.0f),
        K("_", "_", 1.0f),
        K("+", "+", 1.0f),
        K("^BACKSPACE:20^", "BACKSPACE", 1.0f),
        K_END},
    // 2. Zeile
    {
        K("", "", 1.5f),
        K("Q", "Q", 1.0f),
        K("W", "W", 1.0f),
        K("E", "E", 1.0f),
        K("R", "R", 1.0f),
        K("T", "T", 1.0f),
        K("Y", "Y", 1.0f),
        K("U", "U", 1.0f),
        K("I", "I", 1.0f),
        K("O", "O", 1.0f),
        K("P", "P", 1.0f),
        K("Ü", "Ü", 1.0f),
        K("}", "}", 1.0f),
        K_END},
    // 3. Zeile
    {
        K_L("^CAPSLOCK:24^", "CAPSLOCK", 1.75f),
        K("A", "A", 1.0f),
        K("S", "S", 1.0f),
        K("D", "D", 1.0f),
        K("F", "F", 1.0f),
        K("G", "G", 1.0f),
        K("H", "H", 1.0f),
        K("J", "J", 1.0f),
        K("K", "K", 1.0f),
        K("L", "L", 1.0f),
        K("Ö", "Ö", 1.0f),
        K("Ä", "Ä", 1.0f),
        K("'", "'", 1.0f),
        K_END},
    // 4. Zeile
    {
        K_L("^SHIFT:24^", "SHIFT", 2.5f),
        K("Z", "Z", 1.0f),
        K("X", "X", 1.0f),
        K("C", "C", 1.0f),
        K("V", "V", 1.0f),
        K("B", "B", 1.0f),
        K("N", "N", 1.0f),
        K("M", "M", 1.0f),
        K(";", ";", 1.0f),
        K(":", ":", 1.0f),
        K("_", "_", 1.0f),
        K_R("^SHIFT:24^", "SHIFT", 1.5f),
        K_END},
    // 5. Zeile
    {
        K("!€1", "SYMBOLS", 1.5f),
        K("@", "@", 1.0f),
        K("", "SPACE", 6.0f),
        K(".com", ".com", 1.5f),
        K("^CLEFT:24^", "LEFT", 1.0f),
        K("^CRIGHT:24^", "RIGHT", 1.0f),
        K("OK", "OK", 2.0f),
        K_END}};

/**
 * Layout: Symbole und Sonderzeichen, Kleinbuchstaben-Variante (Englisch)
 */
const KeyDef Keyboard::LAYOUT_EN_SYMBOLS_LOWERCASE[Keyboard::Layout::KEYBOARD_ROWS][Keyboard::Layout::MAX_COLS] = {
    // 1. Zeile
    {
        K("`", "`", 1.0f),
        K("1", "1", 1.0f),
        K_C("²", "²", 1.0f, TFT_GOLD),
        K_C("³", "³", 1.0f, TFT_GOLD),
        K("4", "4", 1.0f),
        K("5", "5", 1.0f),
        K("6", "6", 1.0f),
        K_C("{", "{", 1.0f, TFT_GOLD),
        K_C("[", "[", 1.0f, TFT_GOLD),
        K_C("]", "]", 1.0f, TFT_GOLD),
        K_C("}", "}", 1.0f, TFT_GOLD),
        K_C("\\", "\\", 1.0f, TFT_GOLD),
        K("=", "=", 1.0f),
        K("^BACKSPACE:20^", "BACKSPACE", 1.0f),
        K_END},
    // 2. Zeile
    {
        K("", "", 1.5f),
        K_C("@", "@", 1.0f, TFT_GOLD),
        K("w", "w", 1.0f),
        K_C("€", "€", 1.0f, TFT_GOLD),
        K("r", "r", 1.0f),
        K("t", "t", 1.0f),
        K("y", "y", 1.0f),
        K("u", "u", 1.0f),
        K("i", "i", 1.0f),
        K("o", "o", 1.0f),
        K("p", "p", 1.0f),
        K("ü", "ü", 1.0f),
        K_C("~", "~", 1.0f, TFT_GOLD),
        K_END},
    // 3. Zeile
    {
        K_L("^CAPSLOCK:24^", "CAPSLOCK", 1.75f),
        K("a", "a", 1.0f),
        K("s", "s", 1.0f),
        K("d", "d", 1.0f),
        K("f", "f", 1.0f),
        K("g", "g", 1.0f),
        K("h", "h", 1.0f),
        K("j", "j", 1.0f),
        K("k", "k", 1.0f),
        K("l", "l", 1.0f),
        K("ö", "ö", 1.0f),
        K("ä", "ä", 1.0f),
        K("\\", "\\", 1.0f),
        K_END},
    // 4. Zeile
    {
        K_L("^SHIFT:24^", "SHIFT", 2.5f),
        K("z", "z", 1.0f),
        K("x", "x", 1.0f),
        K("c", "c", 1.0f),
        K("v", "v", 1.0f),
        K("b", "b", 1.0f),
        K("n", "n", 1.0f),
        K("m", "m", 1.0f),
        K(",", ",", 1.0f),
        K(".", ".", 1.0f),
        K("-", "-", 1.0f),
        K_R("^SHIFT:24^", "SHIFT", 1.5f),
        K_END},
    // 5. Zeile
    {
        K("ABC", "LETTERS", 1.5f),
        K("@", "@", 1.0f),
        K("", "SPACE", 6.0f),
        K(".com", ".com", 1.5f),
        K("^CLEFT:24^", "LEFT", 1.0f),
        K("^CRIGHT:24^", "RIGHT", 1.0f),
        K("OK", "OK", 2.0f),
        K_END}};

/**
 * Layout: Symbole und Sonderzeichen, Großbuchstaben-Variante (Englisch)
 */
const KeyDef Keyboard::LAYOUT_EN_SYMBOLS_UPPERCASE[Keyboard::Layout::KEYBOARD_ROWS][Keyboard::Layout::MAX_COLS] = {
    // 1. Zeile
    {
        K("~", "~", 1.0f),
        K("!", "!", 1.0f),
        K_C("²", "²", 1.0f, TFT_GOLD),
        K_C("³", "³", 1.0f, TFT_GOLD),
        K("$", "$", 1.0f),
        K("%", "%", 1.0f),
        K("^", "^", 1.0f),
        K_C("{", "{", 1.0f, TFT_GOLD),
        K_C("[", "[", 1.0f, TFT_GOLD),
        K_C("]", "]", 1.0f, TFT_GOLD),
        K_C("}", "}", 1.0f, TFT_GOLD),
        K_C("\\", "\\", 1.0f, TFT_GOLD),
        K("+", "+", 1.0f),
        K("^BACKSPACE:20^", "BACKSPACE", 1.0f),
        K_END},
    // 2. Zeile
    {
        K("", "", 1.5f),
        K_C("@", "@", 1.0f, TFT_GOLD),
        K("W", "W", 1.0f),
        K_C("€", "€", 1.0f, TFT_GOLD),
        K("R", "R", 1.0f),
        K("T", "T", 1.0f),
        K("Y", "Y", 1.0f),
        K("U", "U", 1.0f),
        K("I", "I", 1.0f),
        K("O", "O", 1.0f),
        K("P", "P", 1.0f),
        K("Ü", "Ü", 1.0f),
        K_C("~", "~", 1.0f, TFT_GOLD),
        K_END},
    // 3. Zeile
    {
        K_L("^CAPSLOCK:24^", "CAPSLOCK", 1.75f),
        K("A", "A", 1.0f),
        K("S", "S", 1.0f),
        K("D", "D", 1.0f),
        K("F", "F", 1.0f),
        K("G", "G", 1.0f),
        K("H", "H", 1.0f),
        K("J", "J", 1.0f),
        K("K", "K", 1.0f),
        K("L", "L", 1.0f),
        K("Ö", "Ö", 1.0f),
        K("Ä", "Ä", 1.0f),
        K("'", "'", 1.0f),
        K_END},
    // 4. Zeile
    {
        K_L("^SHIFT:24^", "SHIFT", 2.5f),
        K("Z", "Z", 1.0f),
        K("X", "X", 1.0f),
        K("C", "C", 1.0f),
        K("V", "V", 1.0f),
        K("B", "B", 1.0f),
        K("N", "N", 1.0f),
        K("M", "M", 1.0f),
        K(";", ";", 1.0f),
        K(":", ":", 1.0f),
        K("_", "_", 1.0f),
        K_R("^SHIFT:24^", "SHIFT", 1.5f),
        K_END},
    // 5. Zeile
    {
        K("ABC", "LETTERS", 1.5f),
        K("@", "@", 1.0f),
        K("", "SPACE", 6.0f),
        K(".com", ".com", 1.5f),
        K("^CLEFT:24^", "LEFT", 1.0f),
        K("^CRIGHT:24^", "RIGHT", 1.0f),
        K("OK", "OK", 2.0f),
        K_END}};

#undef K
#undef K_C
#undef K_L
#undef K_R
#undef K_ICON
#undef K_ICON_C
#undef K_END

// ===================================================================
// Konstruktor und Destruktor
// ===================================================================

/**
 * @brief Konstruktor
 */
Keyboard::Keyboard(LGFX *gfx)
    : m_tft(gfx),
      m_inputSprite(nullptr),
      m_screenBuffer(nullptr),
      m_backupValid(false),
      m_inputBuffer(nullptr),
      m_bufferSize(0),
      m_cursorPos(0),
      m_running(false),
      m_result(false),
      m_state(KeyboardState::LOWERCASE),
      m_capsLock(false),
      m_passwordMode(false),
      m_passwordVisible(false),
      m_keyFont(nullptr),
      m_inputFont(nullptr),
      m_labelFont(nullptr),
      m_labelColor(Theme::ColorTitle),
      m_symbolsLowercaseLabelColor(Colors::SYMBOLS_LOWERCASE_TEXT),
      m_symbolsUppercaseLabelColor(Colors::SYMBOLS_UPPERCASE_TEXT),
      m_posX(Layout::DEFAULT_X),
      m_posY(Layout::DEFAULT_Y),
      m_lastPressedButton(nullptr),
      m_lastAction(nullptr),
      m_touchActive(false),
      m_touchStartTime(0),
      m_lastRepeatTime(0),
      m_lastReleaseTime(0),
      m_startTime(0),
      m_lastBlinkState(false),
      m_forceRedrawInput(false)
{
    m_buttons.reserve(60);
    m_buttonActions.reserve(60);
}

/**
 * @brief Destruktor
 */
Keyboard::~Keyboard()
{
    if (m_screenBuffer)
    {
        heap_caps_free(m_screenBuffer);
        m_screenBuffer = nullptr;
    }

    m_tft->safeDeleteSprite(m_inputSprite);
}

// ===================================================================
// Öffentliche Methoden
// ===================================================================

/**
 * @brief Setzt Position mit Koordinaten
 */
void Keyboard::setPosition(int x, int y)
{
    m_posX = x;
    m_posY = y;
}

/**
 * @brief Setzt zentrierte Position
 */
void Keyboard::setPosition()
{
    m_posX = Layout::DEFAULT_X;
    m_posY = Layout::DEFAULT_Y;
}

/**
 * @brief Setzt die Beschriftung der Bestätigen-Taste
 */
void Keyboard::setOkLabel(const char *label)
{
    if (label && *label)
        stringutils::safeCopy(m_okLabel, label);
}

/**
 * @brief Zeigt die Tastatur und startet die Eingabeschleife
 */
bool Keyboard::show(const char *label, char *buffer, size_t buffer_size, const char *initial_text)
{
    if (m_running || !buffer || buffer_size < 2)
        return false;

    // nichtblockierendes Warten bis Touch losgelassen wird (max. 200ms)
    int32_t dummy_x, dummy_y;
    uint32_t waitStart = millis();
    constexpr uint32_t MAX_WAIT_MS = 200;

    while (m_tft->getTouch(&dummy_x, &dummy_y))
    {
        if (millis() - waitStart > MAX_WAIT_MS)
            break; // Timeout - weitermachen auch wenn noch gedrückt

        yield(); // CPU freigeben
    }

    // Zustand initialisieren
    m_inputBuffer = buffer;
    m_bufferSize = buffer_size;
    m_state = KeyboardState::LOWERCASE;
    m_capsLock = false;
    m_running = true;
    m_result = false;
    m_touchActive = false;
    m_lastPressedButton = nullptr;
    m_lastAction = nullptr;
    m_forceRedrawInput = true;
    m_lastBlinkState = false;
    m_startTime = millis();
    m_lastReleaseTime = 0;

    // Initial-Text sicher kopieren
    stringutils::safeCopy(m_inputBuffer, m_bufferSize, initial_text);
    m_cursorPos = strlen(m_inputBuffer);

    // Input-Sprite erstellen
    int input_width = Layout::KEYBOARD_WIDTH - (2 * Layout::PADDING);

    // Platz für Toggle-Button links: Button-Breite + Abstand
    if (m_passwordMode)
        input_width -= (Layout::INPUT_HEIGHT + Layout::KEY_GAP);

    // Sprite erstellen
    if (!m_tft->safeCreateSprite(m_inputSprite, input_width, Layout::INPUT_HEIGHT))
    {
        LOG_ERROR("Konnte Input-Sprite nicht erstellen");
        return false;
    }

    m_tft->setTextDatum(TL_DATUM);

    // UI aufbauen
    backupScreen();
    drawFrame();
    drawLabel(label);

    // Close-Button (X) erstellen
    constexpr int CLOSE_BTN_SIZE = 30;
    int close_btn_x = m_posX + Layout::KEYBOARD_WIDTH - Layout::PADDING - CLOSE_BTN_SIZE / 2;
    int close_btn_y = m_posY + Layout::PADDING + CLOSE_BTN_SIZE / 2;

    m_closeButton.reset(new Button());

    m_closeButton->initButton(
        m_tft,                            // TFT-Displayobjekt
        close_btn_x,                      // x-Position
        close_btn_y,                      // y-Position
        CLOSE_BTN_SIZE,                   // Weite
        CLOSE_BTN_SIZE,                   // Höhe
        4,                                // Eckenradius
        static_cast<uint16_t>(0xF800),    // Hintergrundfarbe beim drücken
        static_cast<uint16_t>(0xF800),    // gedrückt Hintergrundfarbe
        static_cast<uint16_t>(TFT_WHITE), // Textfarbe
        "^DEL:20^",                       // Beschriftung
        nullptr,                          // kein Icon
        1.0f,                             // horizontale Textgröße
        1.0f);                            // vertikale Textgröße

    m_closeButton->drawButton(false);

    // Passwort-Toggle Button (links vom Eingabefeld)
    if (m_passwordMode)
    {
        // Button hat die gleiche Höhe wie das Eingabefeld
        constexpr int TOGGLE_BTN_SIZE = Layout::INPUT_HEIGHT;
        int toggle_btn_x = m_posX + Layout::PADDING + TOGGLE_BTN_SIZE / 2;
        int toggle_btn_y = m_posY + Layout::INPUT_Y_REL + TOGGLE_BTN_SIZE / 2;

        m_togglePasswordButton.reset(new Button());

        m_togglePasswordButton->initButton(
            m_tft,                                      // TFT-Displayobjekt
            toggle_btn_x,                               // x-Position
            toggle_btn_y,                               // y-Position
            TOGGLE_BTN_SIZE,                            // Weite
            TOGGLE_BTN_SIZE,                            // Höhe
            4,                                          // Eckenradius
            static_cast<uint16_t>(Colors::KEY_SPECIAL), // Hintergrundfarbe
            static_cast<uint16_t>(Colors::KEY_SPECIAL), // Hintergrundfarbe beim drücken
            static_cast<uint16_t>(TFT_LIGHTGREY),       // Textfarbe
            "^EYEOFF:20^",                              // Beschriftung
            nullptr,                                    // kein Icon
            1.0f,                                       // horizontale Textgröße
            1.0f);                                      // vertikale Textgröße

        m_togglePasswordButton->drawButton(false);
    }

    setupAndDrawLayout();

    // Hauptschleife
    while (m_running)
    {
        bool currentBlinkState = ((millis() / Timing::CURSOR_BLINK_MS) % 2) == 0;

        // Eingabefeld nur bei Bedarf neu zeichnen
        if (m_forceRedrawInput || (currentBlinkState != m_lastBlinkState))
        {
            drawInputField();
            m_forceRedrawInput = false;
            m_lastBlinkState = currentBlinkState;
        }

        handleTouch();
        yield(); // CPU freigeben für andere Tasks
    }

    // Aufräumen
    m_passwordMode = false;
    m_passwordVisible = false;
    m_togglePasswordButton.reset();
    m_closeButton.reset();

    restoreScreen();
    m_tft->safeDeleteSprite(m_inputSprite);

    return m_result;
}

// ===================================================================
// Screen-Backup
// ===================================================================

/**
 * @brief Sichert den Bildschirminhalt
 */
void Keyboard::backupScreen()
{
    size_t size = Layout::KEYBOARD_WIDTH * Layout::TOTAL_HEIGHT * sizeof(lgfx::rgb565_t);

    // Buffer immer neu anlegen, denn die Größe könnte sich geändert haben
    if (m_screenBuffer)
    {
        heap_caps_free(m_screenBuffer);
        m_screenBuffer = nullptr;
    }

    m_screenBuffer = static_cast<lgfx::rgb565_t *>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (m_screenBuffer)
    {
        m_tft->readRect(m_posX, m_posY, Layout::KEYBOARD_WIDTH, Layout::TOTAL_HEIGHT, m_screenBuffer);
        m_backupValid = true;
    }
    else
    {
        m_backupValid = false;
        LOG_ERROR("Keyboard: Konnte Backup-Buffer nicht anlegen");
    }
}

/**
 * @brief Stellt den gesicherten Bildschirminhalt wieder her
 */
void Keyboard::restoreScreen()
{
    if (m_backupValid && m_screenBuffer != nullptr)
    {
        m_tft->pushImage(m_posX, m_posY, Layout::KEYBOARD_WIDTH, Layout::TOTAL_HEIGHT, m_screenBuffer);
        m_backupValid = false;
    }
    else
    {
        LOG_WARNING("Restore abgebrochen: Backup ungültig oder Buffer null.");
    }
}

// ===================================================================
// Zeichnen
// ===================================================================

/**
 * @brief Zeichnet den Tastatur-Rahmen
 */
void Keyboard::drawFrame()
{
    m_tft->fillSmoothRoundRect(
        m_posX, m_posY,
        Layout::KEYBOARD_WIDTH, Layout::TOTAL_HEIGHT,
        Layout::BORDER_RADIUS, Theme::ColorDialogBorder);
    m_tft->fillSmoothRoundRect(
        m_posX + 1, m_posY + 1,
        Layout::KEYBOARD_WIDTH - 2, Layout::TOTAL_HEIGHT - 2,
        Layout::BORDER_RADIUS, Theme::ColorMainBackGround);
}

/**
 * @brief Zeichnet das Label über dem Eingabefeld
 */
void Keyboard::drawLabel(const char *label)
{
    if (!label || !*label)
        return;

    const uint8_t *fontToUse = m_labelFont ? m_labelFont : ComfortaaBold18;
    m_tft->loadFont(fontToUse);
    m_tft->setTextColor(m_labelColor, Theme::ColorMainBackGround);

    // Label vertikal am Close-Button ausrichten
    constexpr int CLOSE_BTN_SIZE = 30;
    int close_btn_y = m_posY + Layout::PADDING + CLOSE_BTN_SIZE / 2;
    m_tft->setTextDatum(ML_DATUM);
    m_tft->drawString(label, m_posX + Layout::PADDING, close_btn_y);

    m_tft->unloadFont();
    m_tft->setTextDatum(TL_DATUM);
}

/**
 * @brief Erstellt und zeichnet das aktuelle Tastatur-Layout
 */
void Keyboard::setupAndDrawLayout()
{
    m_tft->startWrite();

    m_buttons.clear();
    m_buttonActions.clear();
    m_lastPressedButton = nullptr;

    // Tastaturbereich löschen
    m_tft->fillRect(
        m_posX + Layout::PADDING,
        m_posY + Layout::KEYBOARD_Y_REL,
        Layout::KEYBOARD_WIDTH - (2 * Layout::PADDING),
        Layout::KEYBOARD_PART_HEIGHT,
        Theme::ColorMainBackGround);

    // Layout auswählen (Sprach-Switch nach DayView-Pointer-Switch-Pattern)
    const bool en = (I18n::currentLang() == LangIndex::EN);
    const KeyDef(*layout)[Layout::MAX_COLS];
    switch (m_state)
    {
    case KeyboardState::UPPERCASE:
        layout = en ? LAYOUT_EN_UPPERCASE : LAYOUT_UPPERCASE;
        break;
    case KeyboardState::SYMBOLS_LOWERCASE:
        layout = en ? LAYOUT_EN_SYMBOLS_LOWERCASE : LAYOUT_SYMBOLS_LOWERCASE;
        break;
    case KeyboardState::SYMBOLS_UPPERCASE:
        layout = en ? LAYOUT_EN_SYMBOLS_UPPERCASE : LAYOUT_SYMBOLS_UPPERCASE;
        break;
    default:
        layout = en ? LAYOUT_EN_LOWERCASE : LAYOUT_LOWERCASE;
        break;
    }

    const uint8_t *fontToUse = m_keyFont ? m_keyFont : ComfortaaBold18;
    const int available_width = Layout::KEYBOARD_WIDTH - (2 * Layout::PADDING);

    // FESTE Unit-Breite aus dem Grid-System
    const float unit_width = Layout::UNIT_WIDTH;

    for (int row = 0; row < Layout::KEYBOARD_ROWS; row++)
    {
        // Tasten dieser Reihe sammeln
        float total_units = 0;
        std::vector<const KeyDef *> row_keys;

        for (int col = 0; col < Layout::MAX_COLS; col++)
        {
            const KeyDef &key = layout[row][col];
            if (key.label == nullptr)
                break; // Ende der Reihe
            if (key.width_units > 0)
            {
                total_units += key.width_units;
                row_keys.push_back(&key);
            }
        }

        if (row_keys.empty())
            continue;

        int key_count = static_cast<int>(row_keys.size());

        // Tastenbreiten mit fester Unit-Breite berechnen
        std::vector<int> key_widths;
        key_widths.reserve(key_count);
        int total_row_width = 0;

        for (int i = 0; i < key_count; ++i)
        {
            float units = row_keys[i]->width_units;
            int width = static_cast<int>(std::round(units * unit_width + (units - 1.0f) * Layout::KEY_GAP));
            key_widths.push_back(width);
            total_row_width += width;
        }

        // Gaps zur Gesamtbreite addieren
        total_row_width += (key_count - 1) * Layout::KEY_GAP;

        // Tasten zeichnen - alle Reihen starten links am PADDING
        int current_x = m_posX + Layout::PADDING;
        int row_y = m_posY + Layout::KEYBOARD_Y_REL + row * (Layout::KEY_HEIGHT + Layout::KEY_GAP);

        for (int i = 0; i < key_count; ++i)
        {
            const KeyDef &key = *row_keys[i];
            int key_width = key_widths[i];

            // Leere Tasten (Spacer) nur als Platzhalter
            if (key.action == nullptr || strlen(key.action) == 0)
            {
                m_tft->fillRect(
                    current_x, row_y, key_width, Layout::KEY_HEIGHT,
                    Theme::ColorMainBackGround);
            }
            else
            {
                // Textfarbe für diese Taste bestimmen: Individuelle Farbe aus KeyDef hat Priorität, sonst State-abhängige Farbe
                uint16_t textColor = (key.labelColor != 0) ? key.labelColor : getKeyTextColor(key.action);

                // OK-Taste verwendet das per Setter konfigurierte Label, sonst Layout-Vorgabe
                const char *labelToUse = (strcmp(key.action, ACTION_OK) == 0) ? m_okLabel : key.label;

                auto btn = std::make_unique<Button>();
                btn->initButton(
                    m_tft,                          // TFT-Displayobjekt
                    current_x + key_width / 2,      // x-Position
                    row_y + Layout::KEY_HEIGHT / 2, // y-Position
                    key_width,                      // Weite
                    Layout::KEY_HEIGHT,             // Höhe
                    Layout::KEY_RADIUS,             // Eckenradius
                    Theme::ColorMainBackGround,     // Hintergrundfarbe
                    getKeyColor(key.action),        // Hintergrundfarbe beim drücken
                    textColor,                      // Textfarbe
                    labelToUse,                     // Beschriftung
                    key.icon,                       // Icon
                    1.0f,                           // horizontale Textgröße
                    1.0f);                          // vertikale Textgröße

                btn->setFont(fontToUse);

                // Label-Ausrichtung anwenden (für CAPSLOCK/SHIFT)
                if (key.labelDatum != MC_DATUM)
                {
                    // 20px Abstand zum Rand (5px Standard aus der Button-Klasse + 15px extra)
                    constexpr int LABEL_PADDING = 15;
                    int x_delta = (key.labelDatum == ML_DATUM) ? LABEL_PADDING : -LABEL_PADDING;
                    btn->setLabelDatum(x_delta, 0, key.labelDatum);
                }

                btn->drawButton(false);

                m_buttons.push_back(std::move(btn));
                m_buttonActions.push_back(key.action);
            }

            current_x += key_width + Layout::KEY_GAP;
        }
    }

    m_tft->endWrite();
}

/**
 * @brief Zeichnet das Eingabefeld mit Cursor
 */
void Keyboard::drawInputField()
{
    if (!m_inputSprite)
        return;

    // Sprite mit Hintergrundfarbe löschen, dann abgerundetes Rechteck zeichnen
    m_inputSprite->clear(Theme::ColorMainBackGround);
    m_inputSprite->fillSmoothRoundRect(
        0, 0,
        m_inputSprite->width(), Layout::INPUT_HEIGHT,
        Layout::KEY_RADIUS, Colors::KEY_SPECIAL);

    const uint8_t *fontToUse = m_inputFont ? m_inputFont : ComfortaaBold18;
    m_inputSprite->loadFont(fontToUse);
    m_inputSprite->setTextColor(Colors::TEXT);
    m_inputSprite->setTextDatum(lgfx::middle_left);

    constexpr int TEXT_OFFSET_X = 8;
    int text_y = Layout::INPUT_HEIGHT / 2;

    if (m_passwordMode && !m_passwordVisible)
    {
        // Passwort als Sterne anzeigen
        size_t len = strlen(m_inputBuffer);

        // Stack-Allokation mit fester Größe für Sicherheit
        char stars[256];
        size_t display_len = (len < sizeof(stars) - 1) ? len : sizeof(stars) - 1;
        memset(stars, '*', display_len);
        stars[display_len] = '\0';

        // Y-Korrektur für Sternchen
        text_y = Layout::INPUT_HEIGHT / 2 + 4;
        m_inputSprite->drawString(stars, TEXT_OFFSET_X, text_y);

        // Cursor zeichnen
        if (m_lastBlinkState)
        {
            size_t cursor_display = (m_cursorPos < display_len) ? m_cursorPos : display_len;
            char temp[256];
            memset(temp, '*', cursor_display);
            temp[cursor_display] = '\0';
            int cursor_x = TEXT_OFFSET_X + m_inputSprite->textWidth(temp);
            m_inputSprite->drawFastVLine(cursor_x, 6, Layout::INPUT_HEIGHT - 12, Colors::CURSOR);
        }
    }
    else
    {
        // Normalen Text anzeigen
        m_inputSprite->drawString(m_inputBuffer, TEXT_OFFSET_X, text_y);

        // Cursor zeichnen
        if (m_lastBlinkState)
        {
            char temp[256];
            size_t copy_len = (m_cursorPos < sizeof(temp) - 1) ? m_cursorPos : sizeof(temp) - 1;

            // Sicheres Kopieren für Teilstring
            if (copy_len > 0)
            {
                memcpy(temp, m_inputBuffer, copy_len);
            }
            temp[copy_len] = '\0';

            int cursor_x = TEXT_OFFSET_X + m_inputSprite->textWidth(temp);
            m_inputSprite->drawFastVLine(cursor_x, 6, Layout::INPUT_HEIGHT - 12, Colors::CURSOR);
        }
    }

    m_inputSprite->unloadFont();

    // Sprite auf Bildschirm übertragen Bei Passwort-Modus: Eingabefeld nach rechts verschieben (Toggle-Button ist links)
    int field_x = m_posX + Layout::PADDING;
    if (m_passwordMode)
    {
        field_x += Layout::INPUT_HEIGHT + Layout::KEY_GAP; // Button-Breite + Abstand
    }
    int field_y = m_posY + Layout::INPUT_Y_REL;
    m_inputSprite->pushSprite(field_x, field_y);
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch-Eingaben
 */
void Keyboard::handleTouch()
{
    // Startup-Guard - verhindert sofortige Reaktion beim Öffnen
    if (millis() - m_startTime < Timing::STARTUP_GUARD_MS)
    {
        return;
    }

    int32_t tx, ty;
    bool touched = m_tft->getTouch(&tx, &ty);
    uint32_t now = millis();

    if (touched)
    {
        if (!m_touchActive)
        {
            // Debounce-Check
            if (now - m_lastReleaseTime < Timing::TOUCH_DEBOUNCE_MS)
            {
                return;
            }

            m_touchActive = true;
            m_touchStartTime = now;

            // Touch im Eingabefeld?
            int field_x = m_posX + Layout::PADDING;
            if (m_passwordMode)
            {
                field_x += Layout::INPUT_HEIGHT + Layout::KEY_GAP; // Button-Breite + Abstand
            }
            int field_y = m_posY + Layout::INPUT_Y_REL;
            int field_w = m_inputSprite ? m_inputSprite->width() : 0;

            if (tx >= field_x && tx <= field_x + field_w &&
                ty >= field_y && ty <= field_y + Layout::INPUT_HEIGHT)
            {
                m_cursorPos = getCursorPositionFromTouch(tx);
                m_forceRedrawInput = true;
                return;
            }

            // Close-Button? - Sofortige Reaktion
            if (m_closeButton && m_closeButton->contains(tx, ty))
            {
                m_closeButton->drawButton(true);
                m_result = false;
                m_running = false;
                return;
            }

            // Password-Toggle? - Sofortige Reaktion
            if (m_togglePasswordButton && m_togglePasswordButton->contains(tx, ty))
            {
                m_passwordVisible = !m_passwordVisible;
                m_togglePasswordButton->setLabelText(m_passwordVisible ? "^EYEON:20^" : "^EYEOFF:20^");
                m_togglePasswordButton->drawButton(true);
                m_forceRedrawInput = true;
                return;
            }

            // Tastatur-Taste?
            for (size_t i = 0; i < m_buttons.size(); i++)
            {
                if (m_buttons[i]->contains(tx, ty))
                {
                    m_lastPressedButton = m_buttons[i].get();
                    m_lastAction = m_buttonActions[i];
                    m_touchStartTime = now;
                    m_lastRepeatTime = now;
                    m_lastPressedButton->drawButton(true);
                    processKey(m_lastAction);

                    // Bei repeatable Actions (Cursor, Backspace) sofort neu zeichnen
                    if (isRepeatableAction(m_lastAction) && m_forceRedrawInput)
                    {
                        drawInputField();
                        m_forceRedrawInput = false;
                    }
                    break;
                }
            }
        }
        else
        {
            // Touch wird gehalten - Auto-Repeat
            if (m_lastPressedButton && isRepeatableAction(m_lastAction))
            {
                if (now - m_touchStartTime > Timing::REPEAT_INITIAL_MS)
                {
                    if (now - m_lastRepeatTime > Timing::REPEAT_INTERVAL_MS)
                    {
                        processKey(m_lastAction);

                        // Sofort neu zeichnen für flüssiges Feedback
                        if (m_forceRedrawInput)
                        {
                            drawInputField();
                            m_forceRedrawInput = false;
                        }
                        m_lastRepeatTime = now;
                    }
                }
            }
        }
    }
    else
    {
        // Touch losgelassen
        if (m_touchActive)
        {
            m_touchActive = false;
            m_lastReleaseTime = now;

            // Password-Toggle zurücksetzen
            if (m_togglePasswordButton)
            {
                m_togglePasswordButton->drawButton(false);
            }

            if (m_lastPressedButton)
            {
                m_lastPressedButton->drawButton(false);
                m_lastPressedButton = nullptr;
            }
            m_lastAction = nullptr;
        }
    }
}

/**
 * @brief Verarbeitet eine Tastenaktion
 */
void Keyboard::processKey(const char *action)
{
    if (!action)
        return;

    if (strcmp(action, ACTION_BACKSPACE) == 0)
    {
        deleteCharacter();
    }
    else if (strcmp(action, ACTION_OK) == 0)
    {
        m_result = true;
        m_running = false;
    }
    else if (strcmp(action, ACTION_CANCEL) == 0)
    {
        m_result = false;
        m_running = false;
    }
    else if (strcmp(action, ACTION_SHIFT) == 0)
    {
        // Shift-Verhalten
        if (m_capsLock)
        {
            // CapsLock deaktivieren
            m_capsLock = false;
            if (isSymbolState())
            {
                switchState(KeyboardState::SYMBOLS_LOWERCASE);
            }
            else
            {
                switchState(KeyboardState::LOWERCASE);
            }
        }
        else if (m_state == KeyboardState::UPPERCASE)
        {
            switchState(KeyboardState::LOWERCASE);
        }
        else if (m_state == KeyboardState::LOWERCASE)
        {
            switchState(KeyboardState::UPPERCASE);
        }
        else if (m_state == KeyboardState::SYMBOLS_UPPERCASE)
        {
            switchState(KeyboardState::SYMBOLS_LOWERCASE);
        }
        else if (m_state == KeyboardState::SYMBOLS_LOWERCASE)
        {
            switchState(KeyboardState::SYMBOLS_UPPERCASE);
        }
    }
    else if (strcmp(action, ACTION_CAPSLOCK) == 0)
    {
        m_capsLock = !m_capsLock;
        if (isSymbolState())
        {
            // Im Symbol-Modus zwischen Lowercase und Uppercase wechseln
            switchState(m_capsLock ? KeyboardState::SYMBOLS_UPPERCASE : KeyboardState::SYMBOLS_LOWERCASE);
        }
        else
        {
            // Im Buchstaben-Modus
            switchState(m_capsLock ? KeyboardState::UPPERCASE : KeyboardState::LOWERCASE);
        }
    }
    else if (strcmp(action, ACTION_SYMBOLS) == 0)
    {
        // Wechsel zu Symbolen - CapsLock-Zustand berücksichtigen
        switchState(m_capsLock ? KeyboardState::SYMBOLS_UPPERCASE : KeyboardState::SYMBOLS_LOWERCASE);
    }
    else if (strcmp(action, ACTION_LETTERS) == 0)
    {
        // Zurück zu Buchstaben - CapsLock-Zustand wiederherstellen
        switchState(m_capsLock ? KeyboardState::UPPERCASE : KeyboardState::LOWERCASE);
    }
    else if (strcmp(action, ACTION_SPACE) == 0)
    {
        insertText(" ");
    }
    else if (strcmp(action, ACTION_LEFT) == 0)
    {
        moveCursor(-1);
    }
    else if (strcmp(action, ACTION_RIGHT) == 0)
    {
        moveCursor(1);
    }
    else
    {
        // Normales Zeichen einfügen
        insertText(action);

        // Nach Eingabe: Shift zurücksetzen (außer bei Caps Lock)
        if (!m_capsLock)
        {
            if (m_state == KeyboardState::UPPERCASE)
            {
                switchState(KeyboardState::LOWERCASE);
            }
            else if (m_state == KeyboardState::SYMBOLS_UPPERCASE)
            {
                switchState(KeyboardState::SYMBOLS_LOWERCASE);
            }
        }
    }
}

// ===================================================================
// Texteingabe
// ===================================================================

/**
 * @brief Fügt Text an der Cursor-Position ein
 */
void Keyboard::insertText(const char *text)
{
    if (!text)
        return;

    size_t text_len = strlen(text);
    size_t current_len = strlen(m_inputBuffer);

    if (current_len + text_len < m_bufferSize)
    {
        // Text nach Cursor verschieben
        memmove(
            &m_inputBuffer[m_cursorPos + text_len],
            &m_inputBuffer[m_cursorPos],
            current_len - m_cursorPos + 1);

        // Neuen Text einfügen
        memcpy(&m_inputBuffer[m_cursorPos], text, text_len);
        m_cursorPos += text_len;
        m_forceRedrawInput = true;
    }
}

/**
 * @brief Löscht das Zeichen vor dem Cursor
 */
void Keyboard::deleteCharacter()
{
    if (m_cursorPos > 0)
    {
        size_t text_len = strlen(m_inputBuffer);
        memmove(
            &m_inputBuffer[m_cursorPos - 1],
            &m_inputBuffer[m_cursorPos],
            text_len - m_cursorPos + 1);
        m_cursorPos--;
        m_forceRedrawInput = true;
    }
}

/**
 * @brief Bewegt den Cursor in die angegebene Richtung
 */
void Keyboard::moveCursor(int direction)
{
    if (direction < 0 && m_cursorPos > 0)
    {
        m_cursorPos--;
        m_forceRedrawInput = true;
    }
    else if (direction > 0 && m_cursorPos < strlen(m_inputBuffer))
    {
        m_cursorPos++;
        m_forceRedrawInput = true;
    }
}

/**
 * @brief Berechnet die Cursor-Position basierend auf Touch-Koordinate
 */
size_t Keyboard::getCursorPositionFromTouch(int touch_x)
{
    constexpr int TEXT_OFFSET_X = 8;

    // Basis-Position des Eingabefelds
    int field_x = m_posX + Layout::PADDING;
    if (m_passwordMode)
    {
        field_x += Layout::INPUT_HEIGHT + Layout::KEY_GAP; // Button-Breite + Abstand
    }

    int relative_x = touch_x - field_x - TEXT_OFFSET_X;
    if (relative_x <= 0)
        return 0;

    const uint8_t *fontToUse = m_inputFont ? m_inputFont : ComfortaaBold18;
    m_inputSprite->loadFont(fontToUse);

    size_t text_len = strlen(m_inputBuffer);

    // Passwort-Modus: Alle Zeichen gleich breit
    if (m_passwordMode && !m_passwordVisible)
    {
        int star_width = m_inputSprite->textWidth("*");
        if (star_width > 0)
        {
            size_t pos = static_cast<size_t>(std::round(static_cast<float>(relative_x) / star_width));
            m_inputSprite->unloadFont();
            return (pos > text_len) ? text_len : pos;
        }
    }

    // Normale Textmodus: Beste Position finden
    size_t best_pos = 0;
    int best_distance = std::numeric_limits<int>::max();

    char temp[256];
    for (size_t i = 0; i <= text_len && i < sizeof(temp); i++)
    {
        // Sicheres Kopieren für Teilstring
        if (i > 0)
        {
            memcpy(temp, m_inputBuffer, i);
        }
        temp[i] = '\0';

        int text_width = m_inputSprite->textWidth(temp);
        int distance = std::abs(relative_x - text_width);

        if (distance < best_distance)
        {
            best_distance = distance;
            best_pos = i;
        }
    }

    m_inputSprite->unloadFont();
    return best_pos;
}

// ===================================================================
// Zustandswechsel
// ===================================================================

/**
 * @brief Wechselt den Tastatur-Zustand
 */
void Keyboard::switchState(KeyboardState newState)
{
    if (m_state == newState)
        return;
    m_state = newState;
    setupAndDrawLayout();
}

// ===================================================================
// Hilfsfunktionen
// ===================================================================

/**
 * @brief Gibt die Hintergrundfarbe für eine Taste zurück
 */
uint16_t Keyboard::getKeyColor(const char *action) const
{
    if (!action)
        return Colors::KEY_NORMAL;

    // OK-Taste hervorheben
    if (strcmp(action, ACTION_OK) == 0)
        return Colors::KEY_SPECIAL;

    // Shift-Taste aktiv? (auch in Symbol-States)
    if (strcmp(action, ACTION_SHIFT) == 0)
    {
        if (m_state == KeyboardState::UPPERCASE || m_state == KeyboardState::SYMBOLS_UPPERCASE)
        {
            return Colors::KEY_SHIFT_ACTIVE;
        }
    }

    // Caps-Taste aktiv?
    if (strcmp(action, ACTION_CAPSLOCK) == 0 && m_capsLock)
        return Colors::KEY_SHIFT_ACTIVE;

    // Spezial-Tasten
    if (isSpecialAction(action))
        return Colors::KEY_SPECIAL;

    return Colors::KEY_NORMAL;
}

/**
 * @brief Gibt die Textfarbe für eine Taste zurück
 */
uint16_t Keyboard::getKeyTextColor(const char *action) const
{
    // Spezial-Tasten bekommen immer die Standard-Textfarbe
    if (isSpecialAction(action))
        return Colors::TEXT;

    // Symbol-States: Konfigurierbare Farbe für normale Tasten
    if (m_state == KeyboardState::SYMBOLS_LOWERCASE)
        return m_symbolsLowercaseLabelColor;

    if (m_state == KeyboardState::SYMBOLS_UPPERCASE)
        return m_symbolsUppercaseLabelColor;

    // Standard: Weiß
    return Colors::TEXT;
}

/**
 * @brief Prüft ob eine Aktion eine Spezial-Aktion ist
 */
bool Keyboard::isSpecialAction(const char *action) const
{
    if (!action)
        return false;

    return (strcmp(action, ACTION_BACKSPACE) == 0 ||
            strcmp(action, ACTION_SHIFT) == 0 ||
            strcmp(action, ACTION_CAPSLOCK) == 0 ||
            strcmp(action, ACTION_SYMBOLS) == 0 ||
            strcmp(action, ACTION_LETTERS) == 0 ||
            strcmp(action, ACTION_SPACE) == 0 ||
            strcmp(action, ACTION_CANCEL) == 0 ||
            strcmp(action, ACTION_LEFT) == 0 ||
            strcmp(action, ACTION_RIGHT) == 0);
}

/**
 * @brief Prüft ob eine Aktion wiederholbar ist (Auto-Repeat)
 */
bool Keyboard::isRepeatableAction(const char *action) const
{
    if (!action)
        return false;

    return (strcmp(action, ACTION_BACKSPACE) == 0 ||
            strcmp(action, ACTION_LEFT) == 0 ||
            strcmp(action, ACTION_RIGHT) == 0);
}

/**
 * @brief Prüft ob aktueller State ein Symbol-State ist
 */
bool Keyboard::isSymbolState() const
{
    return (m_state == KeyboardState::SYMBOLS_LOWERCASE ||
            m_state == KeyboardState::SYMBOLS_UPPERCASE);
}
