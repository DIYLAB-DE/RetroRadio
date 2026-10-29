#ifndef STRINGS_SETTINGS_H
#define STRINGS_SETTINGS_H

#include <stdint.h>

/**
 * @brief Schlüssel für anwender-sichtbare Texte des Settings-Moduls
 *
 * Beim Hinzufügen eines Keys auch die Pointer-Tabelle in `Strings_settings.cpp` erweitern. Der `static_assert` dort verhindert eine Drift zwischen Enum und Tabelle. Die Reihenfolge hier muss exakt der Tabellenreihenfolge entsprechen.
 */
enum class SettingsKey : uint16_t
{
    SETTINGS_EMPTY = 0, ///< Sonder-Key: leerer Text

    // ===================================================================
    // Sektionsköpfe im Menü
    // ===================================================================
    SETTINGS_SEC_CONNECTION, ///< "verbindung" / "connectivity"
    SETTINGS_SEC_SERVICES,   ///< "dienste" / "services"
    SETTINGS_SEC_SYSTEM,     ///< "system" / "system"

    // ===================================================================
    // Menü-Einträge
    // ===================================================================
    SETTINGS_MENU_WIFI,     ///< "WLAN" / "Wi-Fi"
    SETTINGS_MENU_TIME,     ///< "Zeit & Zeitserver" / "Time & time server"
    SETTINGS_MENU_TIME_WINDOW, ///< "Zeitfenster" / "Time windows"
    SETTINGS_MENU_WEATHER,  ///< "Wetter" / "Weather"
    SETTINGS_MENU_NEWS,     ///< "Nachrichten" / "News"
    SETTINGS_MENU_CLOUD,    ///< "Cloud-Anbindung" / "Cloud sync"
    SETTINGS_MENU_LANGUAGE, ///< "Sprache" / "Language"
    SETTINGS_MENU_SECURITY, ///< "Sicherheit" / "Security"
    SETTINGS_MENU_AUDIO,    ///< "Klang" / "Sound"
    SETTINGS_MENU_APPEARANCE,  ///< "Anzeige" / "Display"
    SETTINGS_MENU_INFO,     ///< "Info & Neustart" / "Info & restart"

    // ===================================================================
    // Fußzeile
    // ===================================================================
    SETTINGS_FOOTER_SAVE, ///< "speichern" / "save"

    // ===================================================================
    // Platzhalter-Panels (Dienste, noch in Vorbereitung)
    // ===================================================================
    SETTINGS_PLACEHOLDER_TITLE,   ///< "In Vorbereitung" / "Coming soon"
    SETTINGS_PLACEHOLDER_MESSAGE, ///< "Dieser Bereich wird gerade gebaut." / "This area is being built."

    // ===================================================================
    // WLAN-Panel
    // ===================================================================
    SETTINGS_WIFI_TITLE,    ///< "WLAN" / "Wi-Fi"
    SETTINGS_WIFI_SUBTITLE, ///< "Profile einzeln aktivierbar" / "Profiles individually switchable"

    SETTINGS_WIFI_PROFILE_FMT,      ///< "Profil {0:%d}" / "Profile {0:%d}"
    SETTINGS_WIFI_EMPTY_NAME,       ///< "(leer)" / "(empty)"
    SETTINGS_WIFI_STATUS_CONNECTED, ///< "verbunden" / "connected"
    SETTINGS_WIFI_STATUS_ACTIVE,    ///< "aktiv (bereit)" / "active (ready)"
    SETTINGS_WIFI_STATUS_INACTIVE,  ///< "inaktiv" / "inactive"
    SETTINGS_WIFI_STATUS_EMPTY,     ///< "kein Netz hinterlegt" / "no network set"

    SETTINGS_WIFI_ACT_SCAN,     ///< "Scannen" / "Scan"
    SETTINGS_WIFI_ACT_PASSWORD, ///< "Passwort" / "Password"
    SETTINGS_WIFI_ACT_TEST,     ///< "Test" / "Test"
    SETTINGS_WIFI_ACT_DELETE,   ///< "Löschen" / "Delete"

    SETTINGS_WIFI_SCAN_TITLE,   ///< "Netzwerk wählen:" / "Choose network:"
    SETTINGS_WIFI_PW_TITLE_FMT, ///< "Passwort für {0}:" / "Password for {0}:"

    SETTINGS_WIFI_NEED_SSID_TITLE, ///< "Kein Netz gewählt" / "No network chosen"
    SETTINGS_WIFI_NEED_SSID_MSG,   ///< "Bitte zuerst ein Netzwerk wählen." / "Please choose a network first."

    SETTINGS_WIFI_PW_BAD_TITLE,    ///< "Ungültiges Passwort" / "Invalid password"
    SETTINGS_WIFI_PW_ERR_LEN,      ///< "Passwort muss 8 bis 63 Zeichen lang sein." / "Password must be 8 to 63 characters long."
    SETTINGS_WIFI_PW_ERR_CHARS,    ///< "Bitte nur normale Tastaturzeichen verwenden." / "Please use standard keyboard characters only."
    SETTINGS_WIFI_PW_ERR_HEX,      ///< "64 Zeichen erlauben nur die Ziffern 0-9 und A-F." / "64 characters allow only digits 0-9 and A-F."
    SETTINGS_WIFI_PW_ERR_INTERNAL, ///< "Interner Fehler." / "Internal error."

    SETTINGS_WIFI_DELETE_TITLE, ///< "Profil löschen" / "Delete profile"
    SETTINGS_WIFI_DELETE_MSG,   ///< "Dieses Profil wirklich dauerhaft löschen?" / "Really delete this profile permanently?"

    SETTINGS_WIFI_TEST_BOX_TITLE,   ///< "Verbindungstest" / "Connection test"
    SETTINGS_WIFI_TEST_PREPARE,     ///< "Verbindung wird vorbereitet..." / "Preparing connection..."
    SETTINGS_WIFI_TEST_RUNNING,     ///< "Verbindung wird getestet..." / "Testing connection..."
    SETTINGS_WIFI_TEST_OK_TITLE,    ///< "Test erfolgreich" / "Test successful"
    SETTINGS_WIFI_TEST_OK_FMT,      ///< "Verbunden.\n\nIP: {0}\nSignal: {1:%d} dBm" (sprachneutral bis auf "Verbunden")
    SETTINGS_WIFI_TEST_FAIL_TITLE,  ///< "Test fehlgeschlagen" / "Test failed"
    SETTINGS_WIFI_TEST_FAIL_MSG,    ///< "Verbindung fehlgeschlagen.\nSSID und Passwort prüfen." / "Connection failed.\nCheck SSID and password."
    SETTINGS_WIFI_TEST_TIMEOUT_MSG, ///< "Zeitüberschreitung.\nSSID und Passwort prüfen." / "Timed out.\nCheck SSID and password."

    SETTINGS_WIFI_HOSTNAME_LABEL,     ///< "Netzwerkname" / "Network name"
    SETTINGS_WIFI_HOSTNAME_PROMPT,    ///< "Netzwerkname:" / "Network name:"
    SETTINGS_WIFI_HOSTNAME_EDIT,      ///< "ändern" / "edit"
    SETTINGS_WIFI_HOSTNAME_BAD_TITLE, ///< "Ungültiger Name" / "Invalid name"
    SETTINGS_WIFI_HOSTNAME_ERR,       ///< Hinweis auf erlaubte Zeichen und Länge

    // ===================================================================
    // Speichern und Verlassen
    // ===================================================================
    SETTINGS_SAVED_TITLE,       ///< "Gespeichert" / "Saved"
    SETTINGS_SAVED_MSG,         ///< "Einstellungen gespeichert.\nEin Neustart übernimmt die Änderungen." / "Settings saved.\nA restart applies the changes."
    SETTINGS_SAVED_RESTART_BTN, ///< "neustart" / "restart"
    SETTINGS_SAVED_LATER_BTN,   ///< "später" / "later"

    SETTINGS_EXIT_TITLE,       ///< "Ungespeicherte Änderungen" / "Unsaved changes"
    SETTINGS_EXIT_MSG,         ///< "Änderungen speichern?" / "Save changes?"
    SETTINGS_EXIT_DISCARD_BTN, ///< "verwerfen" / "discard"

    // ===================================================================
    // Sprache-Panel
    // ===================================================================
    SETTINGS_LANG_TITLE,    ///< "Sprache" / "Language"
    SETTINGS_LANG_SUBTITLE, ///< "Sprache der Oberfläche" / "Interface language"
    SETTINGS_LANG_ACTIVE,   ///< "aktiv" / "active"
    SETTINGS_LANG_NAME_DE,  ///< "Deutsch" / "Deutsch" (Endonym, in beiden Sprachen gleich)
    SETTINGS_LANG_NAME_EN,  ///< "English" / "English" (Endonym, in beiden Sprachen gleich)

    // ===================================================================
    // Sicherheit-Panel
    // ===================================================================
    SETTINGS_SEC_TITLE,       ///< "Sicherheit" / "Security"
    SETTINGS_SEC_SUBTITLE,    ///< "Sperr-PIN ändern" / "Change lock PIN"
    SETTINGS_SEC_PIN_HINT,    ///< "Tippe 4 Ziffern für eine neue PIN." / "Type 4 digits for a new PIN."
    SETTINGS_SEC_PIN_SET,     ///< "Eine PIN ist gesetzt." / "A PIN is set."
    SETTINGS_SEC_SAVED_TITLE, ///< "PIN geändert" / "PIN changed"
    SETTINGS_SEC_SAVED_MSG,   ///< "Die neue PIN wurde gespeichert." / "The new PIN has been saved."
    SETTINGS_SEC_PIN_REPEAT,   ///< "PIN wiederholen" / "Repeat PIN"
    SETTINGS_SEC_PIN_MISMATCH, ///< "Eingaben stimmen nicht überein" / "Entries do not match"
    SETTINGS_SEC_PIN_READY,    ///< "PIN vorgemerkt. Mit Speichern bestätigen." / "PIN ready. Confirm with Save."

    // ===================================================================
    // Info-Panel
    // ===================================================================
    SETTINGS_INFO_TITLE,        ///< "Info & Neustart" / "Info & restart"
    SETTINGS_INFO_SUBTITLE,     ///< "Geräteinformationen" / "Device information"
    SETTINGS_INFO_FIRMWARE,     ///< "Version" / "Version"
    SETTINGS_INFO_UPTIME,       ///< "Laufzeit" / "Uptime"
    SETTINGS_INFO_RESTART_BTN,   ///< "Neustart" / "Restart"
    SETTINGS_INFO_RESTART_TITLE, ///< "Neustart" / "Restart"
    SETTINGS_INFO_RESTART_MSG,   ///< "Wirklich neu starten?\nUngespeicherte Änderungen gehen verloren." / "Really restart?\nUnsaved changes will be lost."
    SETTINGS_INFO_RESET_BTN,     ///< "Zurücksetzen" / "Reset"
    SETTINGS_INFO_RESET_TITLE,   ///< "Zurücksetzen" / "Reset"
    SETTINGS_INFO_RESET_MSG,     ///< "Auf Werkseinstellungen zurücksetzen?\nAlle Profile und Einstellungen gehen verloren." / "Reset to factory settings?\nAll profiles and settings will be lost."

    SETTINGS_LANG_RESTART_TITLE, ///< "Sprache ändern" / "Change language"
    SETTINGS_LANG_RESTART_MSG,   ///< "Die neue Sprache gilt erst nach einem Neustart.\nJetzt neu starten?" / "The new language takes effect after a restart.\nRestart now?"

    // ===================================================================
    // Nachrichten-Panel
    // ===================================================================
    SETTINGS_NEWS_TITLE,         ///< "Nachrichten" / "News"
    SETTINGS_NEWS_SUBTITLE,      ///< "Themen und Bundesland" / "Topics and state"
    SETTINGS_NEWS_RESSORT_LABEL, ///< "Themen" / "Topics"
    SETTINGS_NEWS_REGION_LABEL,  ///< "Bundesland" / "State"

    SETTINGS_NEWS_RESSORT_ALL,          ///< "alle" / "all"
    SETTINGS_NEWS_RESSORT_INLAND,       ///< "Inland" / "Domestic"
    SETTINGS_NEWS_RESSORT_AUSLAND,      ///< "Ausland" / "International"
    SETTINGS_NEWS_RESSORT_WIRTSCHAFT,   ///< "Wirtschaft" / "Economy"
    SETTINGS_NEWS_RESSORT_SPORT,        ///< "Sport" / "Sport"
    SETTINGS_NEWS_RESSORT_VIDEO,        ///< "Video" / "Video"
    SETTINGS_NEWS_RESSORT_INVESTIGATIV, ///< "Investigativ" / "Investigative"
    SETTINGS_NEWS_RESSORT_WISSEN,       ///< "Wissen" / "Science"

    SETTINGS_NEWS_REGION_PICK_TITLE, ///< "Bundesland wählen:" / "Choose state:"

    SETTINGS_NEWS_BL_BW, ///< "Baden-Württemberg" / "Baden-Württemberg"
    SETTINGS_NEWS_BL_BY, ///< "Bayern" / "Bavaria"
    SETTINGS_NEWS_BL_BE, ///< "Berlin" / "Berlin"
    SETTINGS_NEWS_BL_BB, ///< "Brandenburg" / "Brandenburg"
    SETTINGS_NEWS_BL_HB, ///< "Bremen" / "Bremen"
    SETTINGS_NEWS_BL_HH, ///< "Hamburg" / "Hamburg"
    SETTINGS_NEWS_BL_HE, ///< "Hessen" / "Hesse"
    SETTINGS_NEWS_BL_MV, ///< "Mecklenburg-Vorpommern" / "Mecklenburg-Western Pomerania"
    SETTINGS_NEWS_BL_NI, ///< "Niedersachsen" / "Lower Saxony"
    SETTINGS_NEWS_BL_NW, ///< "Nordrhein-Westfalen" / "North Rhine-Westphalia"
    SETTINGS_NEWS_BL_RP, ///< "Rheinland-Pfalz" / "Rhineland-Palatinate"
    SETTINGS_NEWS_BL_SL, ///< "Saarland" / "Saarland"
    SETTINGS_NEWS_BL_SN, ///< "Sachsen" / "Saxony"
    SETTINGS_NEWS_BL_ST, ///< "Sachsen-Anhalt" / "Saxony-Anhalt"
    SETTINGS_NEWS_BL_SH, ///< "Schleswig-Holstein" / "Schleswig-Holstein"
    SETTINGS_NEWS_BL_TH, ///< "Thüringen" / "Thuringia"

    SETTINGS_NEWS_SAVED_MSG, ///< "Die Filter für die Nachrichten wurden gespeichert." / "The news filters have been saved."

    // ===================================================================
    // Wetter-Panel
    // ===================================================================
    SETTINGS_WEATHER_TITLE,         ///< "Wetter" / "Weather"
    SETTINGS_WEATHER_SUBTITLE,      ///< "Standort über Postleitzahl" / "Location via postal code"
    SETTINGS_WEATHER_PLZ_LABEL,     ///< "Postleitzahl" / "Postal code"
    SETTINGS_WEATHER_LOCATION_LABEL,///< "Ort" / "Place"
    SETTINGS_WEATHER_COORDS_LABEL,  ///< "Koordinaten" / "Coordinates"
    SETTINGS_WEATHER_NOT_RESOLVED,  ///< "noch nicht ermittelt" / "not yet determined"
    SETTINGS_WEATHER_NO_WIFI_HINT,  ///< "Ohne WLAN kann der Ort nicht ermittelt werden." / "Without Wi-Fi the place cannot be determined."
    SETTINGS_WEATHER_RESOLVING,     ///< "Ort wird ermittelt..." / "Determining place..."
    SETTINGS_WEATHER_NO_WIFI,       ///< "Kein WLAN - der Ort wird ermittelt, sobald eine Verbindung besteht." / "No Wi-Fi - the place will be determined as soon as there is a connection."
    SETTINGS_WEATHER_NOT_FOUND,     ///< "Diese Postleitzahl wurde nicht gefunden." / "This postal code was not found."
    SETTINGS_WEATHER_OUTSIDE_DE,    ///< "Diese Postleitzahl liegt außerhalb Deutschlands." / "This postal code is outside Germany."
    SETTINGS_WEATHER_LOOKUP_FAILED, ///< "Die Suche ist fehlgeschlagen - der Ort wird beim nächsten Mal neu ermittelt." / "The lookup failed - the place will be determined the next time."
    SETTINGS_WEATHER_SAVED_FMT,     ///< "Postleitzahl gespeichert.\nOrt: {0}" / "Postal code saved.\nPlace: {0}"
    SETTINGS_WEATHER_SAVED_MSG,     ///< "Die Postleitzahl wurde gespeichert. Der Ort wird beim nächsten Wetter-Abruf neu ermittelt." / "The postal code has been saved. The place will be determined at the next weather update."

    // ===================================================================
    // Cloud-Anbindung
    // ===================================================================
    SETTINGS_CLOUD_TITLE,           ///< "Cloud-Anbindung" / "Cloud sync"
    SETTINGS_CLOUD_SUBTITLE,        ///< "Kalender, Aufgaben, Kontakte, OpenSky" / "Calendar, tasks, contacts, OpenSky"

    SETTINGS_CLOUD_CALENDAR_LABEL,  ///< "Kalender" / "Calendar"
    SETTINGS_CLOUD_TODO_LABEL,      ///< "Aufgaben" / "Tasks"
    SETTINGS_CLOUD_CONTACTS_LABEL,  ///< "Kontakte" / "Contacts"
    SETTINGS_CLOUD_OPENSKY_ID_LABEL, ///< "OpenSky Client-ID" / "OpenSky client ID"
    SETTINGS_CLOUD_OPENSKY_SECRET_LABEL, ///< "OpenSky Client-Secret" / "OpenSky client secret"

    SETTINGS_CLOUD_CALENDAR_PROMPT, ///< "Kalender-Adresse:" / "Calendar address:"
    SETTINGS_CLOUD_TODO_PROMPT,     ///< "Aufgaben-Adresse:" / "Tasks address:"
    SETTINGS_CLOUD_CONTACTS_PROMPT, ///< "Kontakte-Adresse:" / "Contacts address:"
    SETTINGS_CLOUD_OPENSKY_ID_PROMPT, ///< "OpenSky Client-ID:" / "OpenSky client ID:"
    SETTINGS_CLOUD_OPENSKY_SECRET_PROMPT, ///< "OpenSky Client-Secret:" / "OpenSky client secret:"

    SETTINGS_CLOUD_NOT_SET,         ///< "nicht hinterlegt" / "not set"

    SETTINGS_CLOUD_CLEAR_BTN,       ///< "alle löschen" / "clear all"
    SETTINGS_CLOUD_CLEAR_TITLE,     ///< "Alle Adressen löschen" / "Clear all addresses"
    SETTINGS_CLOUD_CLEAR_MSG,       ///< "Alle drei Cloud-Adressen wirklich löschen?\nDie Dienste sind danach nicht mehr erreichbar." / "Really clear all three cloud addresses?\nThe services will no longer be reachable."

    SETTINGS_CLOUD_SAVED_MSG,       ///< "Die Cloud-Adressen wurden gespeichert." / "The cloud addresses have been saved."

    SETTINGS_CLOUD_INVALID_TITLE,   ///< "Ungültige Adressen" / "Invalid addresses"
    SETTINGS_CLOUD_INVALID_PREFIX,  ///< "muss mit https://script.google.com/ beginnen" / "must start with https://script.google.com/"
    SETTINGS_CLOUD_INVALID_SUFFIX,  ///< "muss auf /exec enden" / "must end with /exec"

    // ===================================================================
    // Zeit-Panel
    // ===================================================================
    SETTINGS_TIME_TITLE,             ///< "Zeit & Zeitserver" / "Time & time server"
    SETTINGS_TIME_SUBTITLE,          ///< "Zeitzone, Zeitserver und manuelle Zeit" / "Time zone, time server and manual time"
    SETTINGS_TIME_WINDOW_TITLE,       ///< "Zeitfenster" / "Time windows"
    SETTINGS_TIME_WINDOW_SUBTITLE,    ///< "Wochentage und volle Stunden" / "Weekdays and full hours"
    SETTINGS_TIME_WINDOW_SAVED_MSG,   ///< "Die Zeitfenster wurden gespeichert." / "The time windows have been saved."
    SETTINGS_TIME_WINDOW_RESET_TITLE, ///< "Zeitfenster zurücksetzen" / "Reset time windows"
    SETTINGS_TIME_WINDOW_RESET_MSG,   ///< "Alle Zeitfenster auf aktiv setzen?" / "Set all time windows to active?"
    SETTINGS_TIME_WINDOW_HINT_ACTIVE,  ///< "Angehakt: Das Gerät bleibt in dieser vollen Stunde aktiv." / "Checked: device stays active during this full hour."
    SETTINGS_TIME_WINDOW_HINT_SLEEP,   ///< "Leer: Display und Radio schlafen. Berühren weckt bis zur nächsten Stunde." / "Empty: display and radio sleep. Touch wakes until the next hour."

    SETTINGS_TIME_TZ_LABEL,          ///< "Zeitzone" / "Time zone"
    SETTINGS_TIME_TZ_PICK_TITLE,     ///< "Zeitzone wählen" / "Choose time zone"
    SETTINGS_TIME_TZ_NOT_SET,        ///< "noch nicht gewählt" / "not yet chosen"

    SETTINGS_TIME_NTP_LABEL,         ///< "Zeitserver (NTP)" / "Time server (NTP)"
    SETTINGS_TIME_NTP_PROMPT,        ///< "Zeitserver:" / "Time server:"
    SETTINGS_TIME_NTP_NOT_SET,       ///< "kein Server hinterlegt" / "no server set"

    SETTINGS_TIME_SYNC_LABEL,        ///< "Sync-Intervall" / "Sync interval"
    SETTINGS_TIME_SYNC_PICK_TITLE,   ///< "Sync-Intervall wählen" / "Choose sync interval"
    SETTINGS_TIME_SYNC_24H,          ///< "24 Stunden" / "24 hours"
    SETTINGS_TIME_SYNC_12H,          ///< "12 Stunden" / "12 hours"
    SETTINGS_TIME_SYNC_6H,           ///< "6 Stunden" / "6 hours"
    SETTINGS_TIME_SYNC_3H,           ///< "3 Stunden" / "3 hours"
    SETTINGS_TIME_SYNC_1H,           ///< "1 Stunde" / "1 hour"
    SETTINGS_TIME_SYNC_30M,          ///< "30 Minuten" / "30 minutes"
    SETTINGS_TIME_SYNC_10M,          ///< "10 Minuten" / "10 minutes"
    SETTINGS_TIME_SYNC_1M,           ///< "1 Minute (nicht empfohlen)" / "1 minute (not recommended)"

    SETTINGS_TIME_MANUAL_LABEL,      ///< "Zeit manuell setzen" / "Set time manually"
    SETTINGS_TIME_MANUAL_BTN,        ///< "Zeit setzen" / "Set time"
    SETTINGS_TIME_MANUAL_CONFIRM_TITLE, ///< "Zeit setzen" / "Set time"
    SETTINGS_TIME_MANUAL_CONFIRM_MSG,   ///< "Die Uhr wirklich auf den eingestellten Wert setzen?\nDer nächste Zeitserver-Abgleich kann sie wieder ändern." / "Really set the clock to the chosen value?\nThe next time server sync may change it again."
    SETTINGS_TIME_MANUAL_OK,         ///< "Die Uhr wurde gesetzt." / "The clock has been set."
    SETTINGS_TIME_MANUAL_FAIL,       ///< "Die Uhr konnte nicht gesetzt werden." / "The clock could not be set."

    SETTINGS_TIME_SAVED_MSG,         ///< "Die Zeit-Einstellungen wurden gespeichert." / "The time settings have been saved."

    // ===================================================================
    // Info-Panel (erweiterte Anzeige)
    // ===================================================================
    SETTINGS_INFO_SECTION_NETWORK,   ///< "Netzwerk" / "Network"
    SETTINGS_INFO_SECTION_SYSTEM,    ///< "System" / "System"
    SETTINGS_INFO_SSID,              ///< "SSID" / "SSID"
    SETTINGS_INFO_HOSTNAME,          ///< "Host-Name" / "Host name"
    SETTINGS_INFO_IP,                ///< "IP-Adresse" / "IP address"
    SETTINGS_INFO_GATEWAY,           ///< "Gateway" / "Gateway"
    SETTINGS_INFO_SIGNAL,            ///< "Signal" / "Signal"
    SETTINGS_INFO_MAC,               ///< "MAC-Adresse" / "MAC address"
    SETTINGS_INFO_CHANNEL,           ///< "Kanal" / "Channel"
    SETTINGS_INFO_NTP_SERVER,        ///< "Zeitserver" / "Time server"
    SETTINGS_INFO_SYNC_STATUS,       ///< "l.Sync" / "l.Sync"
    SETTINGS_INFO_MCU,               ///< "MCU" / "MCU"
    SETTINGS_INFO_HARDWARE,          ///< "Hardware" / "Hardware"
    SETTINGS_INFO_FIRMWARE_SIZE,     ///< "FW" / "FW"
    SETTINGS_INFO_HEAP_FREE,         ///< "Heap" / "Heap"
    SETTINGS_INFO_PSRAM_FREE,        ///< "PSRAM" / "PSRAM"
    SETTINGS_INFO_CHANNEL_FMT,       ///< "{0:%d} (2,4 GHz)" / "{0:%d} (2.4 GHz)"
    SETTINGS_INFO_HARDWARE_FMT,      ///< "Flash: {0:%d} MB, PSRAM: {1:%d} MB" / "Flash: {0:%d} MB, PSRAM: {1:%d} MB"
    SETTINGS_INFO_NOT_CONNECTED,     ///< "nicht verbunden" / "not connected"
    SETTINGS_INFO_NEVER,             ///< "noch nie" / "never"
    SETTINGS_INFO_SYNC_OK,            ///< "aktuell" / "current"
    SETTINGS_INFO_SYNC_STALE,         ///< "veraltet" / "stale"
    SETTINGS_INFO_SIGNAL_FMT,        ///< "{0:%d} dB ({1:%d}%)" / "{0:%d} dB ({1:%d}%)"
    SETTINGS_INFO_UART,              ///< "UART" / "UART"
    SETTINGS_INFO_I2C,               ///< "I2C" / "I2C"
    SETTINGS_INFO_I2C_NOT_CHECKED,   ///< "nicht geprüft" / "not checked"
    SETTINGS_INFO_I2C_NONE,          ///< "keine Geräte" / "no devices"
    SETTINGS_INFO_I2C_BUS_BLOCKED,   ///< "Bus blockiert" / "bus blocked"
    SETTINGS_INFO_HEAP_FMT,          ///< "{0:%d} KB frei ({1:%d}% fragm.)" / "{0:%d} KB free ({1:%d}% frag.)"
    SETTINGS_INFO_PSRAM_FMT,         ///< "{0:%d} KB frei ({1:%d}%)" / "{0:%d} KB free ({1:%d}%)"
    SETTINGS_INFO_UPTIME_FMT,        ///< "{0:%d}T, {1:%d}Std, {2:%d}Min ({3:%d}x)" / "{0:%d}d, {1:%d}h, {2:%d}min ({3:%d}x)"

    // ===================================================================
    // Klang-Panel (Eckfrequenzen der Klangregelung)
    // ===================================================================
    SETTINGS_AUDIO_TITLE,     ///< "Klang" / "Sound"
    SETTINGS_AUDIO_SUBTITLE,  ///< "Eckfrequenzen, Basisbreite und Loudness" / "Tone corner frequencies, stereo width and loudness"
    SETTINGS_AUDIO_BASS,      ///< "Tiefen" / "Bass"
    SETTINGS_AUDIO_MID,       ///< "Mitten" / "Mids"
    SETTINGS_AUDIO_HIGH,      ///< "Höhen" / "Treble"
    SETTINGS_AUDIO_WIDTH,     ///< "Basisbreite" / "Stereo width"
    SETTINGS_AUDIO_LOUDNESS,  ///< "Loudness" / "Loudness"
    SETTINGS_AUDIO_SPEAKERS,  ///< "Lautsprecher" / "Speakers"
    SETTINGS_AUDIO_SPEAKERS_ON, ///< "An" / "On"
    SETTINGS_AUDIO_SPEAKERS_OFF, ///< "Aus" / "Off"
    SETTINGS_AUDIO_HZ_FMT,    ///< "{0:%d} Hz" / "{0:%d} Hz"
    SETTINGS_AUDIO_PCT_FMT,   ///< "{0:%d} %" / "{0:%d} %"
    SETTINGS_AUDIO_SAVED_MSG, ///< "Die Klang-Einstellungen wurden gespeichert." / "The sound settings have been saved."

    // ===================================================================
    // Anzeige-Panel (Darstellung der Skala)
    // ===================================================================
    SETTINGS_APPEARANCE_TITLE,       ///< "Anzeige" / "Display"
    SETTINGS_APPEARANCE_SUBTITLE,    ///< "Magisches Auge und analoges VU-Meter" / "Magic eye and analogue VU meter"
    SETTINGS_APPEARANCE_MAGIC_EYE,   ///< "Magisches Auge" / "Magic eye"
    SETTINGS_APPEARANCE_EYE_EM11, ///< "EM11" / "EM11"
    SETTINGS_APPEARANCE_EYE_EM34,    ///< "EM34" / "EM34"
    SETTINGS_APPEARANCE_EYE_EM80,    ///< "EM80" / "EM80"
    SETTINGS_APPEARANCE_EYE_EM84,    ///< "EM84" / "EM84"
    SETTINGS_APPEARANCE_SECTION_VU,  ///< "Analoges VU-Meter" / "Analogue VU meter"
    SETTINGS_APPEARANCE_VU_BALLISTICS, ///< "Zeigerträgheit" / "Needle ballistics"
    SETTINGS_APPEARANCE_BALLISTICS_HINT, ///< Erklärung der Zeitangaben
    SETTINGS_APPEARANCE_VU_LEVEL,    ///< "Bezugspegel" / "Reference level"
    SETTINGS_APPEARANCE_LEVEL_HINT,  ///< Erklärung des Bezugspegels
    SETTINGS_APPEARANCE_LEVEL_NOW,   ///< "Sender liefert" / "source delivers"
    SETTINGS_APPEARANCE_HINT,        ///< "Ein langer Druck auf das Auge schaltet ebenfalls um." / "A long press on the eye switches as well."
    SETTINGS_APPEARANCE_SAVED_MSG,   ///< "Anzeige gespeichert." / "Display settings saved."

    SETTINGS_KEY_COUNT ///< Endmarker für Sanity-Checks
};

#endif // STRINGS_SETTINGS_H
