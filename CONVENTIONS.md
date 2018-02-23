# Richtlinien für Erweiterungen am c't-Bot und c't-Sim (coding conventions)

Fremden Code lesen kann ganz schön anstrengend sein - ein Variablenname, der dem einen selbsterklärend vorkommt, kann auf den anderen kryptisch wirken. Wenn du eine Erweiterung schreibst und deinen Code möglichst verständlich programmierst und dokumentierst, steigen die Chancen, dass andere c't-Bot-Fans von deiner Arbeit ebenfalls profitieren. Als Hilfe haben wir ein paar Richtlinien zusammengestellt, die für Übersicht im c't-Roboter-Projekt sorgen sollen.

Zugegeben: Bislang erfüllt die offizielle Codebasis diese Richtlinie ebenfalls (noch) nicht bis ins letzte Detail - wir arbeiten aber daran das zu ändern. Deshalb kommen uns Beiträge von deiner Seite sehr entgegen, welche die aufgestellten Regeln einhalten.

Damit Pull-Requests für den Code möglichst kompatibel zueinander sind und überschaubar bleiben, sollten sie schlank sein. Faustregel: Ein Request pro Thema. Insbesondere Formatierungsänderungen usw. blähen den Request stark auf und machen ihn im Zweifelsfall inkompatibel zu anderen.

## Allgemeine Richtlinien:

* Namen von Variablen, Funktionen und alles, was direkt mit dem Quellcode zu tun hat, sollten rein englisch sein.
* Kommentare und Dokumentation können auch auf Deutsch verfasst werden.
* Variablen, die in c't-Bot und c't-Sim die gleichen Werte haben, sollten auch die gleichen Namen bekommen. Das macht vieles einfacher und vermeidet Verwechselungen.
* Alle Funktionen und globalen Variablen sollen vollständig kommentiert werden. Hierzu gehört eine kurze Beschreibung, **was** die Funktion tut (aber nicht zwingend, **wie** sie das tut), eine Erläuterung von Übergabeparametern und Rückgabewerten.
* Die Kommentare zu Funktionen und Variablen sollen Doxygen-konform gestaltet werden - auf diese Weise kann man aus dem Code bequem durchsuchbare HTML-Seiten generieren.
* Bei allen Änderungen bitte Einträge in die jeweilige Changelog-Datei nicht vergessen. Hier sollte neben dem Datum der Änderung, Namen und E-Mail-Adresse noch ein kurzer Text stehen, der darstellt, inwiefern die Erweiterung den Roboter oder den Simulator verbessert.
* Der komplette Code - und damit auch jede Erweiterung - steht unter der GPL. Daher müssen neu hinzugefügte Dateien auch einen GPL-Header besitzen.

### c't-Bot:
* Bitte benutze im Code und in dessen Dokumentation und Kommentierung **keine Umlaute**, **kein Eszett** und ähnliches, da diese Zeichen von Windows, Linux, Mac, Solaris, usw. unterschiedlich gehandhabt werden - sie tauchen sonst bald nur noch als Zeichensalat im Code auf oder führen zu Übersetzungsfehlern.

* Bitte verwende **nicht** den Datentyp *int* - PC und MCU interpretieren diesen verschieden! Stattdessen stehen die Typen *int8_t*, *uint8_t*, *int16_t*, *uint16_t*, *int32_t*, *uint32_t* zur Verfügung. Hierbei steht die angehängte Zahl für die Bits, die zum Speichern des Integer-Werts verwendet werden, das vorangestellte 'u' steht für 'unsigned', diese Typen speichern nur positive Werte. Siehe auch http://en.cppreference.com/w/c/types/integer für weitere Informationen. Bitte prüfe sorgfältig, in welchem Bereich die Werte Ihrer Variable liegen werden und wähle den passenden Datentyp.
Die Wertebereiche liegen im Einzelnen bei:
  * int8_t zwischen -128 und 127
  * uint8_t zwischen 0 und 255
  * int16_t zwischen -32.768 und 32.767
  * uint16_t zwischen 0 und 65.535
  * int32_t zwischen -2.147.483.648 und 2.147.483.647
  * uint32_t zwischen 0 und 4.294.967.295

* Den Datentyp char solltest du ebenfalls nur verwenden, wenn es sich wirklich um ein Zeichen handelt. Ansonsten verwende bitte ebenfalls uint8_t oder int8_t.

* Bei Programmieren von Verhaltensweisen für das c't-Bot-Framework gilt folgende Konvention:
  * Verhaltensweisen (Behaviours), die vom Framework bearbeitet werden, heißen bot_*xxx*_behaviour.
  * Die Botenfunktionen, die solche Verhaltensweisen aufrufen und aktivieren, heißen bot_*xxx*.
  * Beispiel: Die Botenfunktion <code>bot_drive_distance()</code> ruft die Verhaltensweise <code>bot_drive_distance_behaviour()</code> auf.
