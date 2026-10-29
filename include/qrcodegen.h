/*
 * QR-Code-Generator-Bibliothek (C)
 *
 * Copyright (c) Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/qr-code-generator-library
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * - The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, express or
 *   implied, including but not limited to the warranties of merchantability,
 *   fitness for a particular purpose and noninfringement. In no event shall the
 *   authors or copyright holders be liable for any claim, damages or other
 *   liability, whether in an action of contract, tort or otherwise, arising from,
 *   out of or in connection with the Software or the use or other dealings in the
 *   Software.
 */

#ifndef QRCODEGEN_H
#define QRCODEGEN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

	/*
	 * Diese Bibliothek erzeugt QR-Code-Symbole gemäß ISO/IEC 18004 (Model 2).
	 * Ein QR-Code ist ein unveränderliches quadratisches Raster aus dunklen und
	 * hellen Modulen. Die Bibliothek unterstützt alle Versionen 1–40, alle vier
	 * Fehlerkorrektur-Stufen und die vier Kodierungsmodi (Numerisch, Alphanumerisch,
	 * Byte, Kanji).
	 *
	 * Wege zur Erzeugung eines QR-Codes:
	 * - High-Level: Nutzdaten übergeben und qrcodegen_encodeText() oder
	 *   qrcodegen_encodeBinary() aufrufen.
	 * - Low-Level: Segmente manuell zusammenstellen und
	 *   qrcodegen_encodeSegments() oder qrcodegen_encodeSegmentsAdvanced() aufrufen.
	 *   (Alle Wege erfordern die gewünschte Fehlerkorrektur-Stufe und Byte-Puffer.)
	 */

	/* =========================================================================
	   Enum- und Struct-Typen
	   ========================================================================= */

	/**
	 * @brief Fehlerkorrektur-Stufe eines QR-Code-Symbols.
	 *
	 * Muss in aufsteigender Reihenfolge des Schutzniveaus deklariert sein,
	 * damit eine interne Bibliotheksfunktion korrekt arbeitet.
	 */
	enum qrcodegen_Ecc
	{
		qrcodegen_Ecc_LOW = 0,	///< ~7 % fehlerhafte Codewörter tolerierbar
		qrcodegen_Ecc_MEDIUM,	///< ~15 % fehlerhafte Codewörter tolerierbar
		qrcodegen_Ecc_QUARTILE, ///< ~25 % fehlerhafte Codewörter tolerierbar
		qrcodegen_Ecc_HIGH,		///< ~30 % fehlerhafte Codewörter tolerierbar
	};

	/**
	 * @brief Maskenmuster eines QR-Code-Symbols.
	 */
	enum qrcodegen_Mask
	{
		qrcodegen_Mask_AUTO = -1, ///< Automatische Wahl des optimalen Musters
		qrcodegen_Mask_0 = 0,	  ///< Muster 0
		qrcodegen_Mask_1,		  ///< Muster 1
		qrcodegen_Mask_2,		  ///< Muster 2
		qrcodegen_Mask_3,		  ///< Muster 3
		qrcodegen_Mask_4,		  ///< Muster 4
		qrcodegen_Mask_5,		  ///< Muster 5
		qrcodegen_Mask_6,		  ///< Muster 6
		qrcodegen_Mask_7,		  ///< Muster 7
	};

	/**
	 * @brief Kodierungsmodus eines Segments – bestimmt die Interpretation der Datenbits.
	 */
	enum qrcodegen_Mode
	{
		qrcodegen_Mode_NUMERIC = 0x1,	   ///< Nur Dezimalziffern 0–9
		qrcodegen_Mode_ALPHANUMERIC = 0x2, ///< Ziffern, Großbuchstaben und Sonderzeichen
		qrcodegen_Mode_BYTE = 0x4,		   ///< Beliebige Bytes (z. B. UTF-8)
		qrcodegen_Mode_KANJI = 0x8,		   ///< Doppelbyte-Kanji (Shift-JIS)
		qrcodegen_Mode_ECI = 0x7,		   ///< Extended Channel Interpretation
	};

	/**
	 * @brief Ein Datensegment innerhalb eines QR-Code-Symbols.
	 *
	 * Auf mittlerem Niveau wird ein Segment durch Aufruf einer Factory-Funktion
	 * wie qrcodegen_makeNumeric() erzeugt. Auf unterem Niveau kann der Bit-Puffer
	 * manuell befüllt und das Struct direkt initialisiert werden.
	 *
	 * Maximale Nutzdaten: 7089 Zeichen (QR-Code-Grenzwert). Segmente die länger
	 * sind haben keine sinnvolle Verwendung. Die maximale Bit-Länge beträgt 32767,
	 * da der größte QR-Code (Version 40) 31329 Module besitzt.
	 */
	struct qrcodegen_Segment
	{
		enum qrcodegen_Mode mode; ///< Modus-Indikator dieses Segments

		/**
		 * Länge der unkomprimierten Nutzdaten in Zeichen (Numerisch/Alphanumerisch/Kanji)
		 * bzw. Bytes (Byte-Modus) bzw. 0 (ECI-Modus). Stets ≥ 0.
		 * Entspricht NICHT der Bit-Länge der kodierten Daten.
		 */
		int numChars;

		/**
		 * Datenbits dieses Segments, bitweise Big-Endian gepackt.
		 * Darf NULL sein wenn bitLength == 0.
		 */
		uint8_t *data;

		/**
		 * Anzahl gültiger Datenbits im Puffer.
		 * Bedingungen: 0 <= bitLength <= 32767 und bitLength <= (Kapazität von data) * 8.
		 * numChars muss mit Modus und Bit-Puffer-Länge übereinstimmen.
		 */
		int bitLength;
	};

/* =========================================================================
   Makro-Konstanten und -Funktionen
   ========================================================================= */

/** @brief Kleinste unterstützte Versionsnummer (QR-Code Model 2). */
#define qrcodegen_VERSION_MIN 1

/** @brief Größte unterstützte Versionsnummer (QR-Code Model 2). */
#define qrcodegen_VERSION_MAX 40

/**
 * @brief Berechnet zur Compile-Zeit die Byte-Anzahl für einen QR-Code bis zur
 *        angegebenen Version (einschließlich).
 * @param n Maximale Version (qrcodegen_VERSION_MIN <= n <= qrcodegen_VERSION_MAX)
 * @return Byte-Anzahl als Compile-Zeit-Konstante, passend für ein uint8_t-Array.
 *
 * Beispiel: `uint8_t puffer[qrcodegen_BUFFER_LEN_FOR_VERSION(25)];`
 * kann jeden QR-Code von Version 1 bis 25 aufnehmen. Das Ergebnis passt in int.
 */
#define qrcodegen_BUFFER_LEN_FOR_VERSION(n) ((((n) * 4 + 17) * ((n) * 4 + 17) + 7) / 8 + 1)

/**
 * @brief Worst-Case-Byte-Anzahl für einen QR-Code bis Version 40 (= 3918 Bytes, knapp unter 4 KB).
 *
 * Bequemer Pauschalwert – vermeidet das manuelle Berechnen enger Puffergrenzen.
 */
#define qrcodegen_BUFFER_LEN_MAX qrcodegen_BUFFER_LEN_FOR_VERSION(qrcodegen_VERSION_MAX)

	/* =========================================================================
	   High-Level-Funktionen zur QR-Code-Erzeugung
	   ========================================================================= */

	/**
	 * @brief Kodiert einen UTF-8-Text als QR-Code.
	 * @param text        Eingabetext (UTF-8, keine NUL-Zeichen)
	 * @param tempBuffer  Arbeitspuffer, Länge ≥ qrcodegen_BUFFER_LEN_FOR_VERSION(maxVersion)
	 * @param qrcode      Ausgabepuffer, Länge ≥ qrcodegen_BUFFER_LEN_FOR_VERSION(maxVersion)
	 * @param ecl         Gewünschte Fehlerkorrektur-Stufe
	 * @param minVersion  Kleinste erlaubte Version (1–40)
	 * @param maxVersion  Größte erlaubte Version (minVersion–40)
	 * @param mask        Maskenmuster (0–7) oder qrcodegen_Mask_AUTO
	 * @param boostEcl    true = ECC-Stufe erhöhen wenn ohne Versions-Anstieg möglich
	 * @return true bei Erfolg, false wenn die Daten nicht in den Versionsbereich passen
	 *
	 * Die kleinste passende Version innerhalb [minVersion, maxVersion] wird automatisch
	 * gewählt. tempBuffer und qrcode dürfen sich nicht überlappen. Beide Puffer müssen
	 * vor dem Aufruf nicht initialisiert sein – die Funktion schreibt stets vor dem Lesen.
	 * Nach Rückkehr enthält tempBuffer keine nutzbaren Daten. Bei Erfolg kann qrcode an
	 * qrcodegen_getSize() und qrcodegen_getModule() übergeben werden.
	 */
	bool qrcodegen_encodeText(const char *text, uint8_t tempBuffer[], uint8_t qrcode[], enum qrcodegen_Ecc ecl, int minVersion, int maxVersion, enum qrcodegen_Mask mask, bool boostEcl);

	/**
	 * @brief Kodiert beliebige Binärdaten als QR-Code (Byte-Modus).
	 * @param dataAndTemp Eingabedaten und gleichzeitig Arbeitspuffer;
	 *                    Länge ≥ qrcodegen_BUFFER_LEN_FOR_VERSION(maxVersion)
	 * @param dataLen     Anzahl der Eingabe-Bytes
	 * @param qrcode      Ausgabepuffer, Länge ≥ qrcodegen_BUFFER_LEN_FOR_VERSION(maxVersion)
	 * @param ecl         Gewünschte Fehlerkorrektur-Stufe
	 * @param minVersion  Kleinste erlaubte Version (1–40)
	 * @param maxVersion  Größte erlaubte Version (minVersion–40)
	 * @param mask        Maskenmuster (0–7) oder qrcodegen_Mask_AUTO
	 * @param boostEcl    true = ECC-Stufe erhöhen wenn ohne Versions-Anstieg möglich
	 * @return true bei Erfolg, false wenn die Daten nicht in den Versionsbereich passen
	 *
	 * dataAndTemp wird als Eingabe- und Arbeitspuffer gleichzeitig genutzt; der Inhalt
	 * nach Rückkehr ist undefiniert. Bei Version 40 / ECC-Low können bis zu 2953 Bytes
	 * kodiert werden (absolutes QR-Code-Maximum). Bei Erfolg kann qrcode an
	 * qrcodegen_getSize() und qrcodegen_getModule() übergeben werden.
	 */
	bool qrcodegen_encodeBinary(uint8_t dataAndTemp[], size_t dataLen, uint8_t qrcode[], enum qrcodegen_Ecc ecl, int minVersion, int maxVersion, enum qrcodegen_Mask mask, bool boostEcl);

	/* =========================================================================
	   Low-Level-Funktionen zur QR-Code-Erzeugung
	   ========================================================================= */

	/**
	 * @brief Kodiert eine Folge von Segmenten als QR-Code (einfache Variante).
	 * @param segs        Array der Segmente
	 * @param len         Anzahl der Segmente
	 * @param ecl         Gewünschte Fehlerkorrektur-Stufe
	 * @param tempBuffer  Arbeitspuffer, Länge ≥ qrcodegen_BUFFER_LEN_MAX;
	 *                    Datenpuffer einzelner Segmente dürfen sich mit tempBuffer überlappen
	 * @param qrcode      Ausgabepuffer, Länge ≥ qrcodegen_BUFFER_LEN_MAX
	 * @return true bei Erfolg, false wenn die Daten nicht in irgendeine Version passen
	 *
	 * Wählt automatisch die kleinste passende Version. ECC-Stufe kann erhöht werden
	 * ohne die Version zu steigern (boostEcl = true implizit). Low-Level-API –
	 * High-Level-Äquivalent: qrcodegen_encodeText() / qrcodegen_encodeBinary().
	 * Erlaubt benutzerdefinierte Segment-Sequenzen mit Moduswechseln (z. B. zwischen
	 * Alphanumerisch und Byte) zur platzsparenden Kodierung.
	 */
	bool qrcodegen_encodeSegments(const struct qrcodegen_Segment segs[], size_t len, enum qrcodegen_Ecc ecl, uint8_t tempBuffer[], uint8_t qrcode[]);

	/**
	 * @brief Kodiert eine Folge von Segmenten als QR-Code (erweiterte Variante).
	 * @param segs        Array der Segmente
	 * @param len         Anzahl der Segmente
	 * @param ecl         Gewünschte Fehlerkorrektur-Stufe
	 * @param minVersion  Kleinste erlaubte Version (1–40)
	 * @param maxVersion  Größte erlaubte Version (minVersion–40)
	 * @param mask        Maskenmuster (0–7) oder qrcodegen_Mask_AUTO
	 * @param boostEcl    true = ECC-Stufe erhöhen wenn ohne Versions-Anstieg möglich
	 * @param tempBuffer  Arbeitspuffer, Länge ≥ qrcodegen_BUFFER_LEN_MAX;
	 *                    Datenpuffer einzelner Segmente dürfen sich mit tempBuffer überlappen
	 * @param qrcode      Ausgabepuffer, Länge ≥ qrcodegen_BUFFER_LEN_MAX
	 * @return true bei Erfolg, false wenn die Daten nicht in den Versionsbereich passen
	 *
	 * Wählt die kleinste passende Version in [minVersion, maxVersion]. Segmente mit
	 * Datenpuffern die sich mit tempBuffer überlappen haben nach Rückkehr ungültige Werte.
	 * Bei Erfolg kann qrcode an qrcodegen_getSize() und qrcodegen_getModule() übergeben werden.
	 */
	bool qrcodegen_encodeSegmentsAdvanced(const struct qrcodegen_Segment segs[], size_t len, enum qrcodegen_Ecc ecl, int minVersion, int maxVersion, enum qrcodegen_Mask mask, bool boostEcl, uint8_t tempBuffer[], uint8_t qrcode[]);

	/**
	 * @brief Prüft ob der Text ausschließlich im Numerisch-Modus kodierbar ist.
	 * @param text Zu prüfender Text
	 * @return true wenn jedes Zeichen im Bereich '0'–'9' liegt
	 */
	bool qrcodegen_isNumeric(const char *text);

	/**
	 * @brief Prüft ob der Text im Alphanumerisch-Modus kodierbar ist.
	 * @param text Zu prüfender Text
	 * @return true wenn jedes Zeichen aus dem erlaubten Zeichensatz stammt:
	 *         0–9, A–Z (nur Großbuchstaben), Leerzeichen, $ % * + - . / :
	 */
	bool qrcodegen_isAlphanumeric(const char *text);

	/**
	 * @brief Berechnet die benötigte Puffergröße (Bytes) für ein Segment.
	 * @param mode     Kodierungsmodus
	 * @param numChars Anzahl der Zeichen (Byte-Modus: Byte-Anzahl, ECI-Modus: muss 0 sein)
	 * @return Erforderliche Byte-Anzahl, oder SIZE_MAX bei Überlauf
	 *         (numChars > INT16_MAX oder berechnete Bit-Anzahl > 32767).
	 *         Gültige Ergebnisse liegen in [0, 4096]. Mehr Bytes zu allokieren ist erlaubt.
	 *         Im ECI-Modus wird der Worst-Case zurückgegeben; tatsächliche Segmente
	 *         können kürzer sein. Für alle anderen Modi ist das Ergebnis exakt.
	 */
	size_t qrcodegen_calcSegmentBufferSize(enum qrcodegen_Mode mode, size_t numChars);

	/**
	 * @brief Erzeugt ein Byte-Modus-Segment aus beliebigen Binärdaten.
	 * @param data Eingabe-Byte-Array (beliebiger Inhalt; UTF-8-Text ebenfalls möglich)
	 * @param len  Anzahl der Bytes
	 * @param buf  Ausgabepuffer, Größe ≥ qrcodegen_calcSegmentBufferSize(BYTE, len)
	 * @return Befülltes qrcodegen_Segment-Struct
	 */
	struct qrcodegen_Segment qrcodegen_makeBytes(const uint8_t data[], size_t len, uint8_t buf[]);

	/**
	 * @brief Erzeugt ein Numerisch-Modus-Segment aus einer Dezimalziffern-Zeichenkette.
	 * @param digits Nur Ziffern '0'–'9'; NUL-terminiert
	 * @param buf    Ausgabepuffer, Größe ≥ qrcodegen_calcSegmentBufferSize(NUMERIC, strlen(digits))
	 * @return Befülltes qrcodegen_Segment-Struct
	 */
	struct qrcodegen_Segment qrcodegen_makeNumeric(const char *digits, uint8_t buf[]);

	/**
	 * @brief Erzeugt ein Alphanumerisch-Modus-Segment aus einer Zeichenkette.
	 * @param text Erlaubte Zeichen: 0–9, A–Z (nur Groß), Leerzeichen, $ % * + - . / :
	 * @param buf  Ausgabepuffer, Größe ≥ qrcodegen_calcSegmentBufferSize(ALPHANUMERIC, strlen(text))
	 * @return Befülltes qrcodegen_Segment-Struct
	 */
	struct qrcodegen_Segment qrcodegen_makeAlphanumeric(const char *text, uint8_t buf[]);

	/**
	 * @brief Erzeugt ein ECI-Segment (Extended Channel Interpretation).
	 * @param assignVal ECI-Zuweisungswert (0–999999)
	 * @param buf       Ausgabepuffer, Größe ≥ qrcodegen_calcSegmentBufferSize(ECI, 0)
	 * @return Befülltes qrcodegen_Segment-Struct
	 */
	struct qrcodegen_Segment qrcodegen_makeEci(long assignVal, uint8_t buf[]);

	/* =========================================================================
	   Abfrage-Funktionen für fertige QR-Codes
	   ========================================================================= */

	/**
	 * @brief Gibt die Seitenlänge (Modul-Anzahl) des QR-Codes zurück.
	 * @param qrcode Erfolgreich kodierter QR-Code-Puffer
	 * @return Seitenlänge in Modulen, Bereich [21, 177].
	 *         Der Puffer muss Länge ≥ qrcodegen_BUFFER_LEN_FOR_VERSION(version) haben,
	 *         wobei version = (size - 17) / 4.
	 */
	int qrcodegen_getSize(const uint8_t qrcode[]);

	/**
	 * @brief Gibt die Farbe des Moduls an Position (x, y) zurück.
	 * @param qrcode Erfolgreich kodierter QR-Code-Puffer
	 * @param x      Spalte (0 = links)
	 * @param y      Zeile  (0 = oben)
	 * @return true = dunkles Modul, false = helles Modul.
	 *         Bei Koordinaten außerhalb der Grenzen wird false zurückgegeben.
	 */
	bool qrcodegen_getModule(const uint8_t qrcode[], int x, int y);

#ifdef __cplusplus
}
#endif

#endif /* QRCODEGEN_H */