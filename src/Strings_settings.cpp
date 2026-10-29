#include "Strings_settings.h"
#include "I18n.h"

#include <pgmspace.h>

// ===================================================================
// PROGMEM-Literale (Deutsch)
// ===================================================================

namespace
{
    const char DE_EMPTY[] PROGMEM = "";

    const char DE_SEC_CONNECTION[] PROGMEM = "Verbindung";
    const char DE_SEC_SERVICES[] PROGMEM = "Dienste";
    const char DE_SEC_SYSTEM[] PROGMEM = "System";

    const char DE_MENU_WIFI[] PROGMEM = "WLAN";
    const char DE_MENU_TIME[] PROGMEM = "Zeit & Zeitserver";
    const char DE_MENU_TIME_WINDOW[] PROGMEM = "Zeitfenster";
    const char DE_MENU_WEATHER[] PROGMEM = "Wetter";
    const char DE_MENU_NEWS[] PROGMEM = "Nachrichten";
    const char DE_MENU_CLOUD[] PROGMEM = "Cloud-Anbindung";
    const char DE_MENU_LANGUAGE[] PROGMEM = "Sprache";
    const char DE_MENU_SECURITY[] PROGMEM = "Sicherheit";
    const char DE_MENU_AUDIO[] PROGMEM = "Klang";
    const char DE_MENU_APPEARANCE[] PROGMEM = "Anzeige";
    const char DE_MENU_INFO[] PROGMEM = "Info & Neustart";

    const char DE_FOOTER_SAVE[] PROGMEM = "speichern";

    const char DE_PLACEHOLDER_TITLE[] PROGMEM = "In Vorbereitung";
    const char DE_PLACEHOLDER_MESSAGE[] PROGMEM = "Dieser Bereich wird gerade gebaut.";

    const char DE_WIFI_TITLE[] PROGMEM = "WLAN";
    const char DE_WIFI_SUBTITLE[] PROGMEM = "Profile einzeln aktivierbar";

    const char DE_WIFI_PROFILE_FMT[] PROGMEM = "Profil {0:%d}";
    const char DE_WIFI_EMPTY_NAME[] PROGMEM = "(leer)";
    const char DE_WIFI_STATUS_CONNECTED[] PROGMEM = "verbunden";
    const char DE_WIFI_STATUS_ACTIVE[] PROGMEM = "aktiv (bereit)";
    const char DE_WIFI_STATUS_INACTIVE[] PROGMEM = "inaktiv";
    const char DE_WIFI_STATUS_EMPTY[] PROGMEM = "kein Netz hinterlegt";

    const char DE_WIFI_ACT_SCAN[] PROGMEM = "scannen";
    const char DE_WIFI_ACT_PASSWORD[] PROGMEM = "passwort";
    const char DE_WIFI_ACT_TEST[] PROGMEM = "test";
    const char DE_WIFI_ACT_DELETE[] PROGMEM = "löschen";

    const char DE_WIFI_SCAN_TITLE[] PROGMEM = "Netzwerk wählen:";
    const char DE_WIFI_PW_TITLE_FMT[] PROGMEM = "Passwort für {0}:";

    const char DE_WIFI_NEED_SSID_TITLE[] PROGMEM = "Kein Netz gewählt";
    const char DE_WIFI_NEED_SSID_MSG[] PROGMEM = "Bitte zuerst ein Netzwerk wählen.";

    const char DE_WIFI_PW_BAD_TITLE[] PROGMEM = "Ungültiges Passwort";
    const char DE_WIFI_PW_ERR_LEN[] PROGMEM = "Passwort muss 8 bis 63 Zeichen lang sein.";
    const char DE_WIFI_PW_ERR_CHARS[] PROGMEM = "Bitte nur normale Tastaturzeichen verwenden.";
    const char DE_WIFI_PW_ERR_HEX[] PROGMEM = "64 Zeichen erlauben nur die Ziffern 0-9 und A-F.";
    const char DE_WIFI_PW_ERR_INTERNAL[] PROGMEM = "Interner Fehler.";

    const char DE_WIFI_DELETE_TITLE[] PROGMEM = "Profil löschen";
    const char DE_WIFI_DELETE_MSG[] PROGMEM = "Dieses Profil wirklich dauerhaft löschen?";

    const char DE_WIFI_TEST_BOX_TITLE[] PROGMEM = "Verbindungstest";
    const char DE_WIFI_TEST_PREPARE[] PROGMEM = "Verbindung wird vorbereitet...";
    const char DE_WIFI_TEST_RUNNING[] PROGMEM = "Verbindung wird getestet...";
    const char DE_WIFI_TEST_OK_TITLE[] PROGMEM = "Test erfolgreich";
    const char DE_WIFI_TEST_OK_FMT[] PROGMEM = "Verbunden.\n\nIP: {0}\nSignal: {1:%d} dBm";
    const char DE_WIFI_TEST_FAIL_TITLE[] PROGMEM = "Test fehlgeschlagen";
    const char DE_WIFI_TEST_FAIL_MSG[] PROGMEM = "Verbindung fehlgeschlagen.\nSSID und Passwort prüfen.";
    const char DE_WIFI_TEST_TIMEOUT_MSG[] PROGMEM = "Zeitüberschreitung.\nSSID und Passwort prüfen.";

    const char DE_WIFI_HOSTNAME_LABEL[] PROGMEM = "Netzwerkname";
    const char DE_WIFI_HOSTNAME_PROMPT[] PROGMEM = "Netzwerkname:";
    const char DE_WIFI_HOSTNAME_EDIT[] PROGMEM = "ändern";
    const char DE_WIFI_HOSTNAME_BAD_TITLE[] PROGMEM = "Ungültiger Name";
    const char DE_WIFI_HOSTNAME_ERR[] PROGMEM = "Erlaubt sind Buchstaben, Ziffern und Bindestrich, 1 bis 32 Zeichen. Der Name darf nicht mit einem Bindestrich beginnen oder enden.";

    const char DE_SAVED_TITLE[] PROGMEM = "Gespeichert";
    const char DE_SAVED_MSG[] PROGMEM = "Einstellungen gespeichert.\nEin Neustart übernimmt die Änderungen.";
    const char DE_SAVED_RESTART_BTN[] PROGMEM = "neustart";
    const char DE_SAVED_LATER_BTN[] PROGMEM = "später";

    const char DE_EXIT_TITLE[] PROGMEM = "Ungespeicherte Änderungen";
    const char DE_EXIT_MSG[] PROGMEM = "Änderungen speichern?";
    const char DE_EXIT_DISCARD_BTN[] PROGMEM = "verwerfen";

    const char DE_LANG_TITLE[] PROGMEM = "Sprache";
    const char DE_LANG_SUBTITLE[] PROGMEM = "Sprache der Oberfläche";
    const char DE_LANG_ACTIVE[] PROGMEM = "aktiv";
    const char DE_LANG_NAME_DE[] PROGMEM = "Deutsch";
    const char DE_LANG_NAME_EN[] PROGMEM = "English";

    const char DE_SEC_TITLE[] PROGMEM = "Sicherheit";
    const char DE_SEC_SUBTITLE[] PROGMEM = "Sperr-PIN ändern";
    const char DE_SEC_PIN_HINT[] PROGMEM = "Tippe 4 Ziffern für eine neue PIN.";
    const char DE_SEC_PIN_SET[] PROGMEM = "Eine PIN ist gesetzt.";
    const char DE_SEC_SAVED_TITLE[] PROGMEM = "PIN geändert";
    const char DE_SEC_SAVED_MSG[] PROGMEM = "Die neue PIN wurde gespeichert.";
    const char DE_SEC_PIN_REPEAT[] PROGMEM = "PIN wiederholen";
    const char DE_SEC_PIN_MISMATCH[] PROGMEM = "Eingaben stimmen nicht überein";
    const char DE_SEC_PIN_READY[] PROGMEM = "PIN vorgemerkt. Mit Speichern bestätigen.";

    const char DE_INFO_TITLE[] PROGMEM = "Info & Neustart";
    const char DE_INFO_SUBTITLE[] PROGMEM = "Geräteinformationen";
    const char DE_INFO_FIRMWARE[] PROGMEM = "Version";
    const char DE_INFO_UPTIME[] PROGMEM = "Laufzeit";
    const char DE_INFO_RESTART_BTN[] PROGMEM = "neustart";
    const char DE_INFO_RESTART_TITLE[] PROGMEM = "Neustart";
    const char DE_INFO_RESTART_MSG[] PROGMEM = "Wirklich neu starten?\nUngespeicherte Änderungen gehen verloren.";
    const char DE_INFO_RESET_BTN[] PROGMEM = "zurücksetzen";
    const char DE_INFO_RESET_TITLE[] PROGMEM = "Zurücksetzen";
    const char DE_INFO_RESET_MSG[] PROGMEM = "Auf Werkseinstellungen zurücksetzen?\nAlle Profile und Einstellungen gehen verloren.";
    const char DE_LANG_RESTART_TITLE[] PROGMEM = "Sprache ändern";
    const char DE_LANG_RESTART_MSG[] PROGMEM = "Die neue Sprache gilt erst nach einem Neustart.\nJetzt neu starten?";

    const char DE_NEWS_TITLE[] PROGMEM = "Nachrichten";
    const char DE_NEWS_SUBTITLE[] PROGMEM = "Themen und Bundesland";
    const char DE_NEWS_RESSORT_LABEL[] PROGMEM = "Themen";
    const char DE_NEWS_REGION_LABEL[] PROGMEM = "Bundesland";

    const char DE_NEWS_RESSORT_ALL[] PROGMEM = "alle";
    const char DE_NEWS_RESSORT_INLAND[] PROGMEM = "Inland";
    const char DE_NEWS_RESSORT_AUSLAND[] PROGMEM = "Ausland";
    const char DE_NEWS_RESSORT_WIRTSCHAFT[] PROGMEM = "Wirtschaft";
    const char DE_NEWS_RESSORT_SPORT[] PROGMEM = "Sport";
    const char DE_NEWS_RESSORT_VIDEO[] PROGMEM = "Video";
    const char DE_NEWS_RESSORT_INVESTIGATIV[] PROGMEM = "Investigativ";
    const char DE_NEWS_RESSORT_WISSEN[] PROGMEM = "Wissen";

    const char DE_NEWS_REGION_PICK_TITLE[] PROGMEM = "Bundesland wählen:";

    const char DE_NEWS_BL_BW[] PROGMEM = "Baden-Württemberg";
    const char DE_NEWS_BL_BY[] PROGMEM = "Bayern";
    const char DE_NEWS_BL_BE[] PROGMEM = "Berlin";
    const char DE_NEWS_BL_BB[] PROGMEM = "Brandenburg";
    const char DE_NEWS_BL_HB[] PROGMEM = "Bremen";
    const char DE_NEWS_BL_HH[] PROGMEM = "Hamburg";
    const char DE_NEWS_BL_HE[] PROGMEM = "Hessen";
    const char DE_NEWS_BL_MV[] PROGMEM = "Mecklenburg-Vorpommern";
    const char DE_NEWS_BL_NI[] PROGMEM = "Niedersachsen";
    const char DE_NEWS_BL_NW[] PROGMEM = "Nordrhein-Westfalen";
    const char DE_NEWS_BL_RP[] PROGMEM = "Rheinland-Pfalz";
    const char DE_NEWS_BL_SL[] PROGMEM = "Saarland";
    const char DE_NEWS_BL_SN[] PROGMEM = "Sachsen";
    const char DE_NEWS_BL_ST[] PROGMEM = "Sachsen-Anhalt";
    const char DE_NEWS_BL_SH[] PROGMEM = "Schleswig-Holstein";
    const char DE_NEWS_BL_TH[] PROGMEM = "Thüringen";

    const char DE_NEWS_SAVED_MSG[] PROGMEM = "Die Filter für die Nachrichten wurden gespeichert.";

    const char DE_WEATHER_TITLE[] PROGMEM = "Wetter";
    const char DE_WEATHER_SUBTITLE[] PROGMEM = "Standort über Postleitzahl, Daten von Open-Meteo und OpenStreetMap";
    const char DE_WEATHER_PLZ_LABEL[] PROGMEM = "Postleitzahl";
    const char DE_WEATHER_LOCATION_LABEL[] PROGMEM = "Ort";
    const char DE_WEATHER_COORDS_LABEL[] PROGMEM = "Koordinaten";
    const char DE_WEATHER_NOT_RESOLVED[] PROGMEM = "noch nicht ermittelt";
    const char DE_WEATHER_NO_WIFI_HINT[] PROGMEM = "Ohne WLAN kann der Ort nicht ermittelt werden.";
    const char DE_WEATHER_RESOLVING[] PROGMEM = "Ort wird ermittelt...";
    const char DE_WEATHER_NO_WIFI[] PROGMEM = "Kein WLAN - der Ort wird ermittelt, sobald eine Verbindung besteht.";
    const char DE_WEATHER_NOT_FOUND[] PROGMEM = "Diese Postleitzahl wurde nicht gefunden.";
    const char DE_WEATHER_OUTSIDE_DE[] PROGMEM = "Diese Postleitzahl liegt außerhalb Deutschlands.";
    const char DE_WEATHER_LOOKUP_FAILED[] PROGMEM = "Die Suche ist fehlgeschlagen - der Ort wird beim nächsten Mal neu ermittelt.";
    const char DE_WEATHER_SAVED_FMT[] PROGMEM = "Postleitzahl gespeichert.\nOrt: {0}";
    const char DE_WEATHER_SAVED_MSG[] PROGMEM = "Die Postleitzahl wurde gespeichert. Der Ort wird beim nächsten Wetter-Abruf neu ermittelt.";

    const char DE_CLOUD_TITLE[] PROGMEM = "Cloud-Anbindung";
    const char DE_CLOUD_SUBTITLE[] PROGMEM = "Kalender, Aufgaben, Kontakte, OpenSky";
    const char DE_CLOUD_CALENDAR_LABEL[] PROGMEM = "Kalender";
    const char DE_CLOUD_TODO_LABEL[] PROGMEM = "Aufgaben";
    const char DE_CLOUD_CONTACTS_LABEL[] PROGMEM = "Kontakte";
    const char DE_CLOUD_OPENSKY_ID_LABEL[] PROGMEM = "OpenSky Client-ID";
    const char DE_CLOUD_OPENSKY_SECRET_LABEL[] PROGMEM = "OpenSky Client-Secret";
    const char DE_CLOUD_CALENDAR_PROMPT[] PROGMEM = "Kalender-Adresse:";
    const char DE_CLOUD_TODO_PROMPT[] PROGMEM = "Aufgaben-Adresse:";
    const char DE_CLOUD_CONTACTS_PROMPT[] PROGMEM = "Kontakte-Adresse:";
    const char DE_CLOUD_OPENSKY_ID_PROMPT[] PROGMEM = "OpenSky Client-ID:";
    const char DE_CLOUD_OPENSKY_SECRET_PROMPT[] PROGMEM = "OpenSky Client-Secret:";
    const char DE_CLOUD_NOT_SET[] PROGMEM = "nicht hinterlegt";
    const char DE_CLOUD_CLEAR_BTN[] PROGMEM = "alle löschen";
    const char DE_CLOUD_CLEAR_TITLE[] PROGMEM = "Cloud-Anbindung löschen";
    const char DE_CLOUD_CLEAR_MSG[] PROGMEM = "Alle Werte der Cloud-Anbindung wirklich löschen?\nDie Dienste sind danach nicht mehr erreichbar.";
    const char DE_CLOUD_SAVED_MSG[] PROGMEM = "Die Cloud-Anbindung wurde gespeichert.";
    const char DE_CLOUD_INVALID_TITLE[] PROGMEM = "Ungültige Adressen";
    const char DE_CLOUD_INVALID_PREFIX[] PROGMEM = "muss mit https://script.google.com/ beginnen";
    const char DE_CLOUD_INVALID_SUFFIX[] PROGMEM = "muss auf /exec enden";

    const char DE_TIME_TITLE[] PROGMEM = "Zeit & Zeitserver";
    const char DE_TIME_SUBTITLE[] PROGMEM = "Zeitzone, Zeitserver und manuelle Zeit";
    const char DE_TIME_WINDOW_TITLE[] PROGMEM = "Zeitfenster";
    const char DE_TIME_WINDOW_SUBTITLE[] PROGMEM = "Wochentage und volle Stunden";
    const char DE_TIME_WINDOW_SAVED_MSG[] PROGMEM = "Die Zeitfenster wurden gespeichert.";
    const char DE_TIME_WINDOW_RESET_TITLE[] PROGMEM = "Zeitfenster zurücksetzen";
    const char DE_TIME_WINDOW_RESET_MSG[] PROGMEM = "Alle Zeitfenster auf aktiv setzen?";
    const char DE_TIME_WINDOW_HINT_ACTIVE[] PROGMEM = "Angehakt: Das Gerät bleibt in dieser vollen Stunde aktiv.";
    const char DE_TIME_WINDOW_HINT_SLEEP[] PROGMEM = "Leer: Display und Radio schlafen. Berühren weckt bis zur nächsten Stunde.";
    const char DE_TIME_TZ_LABEL[] PROGMEM = "Zeitzone";
    const char DE_TIME_TZ_PICK_TITLE[] PROGMEM = "Zeitzone wählen";
    const char DE_TIME_TZ_NOT_SET[] PROGMEM = "noch nicht gewählt";
    const char DE_TIME_NTP_LABEL[] PROGMEM = "Zeitserver (NTP)";
    const char DE_TIME_NTP_PROMPT[] PROGMEM = "Zeitserver:";
    const char DE_TIME_NTP_NOT_SET[] PROGMEM = "kein Server hinterlegt";
    const char DE_TIME_SYNC_LABEL[] PROGMEM = "Sync-Intervall";
    const char DE_TIME_SYNC_PICK_TITLE[] PROGMEM = "Sync-Intervall wählen";
    const char DE_TIME_SYNC_24H[] PROGMEM = "24 Stunden";
    const char DE_TIME_SYNC_12H[] PROGMEM = "12 Stunden";
    const char DE_TIME_SYNC_6H[] PROGMEM = "6 Stunden";
    const char DE_TIME_SYNC_3H[] PROGMEM = "3 Stunden";
    const char DE_TIME_SYNC_1H[] PROGMEM = "1 Stunde";
    const char DE_TIME_SYNC_30M[] PROGMEM = "30 Minuten";
    const char DE_TIME_SYNC_10M[] PROGMEM = "10 Minuten";
    const char DE_TIME_SYNC_1M[] PROGMEM = "1 Minute (nicht empfohlen)";
    const char DE_TIME_MANUAL_LABEL[] PROGMEM = "Zeit manuell setzen";
    const char DE_TIME_MANUAL_BTN[] PROGMEM = "zeit setzen";
    const char DE_TIME_MANUAL_CONFIRM_TITLE[] PROGMEM = "Zeit setzen";
    const char DE_TIME_MANUAL_CONFIRM_MSG[] PROGMEM = "Die Uhr wirklich auf den eingestellten Wert setzen?\nDer nächste Zeitserver-Abgleich kann sie wieder ändern.";
    const char DE_TIME_MANUAL_OK[] PROGMEM = "Die Uhr wurde gesetzt.";
    const char DE_TIME_MANUAL_FAIL[] PROGMEM = "Die Uhr konnte nicht gesetzt werden.";
    const char DE_TIME_SAVED_MSG[] PROGMEM = "Die Zeit-Einstellungen wurden gespeichert.";

    const char DE_INFO_SECTION_NETWORK[] PROGMEM = "Netzwerk";
    const char DE_INFO_SECTION_SYSTEM[] PROGMEM = "System";
    const char DE_INFO_SSID[] PROGMEM = "SSID";
    const char DE_INFO_HOSTNAME[] PROGMEM = "Host";
    const char DE_INFO_IP[] PROGMEM = "IP-Adresse";
    const char DE_INFO_GATEWAY[] PROGMEM = "Gateway";
    const char DE_INFO_SIGNAL[] PROGMEM = "Signal";
    const char DE_INFO_MAC[] PROGMEM = "MAC";
    const char DE_INFO_CHANNEL[] PROGMEM = "Kanal";
    const char DE_INFO_NTP_SERVER[] PROGMEM = "Zeitserver";
    const char DE_INFO_SYNC_STATUS[] PROGMEM = "l.Sync";
    const char DE_INFO_MCU[] PROGMEM = "MCU";
    const char DE_INFO_HARDWARE[] PROGMEM = "RAM";
    const char DE_INFO_FIRMWARE_SIZE[] PROGMEM = "FW";
    const char DE_INFO_HEAP_FREE[] PROGMEM = "Heap";
    const char DE_INFO_PSRAM_FREE[] PROGMEM = "PSRAM";
    const char DE_INFO_CHANNEL_FMT[] PROGMEM = "{0:%d} (2,4 GHz)";
    const char DE_INFO_HARDWARE_FMT[] PROGMEM = "Flash: {0:%d} MB, PS: {1:%d} MB";
    const char DE_INFO_NOT_CONNECTED[] PROGMEM = "nicht verbunden";
    const char DE_INFO_NEVER[] PROGMEM = "noch nie";
    const char DE_INFO_SYNC_OK[] PROGMEM = "aktuell";
    const char DE_INFO_SYNC_STALE[] PROGMEM = "veraltet";
    const char DE_INFO_SIGNAL_FMT[] PROGMEM = "{0:%d} dB ({1:%d}%)";
    const char DE_INFO_UART[] PROGMEM = "UART";
    const char DE_INFO_I2C[] PROGMEM = "I2C";
    const char DE_INFO_I2C_NOT_CHECKED[] PROGMEM = "nicht geprüft";
    const char DE_INFO_I2C_NONE[] PROGMEM = "keine Geräte";
    const char DE_INFO_I2C_BUS_BLOCKED[] PROGMEM = "Bus blockiert";
    const char DE_INFO_HEAP_FMT[] PROGMEM = "{0:%d} KB frei ({1:%d}% fragm.)";
    const char DE_INFO_PSRAM_FMT[] PROGMEM = "{0:%d} KB frei ({1:%d}%)";
    const char DE_INFO_UPTIME_FMT[] PROGMEM = "{0:%d}T, {1:%d}Std, {2:%d}Min ({3:%d}x)";

    const char DE_AUDIO_TITLE[] PROGMEM = "Klang";
    const char DE_AUDIO_SUBTITLE[] PROGMEM = "Eckfrequenzen, Basisbreite und Loudness";
    const char DE_AUDIO_BASS[] PROGMEM = "Tiefen";
    const char DE_AUDIO_MID[] PROGMEM = "Mitten";
    const char DE_AUDIO_HIGH[] PROGMEM = "Höhen";
    const char DE_AUDIO_WIDTH[] PROGMEM = "Basisbreite";
    const char DE_AUDIO_LOUDNESS[] PROGMEM = "Loudness";
    const char DE_AUDIO_SPEAKERS[] PROGMEM = "Lautsprecher";
    const char DE_AUDIO_SPEAKERS_ON[] PROGMEM = "An";
    const char DE_AUDIO_SPEAKERS_OFF[] PROGMEM = "Aus";
    const char DE_AUDIO_HZ_FMT[] PROGMEM = "{0:%d} Hz";
    const char DE_AUDIO_PCT_FMT[] PROGMEM = "{0:%d} %";
    const char DE_AUDIO_SAVED_MSG[] PROGMEM = "Die Klang-Einstellungen wurden gespeichert.";

    const char DE_APPEARANCE_TITLE[] PROGMEM = "Anzeige";
    const char DE_APPEARANCE_SUBTITLE[] PROGMEM = "Magisches Auge und analoges VU-Meter";
    const char DE_APPEARANCE_MAGIC_EYE[] PROGMEM = "Magisches Auge";
    const char DE_APPEARANCE_EYE_EM11[] PROGMEM = "EM11";
    const char DE_APPEARANCE_EYE_EM34[] PROGMEM = "EM34";
    const char DE_APPEARANCE_EYE_EM80[] PROGMEM = "EM80";
    const char DE_APPEARANCE_EYE_EM84[] PROGMEM = "EM84";
    const char DE_APPEARANCE_SECTION_VU[] PROGMEM = "Analoges VU-Meter";
    const char DE_APPEARANCE_VU_BALLISTICS[] PROGMEM = "Zeigerträgheit";
    const char DE_APPEARANCE_BALLISTICS_HINT[] PROGMEM = "Einschwingzeit bis 99 Prozent. Die Norm verlangt 300 ms.";
    const char DE_APPEARANCE_VU_LEVEL[] PROGMEM = "Bezugspegel";
    const char DE_APPEARANCE_LEVEL_HINT[] PROGMEM = "Pegel unter Vollaussteuerung, bei dem der Zeiger 0 VU erreicht.";
    const char DE_APPEARANCE_LEVEL_NOW[] PROGMEM = "Sender liefert";
    const char DE_APPEARANCE_HINT[] PROGMEM = "Ein langer Druck auf das Auge in der Skala schaltet ebenfalls um.";
    const char DE_APPEARANCE_SAVED_MSG[] PROGMEM = "Anzeige gespeichert.";

    // ===================================================================
    // PROGMEM-Literale (Englisch, britisch)
    // ===================================================================

    const char EN_EMPTY[] PROGMEM = "";

    const char EN_SEC_CONNECTION[] PROGMEM = "connectivity";
    const char EN_SEC_SERVICES[] PROGMEM = "services";
    const char EN_SEC_SYSTEM[] PROGMEM = "system";

    const char EN_MENU_WIFI[] PROGMEM = "Wi-Fi";
    const char EN_MENU_TIME[] PROGMEM = "Time & time server";
    const char EN_MENU_TIME_WINDOW[] PROGMEM = "Time windows";
    const char EN_MENU_WEATHER[] PROGMEM = "Weather";
    const char EN_MENU_NEWS[] PROGMEM = "News";
    const char EN_MENU_CLOUD[] PROGMEM = "Cloud sync";
    const char EN_MENU_LANGUAGE[] PROGMEM = "Language";
    const char EN_MENU_SECURITY[] PROGMEM = "Security";
    const char EN_MENU_AUDIO[] PROGMEM = "Sound";
    const char EN_MENU_APPEARANCE[] PROGMEM = "Display";
    const char EN_MENU_INFO[] PROGMEM = "Info & restart";

    const char EN_FOOTER_SAVE[] PROGMEM = "save";

    const char EN_PLACEHOLDER_TITLE[] PROGMEM = "Coming soon";
    const char EN_PLACEHOLDER_MESSAGE[] PROGMEM = "This area is being built.";

    const char EN_WIFI_TITLE[] PROGMEM = "Wi-Fi";
    const char EN_WIFI_SUBTITLE[] PROGMEM = "Profiles individually switchable";

    const char EN_WIFI_PROFILE_FMT[] PROGMEM = "Profile {0:%d}";
    const char EN_WIFI_EMPTY_NAME[] PROGMEM = "(empty)";
    const char EN_WIFI_STATUS_CONNECTED[] PROGMEM = "connected";
    const char EN_WIFI_STATUS_ACTIVE[] PROGMEM = "active (ready)";
    const char EN_WIFI_STATUS_INACTIVE[] PROGMEM = "inactive";
    const char EN_WIFI_STATUS_EMPTY[] PROGMEM = "no network set";

    const char EN_WIFI_ACT_SCAN[] PROGMEM = "scan";
    const char EN_WIFI_ACT_PASSWORD[] PROGMEM = "password";
    const char EN_WIFI_ACT_TEST[] PROGMEM = "test";
    const char EN_WIFI_ACT_DELETE[] PROGMEM = "delete";

    const char EN_WIFI_SCAN_TITLE[] PROGMEM = "Choose network:";
    const char EN_WIFI_PW_TITLE_FMT[] PROGMEM = "Password for {0}:";

    const char EN_WIFI_NEED_SSID_TITLE[] PROGMEM = "No network chosen";
    const char EN_WIFI_NEED_SSID_MSG[] PROGMEM = "Please choose a network first.";

    const char EN_WIFI_PW_BAD_TITLE[] PROGMEM = "Invalid password";
    const char EN_WIFI_PW_ERR_LEN[] PROGMEM = "Password must be 8 to 63 characters long.";
    const char EN_WIFI_PW_ERR_CHARS[] PROGMEM = "Please use standard keyboard characters only.";
    const char EN_WIFI_PW_ERR_HEX[] PROGMEM = "64 characters allow only digits 0-9 and A-F.";
    const char EN_WIFI_PW_ERR_INTERNAL[] PROGMEM = "Internal error.";

    const char EN_WIFI_DELETE_TITLE[] PROGMEM = "Delete profile";
    const char EN_WIFI_DELETE_MSG[] PROGMEM = "Really delete this profile permanently?";

    const char EN_WIFI_TEST_BOX_TITLE[] PROGMEM = "Connection test";
    const char EN_WIFI_TEST_PREPARE[] PROGMEM = "Preparing connection...";
    const char EN_WIFI_TEST_RUNNING[] PROGMEM = "Testing connection...";
    const char EN_WIFI_TEST_OK_TITLE[] PROGMEM = "Test successful";
    const char EN_WIFI_TEST_OK_FMT[] PROGMEM = "Connected.\n\nIP: {0}\nSignal: {1:%d} dBm";
    const char EN_WIFI_TEST_FAIL_TITLE[] PROGMEM = "Test failed";
    const char EN_WIFI_TEST_FAIL_MSG[] PROGMEM = "Connection failed.\nCheck SSID and password.";
    const char EN_WIFI_TEST_TIMEOUT_MSG[] PROGMEM = "Timed out.\nCheck SSID and password.";

    const char EN_WIFI_HOSTNAME_LABEL[] PROGMEM = "Network name";
    const char EN_WIFI_HOSTNAME_PROMPT[] PROGMEM = "Network name:";
    const char EN_WIFI_HOSTNAME_EDIT[] PROGMEM = "edit";
    const char EN_WIFI_HOSTNAME_BAD_TITLE[] PROGMEM = "Invalid name";
    const char EN_WIFI_HOSTNAME_ERR[] PROGMEM = "Allowed are letters, digits and hyphen, 1 to 32 characters. The name must not start or end with a hyphen.";

    const char EN_SAVED_TITLE[] PROGMEM = "Saved";
    const char EN_SAVED_MSG[] PROGMEM = "Settings saved.\nA restart applies the changes.";
    const char EN_SAVED_RESTART_BTN[] PROGMEM = "restart";
    const char EN_SAVED_LATER_BTN[] PROGMEM = "later";

    const char EN_EXIT_TITLE[] PROGMEM = "Unsaved changes";
    const char EN_EXIT_MSG[] PROGMEM = "Save changes?";
    const char EN_EXIT_DISCARD_BTN[] PROGMEM = "discard";

    const char EN_LANG_TITLE[] PROGMEM = "Language";
    const char EN_LANG_SUBTITLE[] PROGMEM = "Interface language";
    const char EN_LANG_ACTIVE[] PROGMEM = "active";
    const char EN_LANG_NAME_DE[] PROGMEM = "Deutsch";
    const char EN_LANG_NAME_EN[] PROGMEM = "English";

    const char EN_SEC_TITLE[] PROGMEM = "Security";
    const char EN_SEC_SUBTITLE[] PROGMEM = "Change lock PIN";
    const char EN_SEC_PIN_HINT[] PROGMEM = "Type 4 digits for a new PIN.";
    const char EN_SEC_PIN_SET[] PROGMEM = "A PIN is set.";
    const char EN_SEC_SAVED_TITLE[] PROGMEM = "PIN changed";
    const char EN_SEC_SAVED_MSG[] PROGMEM = "The new PIN has been saved.";
    const char EN_SEC_PIN_REPEAT[] PROGMEM = "Repeat PIN";
    const char EN_SEC_PIN_MISMATCH[] PROGMEM = "Entries do not match";
    const char EN_SEC_PIN_READY[] PROGMEM = "PIN ready. Confirm with Save.";

    const char EN_INFO_TITLE[] PROGMEM = "Info & restart";
    const char EN_INFO_SUBTITLE[] PROGMEM = "Device information";
    const char EN_INFO_FIRMWARE[] PROGMEM = "Version";
    const char EN_INFO_UPTIME[] PROGMEM = "Uptime";
    const char EN_INFO_RESTART_BTN[] PROGMEM = "restart";
    const char EN_INFO_RESTART_TITLE[] PROGMEM = "Restart";
    const char EN_INFO_RESTART_MSG[] PROGMEM = "Really restart?\nUnsaved changes will be lost.";
    const char EN_INFO_RESET_BTN[] PROGMEM = "reset";
    const char EN_INFO_RESET_TITLE[] PROGMEM = "Reset";
    const char EN_INFO_RESET_MSG[] PROGMEM = "Reset to factory settings?\nAll profiles and settings will be lost.";
    const char EN_LANG_RESTART_TITLE[] PROGMEM = "Change language";
    const char EN_LANG_RESTART_MSG[] PROGMEM = "The new language takes effect after a restart.\nRestart now?";

    const char EN_NEWS_TITLE[] PROGMEM = "News";
    const char EN_NEWS_SUBTITLE[] PROGMEM = "Topics and state";
    const char EN_NEWS_RESSORT_LABEL[] PROGMEM = "Topics";
    const char EN_NEWS_REGION_LABEL[] PROGMEM = "State";

    const char EN_NEWS_RESSORT_ALL[] PROGMEM = "all";
    const char EN_NEWS_RESSORT_INLAND[] PROGMEM = "Domestic";
    const char EN_NEWS_RESSORT_AUSLAND[] PROGMEM = "International";
    const char EN_NEWS_RESSORT_WIRTSCHAFT[] PROGMEM = "Economy";
    const char EN_NEWS_RESSORT_SPORT[] PROGMEM = "Sport";
    const char EN_NEWS_RESSORT_VIDEO[] PROGMEM = "Video";
    const char EN_NEWS_RESSORT_INVESTIGATIV[] PROGMEM = "Investigative";
    const char EN_NEWS_RESSORT_WISSEN[] PROGMEM = "Science";

    const char EN_NEWS_REGION_PICK_TITLE[] PROGMEM = "Choose state:";

    const char EN_NEWS_BL_BW[] PROGMEM = "Baden-Württemberg";
    const char EN_NEWS_BL_BY[] PROGMEM = "Bavaria";
    const char EN_NEWS_BL_BE[] PROGMEM = "Berlin";
    const char EN_NEWS_BL_BB[] PROGMEM = "Brandenburg";
    const char EN_NEWS_BL_HB[] PROGMEM = "Bremen";
    const char EN_NEWS_BL_HH[] PROGMEM = "Hamburg";
    const char EN_NEWS_BL_HE[] PROGMEM = "Hesse";
    const char EN_NEWS_BL_MV[] PROGMEM = "Mecklenburg-Western Pomerania";
    const char EN_NEWS_BL_NI[] PROGMEM = "Lower Saxony";
    const char EN_NEWS_BL_NW[] PROGMEM = "North Rhine-Westphalia";
    const char EN_NEWS_BL_RP[] PROGMEM = "Rhineland-Palatinate";
    const char EN_NEWS_BL_SL[] PROGMEM = "Saarland";
    const char EN_NEWS_BL_SN[] PROGMEM = "Saxony";
    const char EN_NEWS_BL_ST[] PROGMEM = "Saxony-Anhalt";
    const char EN_NEWS_BL_SH[] PROGMEM = "Schleswig-Holstein";
    const char EN_NEWS_BL_TH[] PROGMEM = "Thuringia";

    const char EN_NEWS_SAVED_MSG[] PROGMEM = "The news filters have been saved.";

    const char EN_WEATHER_TITLE[] PROGMEM = "Weather";
    const char EN_WEATHER_SUBTITLE[] PROGMEM = "Location via postal code, data by Open-Meteo and OpenStreetMap";
    const char EN_WEATHER_PLZ_LABEL[] PROGMEM = "Postal code";
    const char EN_WEATHER_LOCATION_LABEL[] PROGMEM = "Place";
    const char EN_WEATHER_COORDS_LABEL[] PROGMEM = "Coordinates";
    const char EN_WEATHER_NOT_RESOLVED[] PROGMEM = "not yet determined";
    const char EN_WEATHER_NO_WIFI_HINT[] PROGMEM = "Without Wi-Fi the place cannot be determined.";
    const char EN_WEATHER_RESOLVING[] PROGMEM = "Determining place...";
    const char EN_WEATHER_NO_WIFI[] PROGMEM = "No Wi-Fi - the place will be determined as soon as there is a connection.";
    const char EN_WEATHER_NOT_FOUND[] PROGMEM = "This postal code was not found.";
    const char EN_WEATHER_OUTSIDE_DE[] PROGMEM = "This postal code is outside Germany.";
    const char EN_WEATHER_LOOKUP_FAILED[] PROGMEM = "The lookup failed - the place will be determined the next time.";
    const char EN_WEATHER_SAVED_FMT[] PROGMEM = "Postal code saved.\nPlace: {0}";
    const char EN_WEATHER_SAVED_MSG[] PROGMEM = "The postal code has been saved. The place will be determined at the next weather update.";

    const char EN_CLOUD_TITLE[] PROGMEM = "Cloud sync";
    const char EN_CLOUD_SUBTITLE[] PROGMEM = "Calendar, tasks, contacts, OpenSky";
    const char EN_CLOUD_CALENDAR_LABEL[] PROGMEM = "Calendar";
    const char EN_CLOUD_TODO_LABEL[] PROGMEM = "Tasks";
    const char EN_CLOUD_CONTACTS_LABEL[] PROGMEM = "Contacts";
    const char EN_CLOUD_OPENSKY_ID_LABEL[] PROGMEM = "OpenSky client ID";
    const char EN_CLOUD_OPENSKY_SECRET_LABEL[] PROGMEM = "OpenSky client secret";
    const char EN_CLOUD_CALENDAR_PROMPT[] PROGMEM = "Calendar address:";
    const char EN_CLOUD_TODO_PROMPT[] PROGMEM = "Tasks address:";
    const char EN_CLOUD_CONTACTS_PROMPT[] PROGMEM = "Contacts address:";
    const char EN_CLOUD_OPENSKY_ID_PROMPT[] PROGMEM = "OpenSky client ID:";
    const char EN_CLOUD_OPENSKY_SECRET_PROMPT[] PROGMEM = "OpenSky client secret:";
    const char EN_CLOUD_NOT_SET[] PROGMEM = "not set";
    const char EN_CLOUD_CLEAR_BTN[] PROGMEM = "clear all";
    const char EN_CLOUD_CLEAR_TITLE[] PROGMEM = "Clear cloud sync";
    const char EN_CLOUD_CLEAR_MSG[] PROGMEM = "Really clear all cloud sync values?\nThe services will no longer be reachable.";
    const char EN_CLOUD_SAVED_MSG[] PROGMEM = "Cloud sync has been saved.";
    const char EN_CLOUD_INVALID_TITLE[] PROGMEM = "Invalid addresses";
    const char EN_CLOUD_INVALID_PREFIX[] PROGMEM = "must start with https://script.google.com/";
    const char EN_CLOUD_INVALID_SUFFIX[] PROGMEM = "must end with /exec";

    const char EN_TIME_TITLE[] PROGMEM = "Time & time server";
    const char EN_TIME_SUBTITLE[] PROGMEM = "Time zone, time server and manual time";
    const char EN_TIME_WINDOW_TITLE[] PROGMEM = "Time windows";
    const char EN_TIME_WINDOW_SUBTITLE[] PROGMEM = "Weekdays and full hours";
    const char EN_TIME_WINDOW_SAVED_MSG[] PROGMEM = "The time windows have been saved.";
    const char EN_TIME_WINDOW_RESET_TITLE[] PROGMEM = "Reset time windows";
    const char EN_TIME_WINDOW_RESET_MSG[] PROGMEM = "Set all time windows to active?";
    const char EN_TIME_WINDOW_HINT_ACTIVE[] PROGMEM = "Checked: device stays active during this full hour.";
    const char EN_TIME_WINDOW_HINT_SLEEP[] PROGMEM = "Empty: display and radio sleep. Touch wakes until the next hour.";
    const char EN_TIME_TZ_LABEL[] PROGMEM = "Time zone";
    const char EN_TIME_TZ_PICK_TITLE[] PROGMEM = "Choose time zone";
    const char EN_TIME_TZ_NOT_SET[] PROGMEM = "not yet chosen";
    const char EN_TIME_NTP_LABEL[] PROGMEM = "Time server (NTP)";
    const char EN_TIME_NTP_PROMPT[] PROGMEM = "Time server:";
    const char EN_TIME_NTP_NOT_SET[] PROGMEM = "no server set";
    const char EN_TIME_SYNC_LABEL[] PROGMEM = "Sync interval";
    const char EN_TIME_SYNC_PICK_TITLE[] PROGMEM = "Choose sync interval";
    const char EN_TIME_SYNC_24H[] PROGMEM = "24 hours";
    const char EN_TIME_SYNC_12H[] PROGMEM = "12 hours";
    const char EN_TIME_SYNC_6H[] PROGMEM = "6 hours";
    const char EN_TIME_SYNC_3H[] PROGMEM = "3 hours";
    const char EN_TIME_SYNC_1H[] PROGMEM = "1 hour";
    const char EN_TIME_SYNC_30M[] PROGMEM = "30 minutes";
    const char EN_TIME_SYNC_10M[] PROGMEM = "10 minutes";
    const char EN_TIME_SYNC_1M[] PROGMEM = "1 minute (not recommended)";
    const char EN_TIME_MANUAL_LABEL[] PROGMEM = "Set time manually";
    const char EN_TIME_MANUAL_BTN[] PROGMEM = "set time";
    const char EN_TIME_MANUAL_CONFIRM_TITLE[] PROGMEM = "Set time";
    const char EN_TIME_MANUAL_CONFIRM_MSG[] PROGMEM = "Really set the clock to the chosen value?\nThe next time server sync may change it again.";
    const char EN_TIME_MANUAL_OK[] PROGMEM = "The clock has been set.";
    const char EN_TIME_MANUAL_FAIL[] PROGMEM = "The clock could not be set.";
    const char EN_TIME_SAVED_MSG[] PROGMEM = "The time settings have been saved.";

    const char EN_INFO_SECTION_NETWORK[] PROGMEM = "Network";
    const char EN_INFO_SECTION_SYSTEM[] PROGMEM = "System";
    const char EN_INFO_SSID[] PROGMEM = "SSID";
    const char EN_INFO_HOSTNAME[] PROGMEM = "Host";
    const char EN_INFO_IP[] PROGMEM = "IP address";
    const char EN_INFO_GATEWAY[] PROGMEM = "Gateway";
    const char EN_INFO_SIGNAL[] PROGMEM = "Signal";
    const char EN_INFO_MAC[] PROGMEM = "MAC";
    const char EN_INFO_CHANNEL[] PROGMEM = "Channel";
    const char EN_INFO_NTP_SERVER[] PROGMEM = "Time server";
    const char EN_INFO_SYNC_STATUS[] PROGMEM = "l.Sync";
    const char EN_INFO_MCU[] PROGMEM = "MCU";
    const char EN_INFO_HARDWARE[] PROGMEM = "RAM";
    const char EN_INFO_FIRMWARE_SIZE[] PROGMEM = "FW";
    const char EN_INFO_HEAP_FREE[] PROGMEM = "Heap";
    const char EN_INFO_PSRAM_FREE[] PROGMEM = "PSRAM";
    const char EN_INFO_CHANNEL_FMT[] PROGMEM = "{0:%d} (2.4 GHz)";
    const char EN_INFO_HARDWARE_FMT[] PROGMEM = "Flash: {0:%d} MB, PS: {1:%d} MB";
    const char EN_INFO_NOT_CONNECTED[] PROGMEM = "not connected";
    const char EN_INFO_NEVER[] PROGMEM = "never";
    const char EN_INFO_SYNC_OK[] PROGMEM = "current";
    const char EN_INFO_SYNC_STALE[] PROGMEM = "stale";
    const char EN_INFO_SIGNAL_FMT[] PROGMEM = "{0:%d} dB ({1:%d}%)";
    const char EN_INFO_UART[] PROGMEM = "UART";
    const char EN_INFO_I2C[] PROGMEM = "I2C";
    const char EN_INFO_I2C_NOT_CHECKED[] PROGMEM = "not checked";
    const char EN_INFO_I2C_NONE[] PROGMEM = "no devices";
    const char EN_INFO_I2C_BUS_BLOCKED[] PROGMEM = "bus blocked";
    const char EN_INFO_HEAP_FMT[] PROGMEM = "{0:%d} KB free ({1:%d}% frag.)";
    const char EN_INFO_PSRAM_FMT[] PROGMEM = "{0:%d} KB free ({1:%d}%)";
    const char EN_INFO_UPTIME_FMT[] PROGMEM = "{0:%d}d, {1:%d}h, {2:%d}min ({3:%d}x)";

    const char EN_AUDIO_TITLE[] PROGMEM = "Sound";
    const char EN_AUDIO_SUBTITLE[] PROGMEM = "Tone corner frequencies, stereo width and loudness";
    const char EN_AUDIO_BASS[] PROGMEM = "Bass";
    const char EN_AUDIO_MID[] PROGMEM = "Mids";
    const char EN_AUDIO_HIGH[] PROGMEM = "Treble";
    const char EN_AUDIO_WIDTH[] PROGMEM = "Stereo width";
    const char EN_AUDIO_LOUDNESS[] PROGMEM = "Loudness";
    const char EN_AUDIO_SPEAKERS[] PROGMEM = "Speakers";
    const char EN_AUDIO_SPEAKERS_ON[] PROGMEM = "On";
    const char EN_AUDIO_SPEAKERS_OFF[] PROGMEM = "Off";
    const char EN_AUDIO_HZ_FMT[] PROGMEM = "{0:%d} Hz";
    const char EN_AUDIO_PCT_FMT[] PROGMEM = "{0:%d} %";
    const char EN_AUDIO_SAVED_MSG[] PROGMEM = "The sound settings have been saved.";

    const char EN_APPEARANCE_TITLE[] PROGMEM = "Display";
    const char EN_APPEARANCE_SUBTITLE[] PROGMEM = "Magic eye and analogue VU meter";
    const char EN_APPEARANCE_MAGIC_EYE[] PROGMEM = "Magic eye";
    const char EN_APPEARANCE_EYE_EM11[] PROGMEM = "EM11";
    const char EN_APPEARANCE_EYE_EM34[] PROGMEM = "EM34";
    const char EN_APPEARANCE_EYE_EM80[] PROGMEM = "EM80";
    const char EN_APPEARANCE_EYE_EM84[] PROGMEM = "EM84";
    const char EN_APPEARANCE_SECTION_VU[] PROGMEM = "Analogue VU meter";
    const char EN_APPEARANCE_VU_BALLISTICS[] PROGMEM = "Needle ballistics";
    const char EN_APPEARANCE_BALLISTICS_HINT[] PROGMEM = "Settling time to 99 per cent. The standard calls for 300 ms.";
    const char EN_APPEARANCE_VU_LEVEL[] PROGMEM = "Reference level";
    const char EN_APPEARANCE_LEVEL_HINT[] PROGMEM = "Level below full scale at which the needle reads 0 VU.";
    const char EN_APPEARANCE_LEVEL_NOW[] PROGMEM = "source delivers";
    const char EN_APPEARANCE_HINT[] PROGMEM = "A long press on the eye in the dial switches as well.";
    const char EN_APPEARANCE_SAVED_MSG[] PROGMEM = "Display settings saved.";
} // namespace

// ===================================================================
// Pointer-Tabelle (DE, EN)
// ===================================================================

// Reihenfolge muss exakt zu SettingsKey passen.

namespace
{
    const char *const SETTINGS_TABLE[][2] PROGMEM = {
        {DE_EMPTY, EN_EMPTY},

        {DE_SEC_CONNECTION, EN_SEC_CONNECTION},
        {DE_SEC_SERVICES, EN_SEC_SERVICES},
        {DE_SEC_SYSTEM, EN_SEC_SYSTEM},

        {DE_MENU_WIFI, EN_MENU_WIFI},
        {DE_MENU_TIME, EN_MENU_TIME},
        {DE_MENU_TIME_WINDOW, EN_MENU_TIME_WINDOW},
        {DE_MENU_WEATHER, EN_MENU_WEATHER},
        {DE_MENU_NEWS, EN_MENU_NEWS},
        {DE_MENU_CLOUD, EN_MENU_CLOUD},
        {DE_MENU_LANGUAGE, EN_MENU_LANGUAGE},
        {DE_MENU_SECURITY, EN_MENU_SECURITY},
        {DE_MENU_AUDIO, EN_MENU_AUDIO},
        {DE_MENU_APPEARANCE, EN_MENU_APPEARANCE},
        {DE_MENU_INFO, EN_MENU_INFO},

        {DE_FOOTER_SAVE, EN_FOOTER_SAVE},

        {DE_PLACEHOLDER_TITLE, EN_PLACEHOLDER_TITLE},
        {DE_PLACEHOLDER_MESSAGE, EN_PLACEHOLDER_MESSAGE},

        {DE_WIFI_TITLE, EN_WIFI_TITLE},
        {DE_WIFI_SUBTITLE, EN_WIFI_SUBTITLE},

        {DE_WIFI_PROFILE_FMT, EN_WIFI_PROFILE_FMT},
        {DE_WIFI_EMPTY_NAME, EN_WIFI_EMPTY_NAME},
        {DE_WIFI_STATUS_CONNECTED, EN_WIFI_STATUS_CONNECTED},
        {DE_WIFI_STATUS_ACTIVE, EN_WIFI_STATUS_ACTIVE},
        {DE_WIFI_STATUS_INACTIVE, EN_WIFI_STATUS_INACTIVE},
        {DE_WIFI_STATUS_EMPTY, EN_WIFI_STATUS_EMPTY},

        {DE_WIFI_ACT_SCAN, EN_WIFI_ACT_SCAN},
        {DE_WIFI_ACT_PASSWORD, EN_WIFI_ACT_PASSWORD},
        {DE_WIFI_ACT_TEST, EN_WIFI_ACT_TEST},
        {DE_WIFI_ACT_DELETE, EN_WIFI_ACT_DELETE},

        {DE_WIFI_SCAN_TITLE, EN_WIFI_SCAN_TITLE},
        {DE_WIFI_PW_TITLE_FMT, EN_WIFI_PW_TITLE_FMT},

        {DE_WIFI_NEED_SSID_TITLE, EN_WIFI_NEED_SSID_TITLE},
        {DE_WIFI_NEED_SSID_MSG, EN_WIFI_NEED_SSID_MSG},

        {DE_WIFI_PW_BAD_TITLE, EN_WIFI_PW_BAD_TITLE},
        {DE_WIFI_PW_ERR_LEN, EN_WIFI_PW_ERR_LEN},
        {DE_WIFI_PW_ERR_CHARS, EN_WIFI_PW_ERR_CHARS},
        {DE_WIFI_PW_ERR_HEX, EN_WIFI_PW_ERR_HEX},
        {DE_WIFI_PW_ERR_INTERNAL, EN_WIFI_PW_ERR_INTERNAL},

        {DE_WIFI_DELETE_TITLE, EN_WIFI_DELETE_TITLE},
        {DE_WIFI_DELETE_MSG, EN_WIFI_DELETE_MSG},

        {DE_WIFI_TEST_BOX_TITLE, EN_WIFI_TEST_BOX_TITLE},
        {DE_WIFI_TEST_PREPARE, EN_WIFI_TEST_PREPARE},
        {DE_WIFI_TEST_RUNNING, EN_WIFI_TEST_RUNNING},
        {DE_WIFI_TEST_OK_TITLE, EN_WIFI_TEST_OK_TITLE},
        {DE_WIFI_TEST_OK_FMT, EN_WIFI_TEST_OK_FMT},
        {DE_WIFI_TEST_FAIL_TITLE, EN_WIFI_TEST_FAIL_TITLE},
        {DE_WIFI_TEST_FAIL_MSG, EN_WIFI_TEST_FAIL_MSG},
        {DE_WIFI_TEST_TIMEOUT_MSG, EN_WIFI_TEST_TIMEOUT_MSG},

        {DE_WIFI_HOSTNAME_LABEL, EN_WIFI_HOSTNAME_LABEL},
        {DE_WIFI_HOSTNAME_PROMPT, EN_WIFI_HOSTNAME_PROMPT},
        {DE_WIFI_HOSTNAME_EDIT, EN_WIFI_HOSTNAME_EDIT},
        {DE_WIFI_HOSTNAME_BAD_TITLE, EN_WIFI_HOSTNAME_BAD_TITLE},
        {DE_WIFI_HOSTNAME_ERR, EN_WIFI_HOSTNAME_ERR},

        {DE_SAVED_TITLE, EN_SAVED_TITLE},
        {DE_SAVED_MSG, EN_SAVED_MSG},
        {DE_SAVED_RESTART_BTN, EN_SAVED_RESTART_BTN},
        {DE_SAVED_LATER_BTN, EN_SAVED_LATER_BTN},

        {DE_EXIT_TITLE, EN_EXIT_TITLE},
        {DE_EXIT_MSG, EN_EXIT_MSG},
        {DE_EXIT_DISCARD_BTN, EN_EXIT_DISCARD_BTN},

        {DE_LANG_TITLE, EN_LANG_TITLE},
        {DE_LANG_SUBTITLE, EN_LANG_SUBTITLE},
        {DE_LANG_ACTIVE, EN_LANG_ACTIVE},
        {DE_LANG_NAME_DE, EN_LANG_NAME_DE},
        {DE_LANG_NAME_EN, EN_LANG_NAME_EN},

        {DE_SEC_TITLE, EN_SEC_TITLE},
        {DE_SEC_SUBTITLE, EN_SEC_SUBTITLE},
        {DE_SEC_PIN_HINT, EN_SEC_PIN_HINT},
        {DE_SEC_PIN_SET, EN_SEC_PIN_SET},
        {DE_SEC_SAVED_TITLE, EN_SEC_SAVED_TITLE},
        {DE_SEC_SAVED_MSG, EN_SEC_SAVED_MSG},
        {DE_SEC_PIN_REPEAT, EN_SEC_PIN_REPEAT},
        {DE_SEC_PIN_MISMATCH, EN_SEC_PIN_MISMATCH},
        {DE_SEC_PIN_READY, EN_SEC_PIN_READY},

        {DE_INFO_TITLE, EN_INFO_TITLE},
        {DE_INFO_SUBTITLE, EN_INFO_SUBTITLE},
        {DE_INFO_FIRMWARE, EN_INFO_FIRMWARE},
        {DE_INFO_UPTIME, EN_INFO_UPTIME},
        {DE_INFO_RESTART_BTN, EN_INFO_RESTART_BTN},
        {DE_INFO_RESTART_TITLE, EN_INFO_RESTART_TITLE},
        {DE_INFO_RESTART_MSG, EN_INFO_RESTART_MSG},
        {DE_INFO_RESET_BTN, EN_INFO_RESET_BTN},
        {DE_INFO_RESET_TITLE, EN_INFO_RESET_TITLE},
        {DE_INFO_RESET_MSG, EN_INFO_RESET_MSG},
        {DE_LANG_RESTART_TITLE, EN_LANG_RESTART_TITLE},
        {DE_LANG_RESTART_MSG, EN_LANG_RESTART_MSG},

        {DE_NEWS_TITLE, EN_NEWS_TITLE},
        {DE_NEWS_SUBTITLE, EN_NEWS_SUBTITLE},
        {DE_NEWS_RESSORT_LABEL, EN_NEWS_RESSORT_LABEL},
        {DE_NEWS_REGION_LABEL, EN_NEWS_REGION_LABEL},

        {DE_NEWS_RESSORT_ALL, EN_NEWS_RESSORT_ALL},
        {DE_NEWS_RESSORT_INLAND, EN_NEWS_RESSORT_INLAND},
        {DE_NEWS_RESSORT_AUSLAND, EN_NEWS_RESSORT_AUSLAND},
        {DE_NEWS_RESSORT_WIRTSCHAFT, EN_NEWS_RESSORT_WIRTSCHAFT},
        {DE_NEWS_RESSORT_SPORT, EN_NEWS_RESSORT_SPORT},
        {DE_NEWS_RESSORT_VIDEO, EN_NEWS_RESSORT_VIDEO},
        {DE_NEWS_RESSORT_INVESTIGATIV, EN_NEWS_RESSORT_INVESTIGATIV},
        {DE_NEWS_RESSORT_WISSEN, EN_NEWS_RESSORT_WISSEN},

        {DE_NEWS_REGION_PICK_TITLE, EN_NEWS_REGION_PICK_TITLE},

        {DE_NEWS_BL_BW, EN_NEWS_BL_BW},
        {DE_NEWS_BL_BY, EN_NEWS_BL_BY},
        {DE_NEWS_BL_BE, EN_NEWS_BL_BE},
        {DE_NEWS_BL_BB, EN_NEWS_BL_BB},
        {DE_NEWS_BL_HB, EN_NEWS_BL_HB},
        {DE_NEWS_BL_HH, EN_NEWS_BL_HH},
        {DE_NEWS_BL_HE, EN_NEWS_BL_HE},
        {DE_NEWS_BL_MV, EN_NEWS_BL_MV},
        {DE_NEWS_BL_NI, EN_NEWS_BL_NI},
        {DE_NEWS_BL_NW, EN_NEWS_BL_NW},
        {DE_NEWS_BL_RP, EN_NEWS_BL_RP},
        {DE_NEWS_BL_SL, EN_NEWS_BL_SL},
        {DE_NEWS_BL_SN, EN_NEWS_BL_SN},
        {DE_NEWS_BL_ST, EN_NEWS_BL_ST},
        {DE_NEWS_BL_SH, EN_NEWS_BL_SH},
        {DE_NEWS_BL_TH, EN_NEWS_BL_TH},

        {DE_NEWS_SAVED_MSG, EN_NEWS_SAVED_MSG},

        {DE_WEATHER_TITLE, EN_WEATHER_TITLE},
        {DE_WEATHER_SUBTITLE, EN_WEATHER_SUBTITLE},
        {DE_WEATHER_PLZ_LABEL, EN_WEATHER_PLZ_LABEL},
        {DE_WEATHER_LOCATION_LABEL, EN_WEATHER_LOCATION_LABEL},
        {DE_WEATHER_COORDS_LABEL, EN_WEATHER_COORDS_LABEL},
        {DE_WEATHER_NOT_RESOLVED, EN_WEATHER_NOT_RESOLVED},
        {DE_WEATHER_NO_WIFI_HINT, EN_WEATHER_NO_WIFI_HINT},
        {DE_WEATHER_RESOLVING, EN_WEATHER_RESOLVING},
        {DE_WEATHER_NO_WIFI, EN_WEATHER_NO_WIFI},
        {DE_WEATHER_NOT_FOUND, EN_WEATHER_NOT_FOUND},
        {DE_WEATHER_OUTSIDE_DE, EN_WEATHER_OUTSIDE_DE},
        {DE_WEATHER_LOOKUP_FAILED, EN_WEATHER_LOOKUP_FAILED},
        {DE_WEATHER_SAVED_FMT, EN_WEATHER_SAVED_FMT},
        {DE_WEATHER_SAVED_MSG, EN_WEATHER_SAVED_MSG},

        {DE_CLOUD_TITLE, EN_CLOUD_TITLE},
        {DE_CLOUD_SUBTITLE, EN_CLOUD_SUBTITLE},
        {DE_CLOUD_CALENDAR_LABEL, EN_CLOUD_CALENDAR_LABEL},
        {DE_CLOUD_TODO_LABEL, EN_CLOUD_TODO_LABEL},
        {DE_CLOUD_CONTACTS_LABEL, EN_CLOUD_CONTACTS_LABEL},
        {DE_CLOUD_OPENSKY_ID_LABEL, EN_CLOUD_OPENSKY_ID_LABEL},
        {DE_CLOUD_OPENSKY_SECRET_LABEL, EN_CLOUD_OPENSKY_SECRET_LABEL},
        {DE_CLOUD_CALENDAR_PROMPT, EN_CLOUD_CALENDAR_PROMPT},
        {DE_CLOUD_TODO_PROMPT, EN_CLOUD_TODO_PROMPT},
        {DE_CLOUD_CONTACTS_PROMPT, EN_CLOUD_CONTACTS_PROMPT},
        {DE_CLOUD_OPENSKY_ID_PROMPT, EN_CLOUD_OPENSKY_ID_PROMPT},
        {DE_CLOUD_OPENSKY_SECRET_PROMPT, EN_CLOUD_OPENSKY_SECRET_PROMPT},
        {DE_CLOUD_NOT_SET, EN_CLOUD_NOT_SET},
        {DE_CLOUD_CLEAR_BTN, EN_CLOUD_CLEAR_BTN},
        {DE_CLOUD_CLEAR_TITLE, EN_CLOUD_CLEAR_TITLE},
        {DE_CLOUD_CLEAR_MSG, EN_CLOUD_CLEAR_MSG},
        {DE_CLOUD_SAVED_MSG, EN_CLOUD_SAVED_MSG},
        {DE_CLOUD_INVALID_TITLE, EN_CLOUD_INVALID_TITLE},
        {DE_CLOUD_INVALID_PREFIX, EN_CLOUD_INVALID_PREFIX},
        {DE_CLOUD_INVALID_SUFFIX, EN_CLOUD_INVALID_SUFFIX},

        {DE_TIME_TITLE, EN_TIME_TITLE},
        {DE_TIME_SUBTITLE, EN_TIME_SUBTITLE},
        {DE_TIME_WINDOW_TITLE, EN_TIME_WINDOW_TITLE},
        {DE_TIME_WINDOW_SUBTITLE, EN_TIME_WINDOW_SUBTITLE},
        {DE_TIME_WINDOW_SAVED_MSG, EN_TIME_WINDOW_SAVED_MSG},
        {DE_TIME_WINDOW_RESET_TITLE, EN_TIME_WINDOW_RESET_TITLE},
        {DE_TIME_WINDOW_RESET_MSG, EN_TIME_WINDOW_RESET_MSG},
        {DE_TIME_WINDOW_HINT_ACTIVE, EN_TIME_WINDOW_HINT_ACTIVE},
        {DE_TIME_WINDOW_HINT_SLEEP, EN_TIME_WINDOW_HINT_SLEEP},
        {DE_TIME_TZ_LABEL, EN_TIME_TZ_LABEL},
        {DE_TIME_TZ_PICK_TITLE, EN_TIME_TZ_PICK_TITLE},
        {DE_TIME_TZ_NOT_SET, EN_TIME_TZ_NOT_SET},
        {DE_TIME_NTP_LABEL, EN_TIME_NTP_LABEL},
        {DE_TIME_NTP_PROMPT, EN_TIME_NTP_PROMPT},
        {DE_TIME_NTP_NOT_SET, EN_TIME_NTP_NOT_SET},
        {DE_TIME_SYNC_LABEL, EN_TIME_SYNC_LABEL},
        {DE_TIME_SYNC_PICK_TITLE, EN_TIME_SYNC_PICK_TITLE},
        {DE_TIME_SYNC_24H, EN_TIME_SYNC_24H},
        {DE_TIME_SYNC_12H, EN_TIME_SYNC_12H},
        {DE_TIME_SYNC_6H, EN_TIME_SYNC_6H},
        {DE_TIME_SYNC_3H, EN_TIME_SYNC_3H},
        {DE_TIME_SYNC_1H, EN_TIME_SYNC_1H},
        {DE_TIME_SYNC_30M, EN_TIME_SYNC_30M},
        {DE_TIME_SYNC_10M, EN_TIME_SYNC_10M},
        {DE_TIME_SYNC_1M, EN_TIME_SYNC_1M},
        {DE_TIME_MANUAL_LABEL, EN_TIME_MANUAL_LABEL},
        {DE_TIME_MANUAL_BTN, EN_TIME_MANUAL_BTN},
        {DE_TIME_MANUAL_CONFIRM_TITLE, EN_TIME_MANUAL_CONFIRM_TITLE},
        {DE_TIME_MANUAL_CONFIRM_MSG, EN_TIME_MANUAL_CONFIRM_MSG},
        {DE_TIME_MANUAL_OK, EN_TIME_MANUAL_OK},
        {DE_TIME_MANUAL_FAIL, EN_TIME_MANUAL_FAIL},
        {DE_TIME_SAVED_MSG, EN_TIME_SAVED_MSG},

        {DE_INFO_SECTION_NETWORK, EN_INFO_SECTION_NETWORK},
        {DE_INFO_SECTION_SYSTEM, EN_INFO_SECTION_SYSTEM},
        {DE_INFO_SSID, EN_INFO_SSID},
        {DE_INFO_HOSTNAME, EN_INFO_HOSTNAME},
        {DE_INFO_IP, EN_INFO_IP},
        {DE_INFO_GATEWAY, EN_INFO_GATEWAY},
        {DE_INFO_SIGNAL, EN_INFO_SIGNAL},
        {DE_INFO_MAC, EN_INFO_MAC},
        {DE_INFO_CHANNEL, EN_INFO_CHANNEL},
        {DE_INFO_NTP_SERVER, EN_INFO_NTP_SERVER},
        {DE_INFO_SYNC_STATUS, EN_INFO_SYNC_STATUS},
        {DE_INFO_MCU, EN_INFO_MCU},
        {DE_INFO_HARDWARE, EN_INFO_HARDWARE},
        {DE_INFO_FIRMWARE_SIZE, EN_INFO_FIRMWARE_SIZE},
        {DE_INFO_HEAP_FREE, EN_INFO_HEAP_FREE},
        {DE_INFO_PSRAM_FREE, EN_INFO_PSRAM_FREE},
        {DE_INFO_CHANNEL_FMT, EN_INFO_CHANNEL_FMT},
        {DE_INFO_HARDWARE_FMT, EN_INFO_HARDWARE_FMT},
        {DE_INFO_NOT_CONNECTED, EN_INFO_NOT_CONNECTED},
        {DE_INFO_NEVER, EN_INFO_NEVER},
        {DE_INFO_SYNC_OK, EN_INFO_SYNC_OK},
        {DE_INFO_SYNC_STALE, EN_INFO_SYNC_STALE},
        {DE_INFO_SIGNAL_FMT, EN_INFO_SIGNAL_FMT},
        {DE_INFO_UART, EN_INFO_UART},
        {DE_INFO_I2C, EN_INFO_I2C},
        {DE_INFO_I2C_NOT_CHECKED, EN_INFO_I2C_NOT_CHECKED},
        {DE_INFO_I2C_NONE, EN_INFO_I2C_NONE},
        {DE_INFO_I2C_BUS_BLOCKED, EN_INFO_I2C_BUS_BLOCKED},
        {DE_INFO_HEAP_FMT, EN_INFO_HEAP_FMT},
        {DE_INFO_PSRAM_FMT, EN_INFO_PSRAM_FMT},
        {DE_INFO_UPTIME_FMT, EN_INFO_UPTIME_FMT},

        {DE_AUDIO_TITLE, EN_AUDIO_TITLE},
        {DE_AUDIO_SUBTITLE, EN_AUDIO_SUBTITLE},
        {DE_AUDIO_BASS, EN_AUDIO_BASS},
        {DE_AUDIO_MID, EN_AUDIO_MID},
        {DE_AUDIO_HIGH, EN_AUDIO_HIGH},
        {DE_AUDIO_WIDTH, EN_AUDIO_WIDTH},
        {DE_AUDIO_LOUDNESS, EN_AUDIO_LOUDNESS},
        {DE_AUDIO_SPEAKERS, EN_AUDIO_SPEAKERS},
        {DE_AUDIO_SPEAKERS_ON, EN_AUDIO_SPEAKERS_ON},
        {DE_AUDIO_SPEAKERS_OFF, EN_AUDIO_SPEAKERS_OFF},
        {DE_AUDIO_HZ_FMT, EN_AUDIO_HZ_FMT},
        {DE_AUDIO_PCT_FMT, EN_AUDIO_PCT_FMT},
        {DE_AUDIO_SAVED_MSG, EN_AUDIO_SAVED_MSG},

        {DE_APPEARANCE_TITLE, EN_APPEARANCE_TITLE},
        {DE_APPEARANCE_SUBTITLE, EN_APPEARANCE_SUBTITLE},
        {DE_APPEARANCE_MAGIC_EYE, EN_APPEARANCE_MAGIC_EYE},
        {DE_APPEARANCE_EYE_EM11, EN_APPEARANCE_EYE_EM11},
        {DE_APPEARANCE_EYE_EM34, EN_APPEARANCE_EYE_EM34},
        {DE_APPEARANCE_EYE_EM80, EN_APPEARANCE_EYE_EM80},
        {DE_APPEARANCE_EYE_EM84, EN_APPEARANCE_EYE_EM84},
        {DE_APPEARANCE_SECTION_VU, EN_APPEARANCE_SECTION_VU},
        {DE_APPEARANCE_VU_BALLISTICS, EN_APPEARANCE_VU_BALLISTICS},
        {DE_APPEARANCE_BALLISTICS_HINT, EN_APPEARANCE_BALLISTICS_HINT},
        {DE_APPEARANCE_VU_LEVEL, EN_APPEARANCE_VU_LEVEL},
        {DE_APPEARANCE_LEVEL_HINT, EN_APPEARANCE_LEVEL_HINT},
        {DE_APPEARANCE_LEVEL_NOW, EN_APPEARANCE_LEVEL_NOW},
        {DE_APPEARANCE_HINT, EN_APPEARANCE_HINT},
        {DE_APPEARANCE_SAVED_MSG, EN_APPEARANCE_SAVED_MSG},
    };

    static_assert(sizeof(SETTINGS_TABLE) / sizeof(SETTINGS_TABLE[0]) == static_cast<size_t>(SettingsKey::SETTINGS_KEY_COUNT), "SETTINGS_TABLE / SettingsKey out of sync");

    // Fallback für den Fall eines fehlenden Eintrags
    const char EMPTY_STR[] PROGMEM = "";
} // namespace

// ===================================================================
// Lookup-Funktion (wird von I18n.cpp aufgerufen)
// ===================================================================

/**
 * @brief Liefert den lokalisierten Einstellungstext für Schlüssel und Sprache.
 */
const __FlashStringHelper *i18n_getSettings(SettingsKey key, LangIndex lang)
{
    const auto idx = static_cast<size_t>(key);
    if (idx >= static_cast<size_t>(SettingsKey::SETTINGS_KEY_COUNT))
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);

    auto li = static_cast<size_t>(lang);
    if (li >= static_cast<size_t>(LangIndex::COUNT))
        li = 0;

    PGM_P p = reinterpret_cast<PGM_P>(pgm_read_ptr(&SETTINGS_TABLE[idx][li]));
    if (!p && li != 0)
        p = reinterpret_cast<PGM_P>(pgm_read_ptr(&SETTINGS_TABLE[idx][0]));
    if (!p)
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);

    return reinterpret_cast<const __FlashStringHelper *>(p);
}
