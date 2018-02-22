Richtlinien fuer Erweiterungen am c't-Bot und c't-Sim (coding conventions)

Fremden Code lesen kann ganz schoen anstrengend sein - ein Variablenname, der dem einen selbsterklaerend vorkommt, kann auf den anderen kryptisch wirken. Wenn du einen Patch schreibst und deinen Code moeglichst verstaendlich programmierst und dokumentierst, steigen die Chancen, dass andere c't-Bot-Fans von deiner Arbeit ebenfalls profitieren. Als Hilfe haben wir ein paar Richtlinien zusammengestellt, die fuer Uebersicht im c't-Roboter-Projekt sorgen sollen.

Zugegeben: Bislang erfuellt die offizielle Codebasis diese Richtlinie ebenfalls (noch) nicht bis ins letzte Detail - wir arbeiten aber daran das zu aendern. Deshalb kommen uns Patches von deiner Seite sehr entgegen, welche die aufgestellten Regeln einhalten.

Damit Patches fuer den Code moeglichst kompatibel zueinander sind und ueberschaubar bleiben, sollten sie schlank sein. Faustregel: Ein Patch pro Thema. Insbesondere Formatierungsaenderungen usw. blaehen den Patch stark auf und machen ihn im Zweifelsfall inkompatibel zu anderen.

Allgemeine Richtlinien:

* Namen von Variablen, Konstanten, Funktionen und alles, was direkt mit dem Quellcode zu tun hat, sollten rein englisch sein.
* Kommentare und Dokumentation sollen dagegen auf Deutsch verfasst werden.
* Variablen, die in c't-Bot und c't-Sim die gleichen Werte haben, sollten auch die gleichen Namen bekommen. Das macht vieles einfacher und vermeidet Verwechselungen.
* Alle Funktionen und Methoden sollen vollstaendig kommentiert werden. Hierzu gehoert eine kurze Beschreibung, WAS die Funktion tut (aber nicht zwingend, WIE sie das tut), eine Erlaeuterung von Uebergabeparametern und Rueckgabewerten sowie der im Java-Teil unter Umstaenden geworfenen Exceptions. Bei trivialen Methoden wie Konstruktoren oder get()- und set()-Methoden kann die Kurzbeschreibung oft auch weggelassen werden.
* Die Kommentare zu Klassen, Funktionen und Methoden sollen Doxygen-beziehungsweise Javadoc-konform gestaltet werden - auf diese Weise kann man aus dem Code bequem durchsuchbare HTML-Seiten generieren.
* Bei allen Aenderungen bitte Eintraege in die jeweilige Changelog-Datei nicht vergessen. Hier sollte neben dem Datum der Aenderung, Ihrem Namen und der E-Mail-Adresse noch ein kurzer Text stehen, der darstellt, inwiefern Ihre Erweiterung den Roboter oder den Simulator verbessert. Wird der Patch (vorerst) nicht ins offizielle Release uebernommen, sondern separat auf der Projektseite veroeffentlicht, sollte dieser Text auch als Beschreibung des Patches tauglich sein.
* Der komplette Code - und damit auch jede Erweiterung - steht unter der GPL. Daher muessen neu hinzugefuegte Dateien auch einen GPL-Header besitzen.

c't-Bot:
> Bitte benutze in der Dokumentation und Kommentierung des Codes keine Umlaute und kein Eszett, da diese Zeichen von Windows, Linux, Mac, Solaris, usw. unterschiedlich gehandhabt werden - sie tauchen bald nur noch als Zeichensalat im Code auf.

Bitte verwende NICHT den Datentyp int - PC und MCU interpretieren diesen verschieden! Stattdessen stehen die Typen int8_t, unint8_t, int16_t, uint16_t zur Verfuegung. Hierbei steht die angehaengte Zahl fuer die Bits, die zum Speichern des Integer-Werts verwendet werden, das vorangestellte 'u' steht fuer 'unsigned', diese Typen speichern nur positive Werte. Bitte pruefe sorgfaeltig, in welchem Bereich die Werte Ihrer Variable liegen werden und waehle den passenden Datentyp.

Die Wertebereiche liegen im Einzelnen bei:

* int8_t	zwischen	-128	und	127
* uint8_t	zwischen	0	und	255
* int16_t	zwischen	-32.768	und	32.767
* uint16_t	zwischen	0	und	65.535

Den Datentyp char solltest du ebenfalls nur verwenden, wenn es sich wirklich um ein Zeichen handelt. Ansonsten verwende bitte ebenfalls uint8_t oder int8_t.

Bei Programmieren von Verhaltensweisen fuer das c't-Bot-Framework gilt folgende Konvention:

* Verhaltensweisen (Behaviours), die vom Framework bearbeitet werden, heissen bot_xxx_behaviour.
* Die Botenfunktionen, die solche Verhaltensweisen aufrufen und aktivieren, heissen bot_xxx.

  Beispiel:
  
  Die Botenfunktion bot_drive_distance() ruft die Verhaltensweise bot_drive_distance_behaviour() auf.