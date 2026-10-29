/**
 * @fileoverview Google Apps Script Web-App zur Bereitstellung von Google Contacts
 * @description Ruft Kontakte aus Google Contacts ab und formatiert sie für ESP32
 * @author DIYLAB
 * @version 1.8
 *
 * Änderungen v1.8:
 * - Response optimiert: stats, timestamp und hasBirthdayToday entfernt
 * - ESP32 berechnet hasBirthdayToday selbst (spart ~20 Bytes pro Kontakt)
 * - Hilfsfunktionen behalten volle Funktionalität
 */

// ============ KONFIGURATION ============
const CONFIG = {
    MAX_CONTACTS: 100,  // Maximale Anzahl Kontakte

    // Welche Kontaktgruppen einbeziehen?
    // Leer = alle Kontakte
    CONTACT_GROUPS: [],

    // Nur Kontakte mit Telefonnummer?
    REQUIRE_PHONE: false,

    // Debug-Modus (mehr Logging)
    DEBUG: false
};
// =======================================

/**
 * GET-Handler: Ruft alle Kontakte ab
 * Response-Format (optimiert für ESP32):
 * {
 *   contacts: [{id, firstName, lastName, phoneHome, phoneMobile, phoneWork,
 *               email, addrStreet, addrPostalCode, addrCity, addrCountry,
 *               company, birthdayDay, birthdayMonth, birthdayYear}],
 *   today: {day, month, year}
 * }
 */
function doGet(e) {
    try {
        const TZ = Session.getScriptTimeZone();
        const now = new Date();

        console.log(`Contacts-Abruf gestartet: ${now.toISOString()}`);

        // Kontakte abrufen
        const allContacts = fetchAllContacts(now);

        // Sortieren: Nachname wenn vorhanden, sonst Vorname
        allContacts.sort((a, b) => {
            // Sortierschlüssel: Nachname bevorzugen, sonst Vorname
            const keyA = (a.lastName || a.firstName || '').toLowerCase();
            const keyB = (b.lastName || b.firstName || '').toLowerCase();
            return keyA.localeCompare(keyB, 'de');
        });

        // Auf Maximum begrenzen
        const limitedContacts = allContacts.slice(0, CONFIG.MAX_CONTACTS);

        // Für Logging: Statistiken berechnen (aber nicht senden)
        const birthdaysToday = limitedContacts.filter(c => c.hasBirthdayToday).length;

        // Response zusammenstellen (optimiert - ohne stats, timestamp, hasBirthdayToday)
        const result = {
            contacts: limitedContacts.map(c => ({
                id: c.id,
                firstName: c.firstName,
                lastName: c.lastName,
                phoneHome: c.phoneHome,
                phoneMobile: c.phoneMobile,
                phoneWork: c.phoneWork,
                email: c.email,
                addrStreet: c.addrStreet,
                addrPostalCode: c.addrPostalCode,
                addrCity: c.addrCity,
                addrCountry: c.addrCountry,
                company: c.company,
                birthdayDay: c.birthdayDay,
                birthdayMonth: c.birthdayMonth,
                birthdayYear: c.birthdayYear
                // hasBirthdayToday entfernt - ESP32 berechnet selbst
            })),
            today: {
                day: now.getDate(),
                month: now.getMonth() + 1,
                year: now.getFullYear()
            }
            // stats und timestamp entfernt - nicht vom ESP32 verwendet
        };

        console.log(`Response: ${limitedContacts.length} Kontakte, ${birthdaysToday} Geburtstage heute`);

        return ContentService
        .createTextOutput(JSON.stringify(result))
        .setMimeType(ContentService.MimeType.JSON);

    } catch (error) {
        console.error("Fehler in doGet: " + error.toString());
        return ContentService
        .createTextOutput(JSON.stringify({
            error: error.toString()
        }))
        .setMimeType(ContentService.MimeType.JSON);
    }
}

/**
 * Ruft alle Kontakte ab und formatiert sie
 * @param {Date} now - Aktuelles Datum für Geburtstags-Check
 * @returns {Array} Array von Kontakt-Objekten
 */
function fetchAllContacts(now) {
    const contacts = [];
    const today = {
        day: now.getDate(),
        month: now.getMonth() + 1
    };

    try {
        // People API verwenden (moderner als ContactsApp)
        const response = People.People.Connections.list('people/me', {
            personFields: 'names,phoneNumbers,emailAddresses,addresses,organizations,birthdays',
            pageSize: 1000,
            sortOrder: 'LAST_NAME_ASCENDING'
        });

        if (!response.connections) {
            console.log("Keine Kontakte gefunden");
            return contacts;
        }

        for (const person of response.connections) {
            const contact = parseContact(person, today);

            if (contact) {
                // Filter: Telefonnummer erforderlich?
                if (CONFIG.REQUIRE_PHONE) {
                    if (!contact.phoneHome && !contact.phoneMobile && !contact.phoneWork) {
                        continue;
                    }
                }

                contacts.push(contact);

                if (CONFIG.DEBUG) {
                    console.log(`Kontakt: ${contact.lastName}, ${contact.firstName}`);
                }
            }
        }

        console.log(`${contacts.length} Kontakte verarbeitet`);

    } catch (error) {
        console.error("Fehler beim Abrufen der Kontakte: " + error.toString());
        throw error;
    }

    return contacts;
}

/**
 * Parst einen einzelnen Kontakt aus der People API Response
 * @param {Object} person - Person-Objekt aus der API
 * @param {Object} today - Heutiges Datum {day, month}
 * @returns {Object|null} Formatiertes Kontakt-Objekt oder null
 */
function parseContact(person, today) {
    // Namen extrahieren
    let firstName = "";
    let lastName = "";

    if (person.names && person.names.length > 0) {
        const name = person.names[0];
        firstName = name.givenName || "";
        lastName = name.familyName || "";

        // Wenn kein strukturierter Name, displayName verwenden
        if (!firstName && !lastName && name.displayName) {
            const parts = name.displayName.split(' ');
            if (parts.length >= 2) {
                firstName = parts.slice(0, -1).join(' ');
                lastName = parts[parts.length - 1];
            } else {
                lastName = name.displayName;
            }
        }
    }

    // Kontakt ohne Namen überspringen
    if (!firstName && !lastName) {
        return null;
    }

    const contact = {
        id: person.resourceName || "",
        firstName: firstName,
        lastName: lastName,
        phoneHome: "",
        phoneMobile: "",
        phoneWork: "",
        email: "",
        // Adresse als einzelne Felder
        addrStreet: "",      // Straße inkl. Hausnummer
        addrPostalCode: "",  // PLZ
        addrCity: "",        // Stadt/Ort
        addrCountry: "",     // Land (oder Ländercode)
        company: "",
        birthdayDay: 0,
        birthdayMonth: 0,
        birthdayYear: 0,
        hasBirthdayToday: false  // Für Hilfsfunktionen, wird nicht an ESP32 gesendet
    };

    // Telefonnummern
    if (person.phoneNumbers) {
        for (const phone of person.phoneNumbers) {
            const type = (phone.type || "").toLowerCase();
            const number = phone.value || "";

            if (type.includes("mobile") || type.includes("mobil") || type === "cell") {
                if (!contact.phoneMobile) contact.phoneMobile = number;
            } else if (type.includes("work") || type.includes("arbeit") || type.includes("dienst") || type.includes("geschäft")) {
                if (!contact.phoneWork) contact.phoneWork = number;
            } else if (type.includes("home") || type.includes("privat") || type.includes("zuhause")) {
                if (!contact.phoneHome) contact.phoneHome = number;
            } else {
                // Unbekannter Typ -> erste freie Stelle
                if (!contact.phoneHome) contact.phoneHome = number;
                else if (!contact.phoneMobile) contact.phoneMobile = number;
                else if (!contact.phoneWork) contact.phoneWork = number;
            }
        }
    }

    // E-Mail (erste nehmen)
    if (person.emailAddresses && person.emailAddresses.length > 0) {
        contact.email = person.emailAddresses[0].value || "";
    }

    // Adresse (erste nehmen, strukturiert in Einzelfelder)
    if (person.addresses && person.addresses.length > 0) {
        const addr = person.addresses[0];

        // PLZ, Stadt und Land aus strukturierten Feldern
        contact.addrPostalCode = addr.postalCode || "";
        contact.addrCity = addr.city || "";
        contact.addrCountry = addr.country || addr.countryCode || "";

        // Straße: formattedValue ist lokalisiert (zeigt korrektes Format wie Web-UI)
        // streetAddress ist oft im US-Format (Nummer vor Straße)
        if (addr.formattedValue) {
            // Erste Zeile von formattedValue = Straße mit Hausnummer (korrekt formatiert)
            const lines = addr.formattedValue.split('\n');
            contact.addrStreet = lines[0] || "";
        } else {
            // Fallback auf streetAddress wenn kein formattedValue
            contact.addrStreet = addr.streetAddress || "";
        }
    }

    // Firma/Organisation
    if (person.organizations && person.organizations.length > 0) {
        contact.company = person.organizations[0].name || "";
    }

    // Geburtstag
    if (person.birthdays && person.birthdays.length > 0) {
        const birthday = person.birthdays[0];

        if (birthday.date) {
            contact.birthdayDay = birthday.date.day || 0;
            contact.birthdayMonth = birthday.date.month || 0;
            contact.birthdayYear = birthday.date.year || 0;

            // Prüfen ob heute Geburtstag (für Hilfsfunktionen)
            if (contact.birthdayDay === today.day && contact.birthdayMonth === today.month) {
                contact.hasBirthdayToday = true;
            }
        }
    }

    return contact;
}

/**
 * Test-Funktion für doGet
 */
function testDoGet() {
    const result = doGet({ parameter: {} });
    console.log(result.getContent());
}

/**
 * Hilfsfunktion: Zeigt alle verfügbaren Kontakte mit Details
 * Aufruf über Script-Editor: Ausführen -> listAllContacts
 */
function listAllContacts() {
    const contacts = fetchAllContacts(new Date());

    console.log("=== Alle Kontakte ===");
    for (const c of contacts) {
        const addrInfo = c.addrCity ? `${c.addrPostalCode} ${c.addrCity}` : '-';
        console.log(`${c.lastName}, ${c.firstName} | Tel: ${c.phoneMobile || c.phoneHome || '-'} | Ort: ${addrInfo} | Geb: ${c.birthdayDay}.${c.birthdayMonth}`);
    }
    console.log(`\nGesamt: ${contacts.length} Kontakte`);
}

/**
 * Hilfsfunktion: Zeigt Kontakte mit heutigem Geburtstag
 */
function showBirthdaysToday() {
    const contacts = fetchAllContacts(new Date());
    const birthdayContacts = contacts.filter(c => c.hasBirthdayToday);

    console.log("=== Geburtstage heute ===");
    if (birthdayContacts.length === 0) {
        console.log("Keine Geburtstage heute");
    } else {
        for (const c of birthdayContacts) {
            console.log(`${c.firstName} ${c.lastName}`);
        }
    }
}
