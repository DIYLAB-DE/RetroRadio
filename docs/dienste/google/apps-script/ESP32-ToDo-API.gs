/**
 * @fileoverview Google Apps Script Web-App zur Bereitstellung von Google Tasks
 * @description Ruft Tasks aus ALLEN Listen automatisch ab und ermöglicht das Markieren als erledigt
 * @author DIYLAB
 * @version 1.5
 */

// ============ KONFIGURATION ============
const CONFIG = {
    MAX_TASKS: 30,              // Maximale Anzahl Tasks
    DAYS_OVERDUE_HIGHLIGHT: 7,  // Tage für "stark überfällig" Markierung

    // Automatisch alle Listen verwenden (true) oder nur bestimmte (false)
    USE_ALL_LISTS: true,

    // Nur relevant wenn USE_ALL_LISTS = false:
    // Liste-IDs die verwendet werden sollen
    TASK_LISTS: [
        "@default"
    ],

    // Optionale Filterung: nur Tasks mit bestimmten Titelpräfixen
    // Leer lassen für alle Tasks: []
    TITLE_PREFIXES: [],

    // Prioritätsreihenfolge der Listen (erste = oben)
    // Listen die hier nicht aufgeführt sind, kommen am Ende (alphabetisch)
    LIST_PRIORITY: [
        "Wichtige Aufgaben",
        "Aufgaben"
    ]
};
// =======================================

/**
 * Holt alle verfügbaren Tasklisten des Benutzers
 * @returns {Array} Array von Listen-Objekten mit id und title
 */
function getAllTaskListIds() {
    const listIds = [];

    try {
        const response = Tasks.Tasklists.list();

        if (response.items) {
            for (const list of response.items) {
                listIds.push({
                    id: list.id,
                    title: list.title
                });
                console.log(`Gefundene Liste: "${list.title}" (${list.id})`);
            }
        }
    } catch (error) {
        console.error("Fehler beim Abrufen der Tasklisten: " + error.toString());
        throw error;
    }

    return listIds;
}

/**
 * GET-Handler: Ruft alle Tasks ab ODER aktualisiert einen Task
 *
 * Abruf: GET /exec
 * Update: GET /exec?action=update&taskId=xxx&listId=xxx&completed=true
 */
function doGet(e) {
    try {
        // Prüfen ob es ein Update-Request ist
        if (e.parameter && e.parameter.action === "update") {
            return handleTaskUpdate(e.parameter);
        }

        const TZ = Session.getScriptTimeZone();
        const now = new Date();

        console.log(`Tasks-Abruf gestartet: ${now.toISOString()}`);

        // Listen ermitteln (alle oder nur konfigurierte)
        let taskLists;
        if (CONFIG.USE_ALL_LISTS) {
            taskLists = getAllTaskListIds();
            console.log(`Verwende ALLE ${taskLists.length} Tasklisten`);
        } else {
            taskLists = CONFIG.TASK_LISTS.map(id => ({ id: id, title: id }));
            console.log(`Verwende ${taskLists.length} konfigurierte Listen`);
        }

        let allTasks = [];

        // Tasks aus allen Listen abrufen
        for (const list of taskLists) {
            try {
                const tasks = fetchTasksFromList(list.id, list.title, now, TZ);
                allTasks = allTasks.concat(tasks);
                console.log(`Liste "${list.title}": ${tasks.length} Tasks`);
            } catch (error) {
                console.log(`Fehler bei Liste "${list.title}": ${error.toString()}`);
                throw error;
            }
        }

        // Nach Listenpriorität gruppieren, dann nach Fälligkeit sortieren
        allTasks.sort((a, b) => {
            // PRIMÄR: Nach Listenpriorität (konfigurierte Reihenfolge)
            const priorityA = CONFIG.LIST_PRIORITY.indexOf(a.listName);
            const priorityB = CONFIG.LIST_PRIORITY.indexOf(b.listName);

            // -1 bedeutet "nicht in Liste" -> ans Ende (alphabetisch)
            const effectivePriorityA = priorityA === -1 ? 999 : priorityA;
            const effectivePriorityB = priorityB === -1 ? 999 : priorityB;

            if (effectivePriorityA !== effectivePriorityB) {
                return effectivePriorityA - effectivePriorityB;
            }

            // Falls beide nicht in der Prioritätsliste: alphabetisch
            if (priorityA === -1 && priorityB === -1) {
                const listCompare = a.listName.localeCompare(b.listName);
                if (listCompare !== 0) return listCompare;
            }

            // SEKUNDÄR: Erledigte ans Ende (innerhalb der Liste)
            if (a.isCompleted && !b.isCompleted) return 1;
            if (!a.isCompleted && b.isCompleted) return -1;

            // TERTIÄR: Überfällige zuerst
            if (a.isOverdue && !b.isOverdue) return -1;
            if (!a.isOverdue && b.isOverdue) return 1;

            // Heute fällige vor anderen
            if (a.isDueToday && !b.isDueToday) return -1;
            if (!a.isDueToday && b.isDueToday) return 1;

            // Dann nach Fälligkeitsdatum
            if (a.dueDate && b.dueDate) {
                return new Date(a.dueDate) - new Date(b.dueDate);
            }

            // Tasks ohne Datum am Ende
            if (a.dueDate && !b.dueDate) return -1;
            if (!a.dueDate && b.dueDate) return 1;

            // Alphabetisch nach Titel
            return a.title.localeCompare(b.title);
        });

        // Auf Maximum begrenzen
        const limitedTasks = allTasks.slice(0, CONFIG.MAX_TASKS);

        // Statistiken berechnen
        const stats = {
            total: limitedTasks.length,
            completed: limitedTasks.filter(t => t.isCompleted).length,
            overdue: limitedTasks.filter(t => t.isOverdue && !t.isCompleted).length,
            dueToday: limitedTasks.filter(t => t.isDueToday && !t.isCompleted).length
        };

        const result = {
            tasks: limitedTasks,
            stats: stats,
            today: {
                day: now.getDate(),
                month: now.getMonth() + 1,
                year: now.getFullYear()
            },
            listsUsed: taskLists.length,
            timestamp: now.toISOString()
        };

        console.log(`Response: ${limitedTasks.length} Tasks aus ${taskLists.length} Listen, ${stats.overdue} überfällig`);

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
 * POST-Handler: Task als erledigt markieren (Fallback)
 */
function doPost(e) {
    try {
        const data = JSON.parse(e.postData.contents);
        return handleTaskUpdate(data);
    } catch (error) {
        console.error("Fehler in doPost: " + error.toString());
        return ContentService
        .createTextOutput(JSON.stringify({
            success: false,
            error: error.toString()
        }))
        .setMimeType(ContentService.MimeType.JSON);
    }
}

/**
 * Gemeinsame Funktion für Task-Updates (von GET und POST verwendet)
 * @param {Object} params - Parameter mit taskId, listId, completed
 */
function handleTaskUpdate(params) {
    try {
        const taskId = params.taskId;
        const listId = params.listId || "@default";
        const completed = params.completed === true || params.completed === "true";

        if (!taskId) {
            return ContentService
            .createTextOutput(JSON.stringify({
                success: false,
                error: "taskId erforderlich"
            }))
            .setMimeType(ContentService.MimeType.JSON);
        }

        console.log(`Aktualisiere Task ${taskId} in Liste ${listId}: completed=${completed}`);

        // Task abrufen und aktualisieren
        const task = Tasks.Tasks.get(listId, taskId);

        if (completed) {
            task.status = "completed";
            task.completed = new Date().toISOString();
        } else {
            task.status = "needsAction";
            task.completed = null;
        }

        Tasks.Tasks.update(task, listId, taskId);

        console.log(`Task ${taskId} erfolgreich aktualisiert`);

        return ContentService
        .createTextOutput(JSON.stringify({
            success: true,
            taskId: taskId,
            completed: completed
        }))
        .setMimeType(ContentService.MimeType.JSON);

    } catch (error) {
        console.error("Fehler in handleTaskUpdate: " + error.toString());
        return ContentService
        .createTextOutput(JSON.stringify({
            success: false,
            error: error.toString()
        }))
        .setMimeType(ContentService.MimeType.JSON);
    }
}

/**
 * Ruft Tasks aus einer spezifischen Liste ab
 * @param {string} listId - ID der Taskliste
 * @param {string} listName - Name der Taskliste
 * @param {Date} now - Aktuelles Datum
 * @param {string} TZ - Zeitzone
 * @returns {Array} Array von Task-Objekten
 */
function fetchTasksFromList(listId, listName, now, TZ) {
    const tasks = [];
    const today = new Date(now);
    today.setHours(0, 0, 0, 0);

    // Tasks abrufen (immer mit erledigten Tasks)
    const response = Tasks.Tasks.list(listId, {
        showCompleted: true,
        showHidden: false,
        maxResults: 100
    });

    if (!response.items) {
        return tasks;
    }

    for (const item of response.items) {
        // Optional: Präfix-Filter
        if (CONFIG.TITLE_PREFIXES.length > 0) {
            const hasPrefix = CONFIG.TITLE_PREFIXES.some(prefix =>
            item.title && item.title.startsWith(prefix)
            );
            if (!hasPrefix) continue;
        }

        const task = {
            id: item.id,
            listId: listId,
            listName: listName,
            title: item.title || "(Kein Titel)",
            notes: item.notes || "",
            isCompleted: item.status === "completed",
            dueDate: null,
            dueDateFormatted: "",
            isOverdue: false,
            isDueToday: false,
            daysOverdue: 0,
            priority: 0  // 0=normal, 1=heute, 2=überfällig, 3=stark überfällig
        };

        // Fälligkeitsdatum verarbeiten
        if (item.due) {
            const dueDate = new Date(item.due);
            task.dueDate = item.due;

            // Datum formatieren (DD.MM.YYYY)
            task.dueDateFormatted = Utilities.formatDate(dueDate, TZ, 'dd.MM.yyyy');

            // Prüfen ob heute
            const dueDateOnly = new Date(dueDate);
            dueDateOnly.setHours(0, 0, 0, 0);

            if (dueDateOnly.getTime() === today.getTime()) {
                task.isDueToday = true;
                task.priority = 1;
            }

            // Prüfen ob überfällig (nur wenn nicht erledigt)
            if (!task.isCompleted && dueDateOnly < today) {
                task.isOverdue = true;
                task.daysOverdue = Math.floor((today - dueDateOnly) / (1000 * 60 * 60 * 24));

                if (task.daysOverdue >= CONFIG.DAYS_OVERDUE_HIGHLIGHT) {
                    task.priority = 3;  // Stark überfällig
                } else {
                    task.priority = 2;  // Überfällig
                }
            }
        }

        tasks.push(task);
    }

    return tasks;
}

/**
 * Hilfsfunktion: Listet alle verfügbaren Tasklisten auf
 * Aufruf über Script-Editor: Ausführen -> listTaskLists
 */
function listTaskLists() {
    const lists = Tasks.Tasklists.list();

    console.log("=== Verfügbare Tasklisten ===");

    if (lists.items) {
        for (const list of lists.items) {
            console.log(`Name: "${list.title}" | ID: ${list.id}`);
        }
        console.log(`\nGesamt: ${lists.items.length} Listen`);
    } else {
        console.log("Keine Tasklisten gefunden");
    }

    return lists;
}

/**
 * Test-Funktion für doGet
 */
function testDoGet() {
    const result = doGet({ parameter: {} });
    console.log(result.getContent());
}
