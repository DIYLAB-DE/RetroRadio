/**
 * @fileoverview Google Apps Script Web-App zur Bereitstellung von Kalender-Events
 * @description Ruft Events für aktuellen und nächsten Monat ab, sowie kommende Termine
 * @author DIYLAB
 * @version 3.2 - Bereinigt von ungenutzten Today-Events
 */

function doGet(e) {
    try {
        // ============ SYNC MIT ESP32 ============
        // WICHTIG: Diese Werte MÜSSEN mit Calendar.h übereinstimmen!
        const MAX_UPCOMING_EVENTS = 25;
        const DAYS_TO_LOOK_AHEAD = 14;
        const MAX_EVENTS_PER_DAY = 5;
        // =========================================

        // === KONFIGURATION: HIER DIE KALENDER EINTRAGEN ===
        const calendarIds = [
            "primary"
        ];

        const birthdayCalendarId = "addressbook#contacts@group.v.calendar.google.com";
        const holidayCalendarId = "de.german#holiday@group.v.calendar.google.com";

        const TZ = Session.getScriptTimeZone();
        const now = new Date();
        const currentYear = now.getFullYear();
        const currentMonth = now.getMonth(); // 0-basiert

        // Zeiträume
        const nextMonth = currentMonth + 1;
        const startDate = new Date(currentYear, currentMonth, 1);
        const endDate = new Date(currentYear, currentMonth + 2, 1);

        // Heute und die folgenden Tage bis zur ausschließenden Endgrenze
        const upcomingStartDate = new Date(now);
        upcomingStartDate.setHours(0, 0, 0, 0);
        const upcomingEndDate = new Date(upcomingStartDate);
        upcomingEndDate.setDate(upcomingEndDate.getDate() + DAYS_TO_LOOK_AHEAD);
        upcomingEndDate.setHours(0, 0, 0, 0);

        console.log(`Lade Events von ${startDate.toDateString()} bis ${endDate.toDateString()}`);
        console.log(`Upcoming Events von ${upcomingStartDate.toDateString()} bis ${upcomingEndDate.toDateString()}`);

        // Events aus allen Kalendern sammeln
        let allEvents = [];
        let upcomingEvents = [];

        // Hauptkalender
        calendarIds.forEach(id => {
            try {
                const cal = id === "primary" ? CalendarApp.getDefaultCalendar() : CalendarApp.getCalendarById(id);
                if (!cal) {
                    throw new Error("Kalender nicht verfügbar: " + id);
                }
                if (cal) {
                    const events = cal.getEvents(startDate, endDate);
                    allEvents = allEvents.concat(events);

                    const upcoming = cal.getEvents(upcomingStartDate, upcomingEndDate);
                    upcomingEvents = upcomingEvents.concat(upcoming.map(e => ({
                        event: e,
                        eventType: 'default'
                    })));

                    console.log(`Kalender ${id}: ${events.length} Events gefunden`);
                }
            } catch (error) {
                console.log("Fehler beim Zugriff auf Kalender " + id + ": " + error.toString());
                throw error;
            }
        });

        // Geburtstage
        try {
            const birthdayCal = CalendarApp.getCalendarById(birthdayCalendarId);
            if (birthdayCal) {
                const birthdays = birthdayCal.getEvents(upcomingStartDate, upcomingEndDate);
                upcomingEvents = upcomingEvents.concat(birthdays.map(e => ({
                    event: e,
                    eventType: 'birthday'
                })));
                console.log(`Geburtstage: ${birthdays.length} gefunden`);
            }
        } catch (error) {
            console.log("Kein Zugriff auf Geburtstags-Kalender: " + error.toString());
        }

        // Feiertage
        try {
            const holidayCal = CalendarApp.getCalendarById(holidayCalendarId);
            if (holidayCal) {
                const holidays = holidayCal.getEvents(upcomingStartDate, upcomingEndDate);
                upcomingEvents = upcomingEvents.concat(holidays.map(e => ({
                    event: e,
                    eventType: 'holiday'
                })));
                console.log(`Feiertage: ${holidays.length} gefunden`);
            }
        } catch (error) {
            console.log("Kein Zugriff auf Feiertags-Kalender: " + error.toString());
        }

        console.log(`Gesamt: ${allEvents.length} Events in 2-Monats-Zeitraum`);
        console.log(`Upcoming vor Expansion: ${upcomingEvents.length} Events`);

        // Mehrtägige Events auf alle Tage expandieren
        upcomingEvents = expandMultiDayEvents(upcomingEvents, upcomingStartDate, upcomingEndDate);
        console.log(`Upcoming nach Expansion: ${upcomingEvents.length} Event-Einträge`);

        // Events für Monatsansichten verarbeiten
        const monthlyDaysWithEvents = {};

        const months = [
            { key: currentMonth + 1, year: currentYear },
            { key: nextMonth + 1 > 12 ? 1 : nextMonth + 1, year: nextMonth + 1 > 12 ? currentYear + 1 : currentYear }
        ];

        months.forEach(monthInfo => {
            monthlyDaysWithEvents[monthInfo.key] = {};
        });

        // Für Monatsansicht mehrtägige Events expandieren
        const expandedAllEvents = expandMultiDayEvents(allEvents.map(e => ({ event: e, eventType: 'default' })), startDate, endDate);

        expandedAllEvents.forEach(item => {
            const eventDate = item.displayDate || item.event.getStartTime();
            const eventMonth = eventDate.getMonth() + 1;
            const eventDay = eventDate.getDate();

            if (monthlyDaysWithEvents[eventMonth]) {
                monthlyDaysWithEvents[eventMonth][eventDay] = true;
            }
        });

        // Upcoming Events nach Tagen gruppieren
        const upcomingByDay = {};

        upcomingEvents.forEach(item => {
            const event = item.event;
            const eventDate = item.displayDate || event.getStartTime();
            const dateKey = Utilities.formatDate(eventDate, TZ, 'yyyy-MM-dd');

            if (!upcomingByDay[dateKey]) {
                upcomingByDay[dateKey] = {
                    date: dateKey,
                    dayName: getDayName(eventDate),
                               dayOfMonth: eventDate.getDate(),
                               month: eventDate.getMonth() + 1,
                               year: eventDate.getFullYear(),
                               isToday: isSameDay(eventDate, now),
                               events: []
                };
            }

            const isAllDay = event.isAllDayEvent();
            const eventStart = event.getStartTime();
            const eventEnd = event.getEndTime();
            const isPast = !isAllDay && eventEnd < now;

            const eventObj = {
                summary: event.getTitle(),
                               isAllDay: isAllDay,
                               isPast: isPast,
                               eventType: item.eventType
            };

            if (isAllDay) {
                eventObj.start = "Ganztägig";
            } else if (item.isFirstDay) {
                eventObj.start = eventStart.toLocaleTimeString('de-DE', {
                    hour: '2-digit',
                    minute: '2-digit'
                });
            } else {
                eventObj.start = "Ganztägig";
            }

            upcomingByDay[dateKey].events.push(eventObj);
        });

        // Events priorisieren und begrenzen
        const sortedDays = Object.keys(upcomingByDay).sort();
        const upcomingDays = [];
        let totalEventCount = 0;

        for (const dateKey of sortedDays) {
            const day = upcomingByDay[dateKey];

            // Events nach Priorität sortieren
            day.events.sort((a, b) => {
                const priorityA = getPriority(a);
                const priorityB = getPriority(b);

                if (priorityA !== priorityB) {
                    return priorityA - priorityB;
                }

                return a.start.localeCompare(b.start);
            });

            // Nur MAX_EVENTS_PER_DAY Events pro Tag behalten
            const eventsToAdd = day.events.slice(0, MAX_EVENTS_PER_DAY);
            const eventsToAddCount = Math.min(eventsToAdd.length, MAX_UPCOMING_EVENTS - totalEventCount);

            if (eventsToAddCount > 0) {
                day.events = eventsToAdd.slice(0, eventsToAddCount);
                upcomingDays.push(day);
                totalEventCount += eventsToAddCount;
            }

            if (totalEventCount >= MAX_UPCOMING_EVENTS) {
                console.log(`Event-Limit erreicht: ${MAX_UPCOMING_EVENTS} Events`);
                break;
            }
        }

        // Stelle sicher, dass heute immer dabei ist
        const todayKey = Utilities.formatDate(now, TZ, 'yyyy-MM-dd');
        if (!upcomingDays.find(d => d.date === todayKey)) {
            upcomingDays.unshift({
                date: todayKey,
                dayName: getDayName(now),
                                 dayOfMonth: now.getDate(),
                                 month: now.getMonth() + 1,
                                 year: now.getFullYear(),
                                 isToday: true,
                                 events: []
            });
        }

        function getMonthYear(monthOffset) {
            const targetMonth = currentMonth + monthOffset;
            if (targetMonth < 0) {
                return { month: 12 + targetMonth + 1, year: currentYear - 1 };
            } else if (targetMonth > 11) {
                return { month: targetMonth - 11, year: currentYear + 1 };
            }
            return { month: targetMonth + 1, year: currentYear };
        }

        const nextMonthInfo = getMonthYear(1);

        // Response-Struktur
        const result = {
            today: {
                day: now.getDate(),
                month: now.getMonth() + 1,
                year: currentYear
            },
            currentMonth: {
                month: currentMonth + 1,
                year: currentYear,
                daysWithEvents: Object.keys(monthlyDaysWithEvents[currentMonth + 1] || {}).map(Number)
            },
            nextMonth: {
                month: nextMonthInfo.month,
                year: nextMonthInfo.year,
                daysWithEvents: Object.keys(monthlyDaysWithEvents[nextMonthInfo.month] || {}).map(Number)
            },
            upcomingEvents: {
                days: upcomingDays,
                totalEventCount: totalEventCount
            }
        };

        console.log(`Response: ${upcomingDays.length} Tage, ${totalEventCount} Events gesamt`);

        return ContentService
        .createTextOutput(JSON.stringify(result))
        .setMimeType(ContentService.MimeType.JSON);

    } catch (error) {
        console.error("Fehler in doGet: " + error.toString());
        return ContentService
        .createTextOutput(JSON.stringify({
            error: error.toString(),
                                         timestamp: new Date().toISOString()
        }))
        .setMimeType(ContentService.MimeType.JSON);
    }
}

/**
 * Expandiert mehrtägige Events auf alle betroffenen Tage
 */
function expandMultiDayEvents(events, startRange, endRange) {
    const expandedEvents = [];

    events.forEach(item => {
        const event = item.event || item;
        const eventType = item.eventType || 'default';
    const eventStart = event.getStartTime();
    const eventEnd = event.getEndTime();

    const startDay = new Date(eventStart);
    startDay.setHours(0, 0, 0, 0);

    const endDay = new Date(eventEnd);
    // Wenn Event genau um Mitternacht endet, gehört dieser Tag nicht mehr dazu
    if (eventEnd.getHours() === 0 && eventEnd.getMinutes() === 0 && eventEnd.getSeconds() === 0) {
        endDay.setDate(endDay.getDate() - 1);
    }
    endDay.setHours(0, 0, 0, 0);

    const isMultiDay = startDay.getTime() !== endDay.getTime();

    if (!isMultiDay) {
        // Eintägiges Event
        if (eventStart >= startRange && eventStart < endRange) {
            expandedEvents.push({
                event: event,
                eventType: eventType,
                displayDate: eventStart,
                isFirstDay: true,
                isLastDay: true
            });
        }
    } else {
        // Mehrtägiges Event - für jeden Tag einen Eintrag
        const currentDate = new Date(startDay);

        while (currentDate <= endDay) {
            if (currentDate >= startRange && currentDate < endRange) {
                expandedEvents.push({
                    event: event,
                    eventType: eventType,
                    displayDate: new Date(currentDate),
                                    isFirstDay: currentDate.getTime() === startDay.getTime(),
                                    isLastDay: currentDate.getTime() === endDay.getTime()
                });
            }

            currentDate.setDate(currentDate.getDate() + 1);
        }
    }
    });

    return expandedEvents;
}

/**
 * Hilfsfunktion: Priorität für Sortierung
 */
function getPriority(event) {
    if (event.eventType === 'holiday') return 0;
    if (event.eventType === 'birthday') return 1;
    if (event.isAllDay) return 2;
    return 3;
}

/**
 * Hilfsfunktion: Wochentag als Abkürzung
 */
function getDayName(date) {
    const days = ['So', 'Mo', 'Di', 'Mi', 'Do', 'Fr', 'Sa'];
    return days[date.getDay()];
}

/**
 * Hilfsfunktion: Prüft ob zwei Daten am selben Tag sind
 */
function isSameDay(date1, date2) {
    return date1.getFullYear() === date2.getFullYear() &&
    date1.getMonth() === date2.getMonth() &&
    date1.getDate() === date2.getDate();
}
