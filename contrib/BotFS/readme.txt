Vorbereitung einer SD-Karte fuer die Benutzung mit dem c't-Bot

Um BotFS auf MCU mit MMC / SD-Karte nutzen zu koennen, muss die Karte einmalig dafuer vorbereitet werden.
Es gibt drei moegliche Varianten:

Variante 1: Vorgefertigtes Disk-Image auf die SD-Karte uebertragen
 + SD-Karte braucht nicht manuell partioniert werden
 + Nur ein Arbeitsschritt noetig
 
 - Loescht die komplette SD-Karte
 - unter Windows externes Tool noetig


Variante 2: Manuelle Partionierung, anschliessende Einrichtung mit Hilfsprogramm
 + volle Kontrolle ueber die Partitionierung
 + Partions- und Imagegroesse anpassbar (max. 32 MB)
 
 - mehrere Arbeitsschritte noetig
 - Partionierung unter Windows evtl. umstaendlich
 - Wert von first_block in pc/botfs-low_pc.c muss angepasst werden, wenn man BotFS-Dateien am PC erzeugen und auf dem echten Bot verwenden moechte


Variante 3: vollstaendig manuelle Einrichtung
 + volle Kontrolle ueber alle Schritte
 
 - recht umstaendlich
 - Wert von first_block in pc/botfs-low_pc.c muss angepasst werden, wenn man BotFS-Dateien am PC erzeugen und auf dem echten Bot verwenden moechte
 
 
=================================================


Durchfuehrung Variante 1:

Uebertragen des vorgefertigten Disk-Images:
unter Linux:
 1. Device-Node des Kartenlesers ausfindig machen, z.B /dev/sdb wie im Folgenden
 2. Evtl. vorhandene Partitionen auf der SD-Karten unmounten
 3. sudo gunzip -c sd.img.zip | dd of=/dev/sdb bs=4k
    (Achtung, /dev/sdb entsprechend anpassen!)
 4. sync

unter Mac OS X:
 1. Disk-Nummer des Kartenlesers ausfinden machen (Festplatten-Dienstprogramm -> Info oder diskutil list), im Folgenden Beispiel disk3
 2. sudo diskutil unmountDisk /dev/rdisk3
 3. sudo gunzip -c sd.img.zip | dd of=/dev/rdisk3 bs=4k
    (Achtung, /dev/rdisk3 entsprechend anpassen!)
 4. sync

unter Windows:
 0. physdiskwrite + PhysGUI von http://m0n0.ch/wall/physdiskwrite.php herunterladen und entpacken
 1. sd.img.zip entpacken, erzeugt die Datei sd.img
 2. PhysGUI.exe (als Administrator) starten
 3. Rechtsklick auf den Eintrag des Kartenlesers -> Image laden -> Öffnen -> sd.img auswählen -> OK -> Ja
 4. Hardawre sicher entfernen -> auswerfen ausführen

-------------------------------------------------


Durchfuehrung Variante 2:

Einrichtung mit Hilfsprogramm BotFS Helper.
Das Programm legt auf einer maximal 32 MB grossen FAT16-Partition einer SD-Karte ein BotFS-Image an.
Der Sourcecode des Hilfsprogramms ist im ct-Bot Repository unter other/ct-Bot-botfshelper zu finden.
 
Zunaechst legt man auf der Karte eine FAT16-Partition an, die maximal 32 MByte gross ist. Diese muss die erste
Partition auf der Karte sein, weitere Partitionen koennen problemlos folgen. Auf der SD-Karte muss unbedingt
eine MBR-Partitionstabelle verwendet werden, GPT wird nicht unterstuetzt! Um durch Rechenweise und Aufrundungen
des verwendeten Partitionstools nicht die 32 MByte-Grenze zu ueberschreiten, empfiehlt es sich, eine Partition von
30 MByte anzulegen - wichtig ist, dass die Groesse der erzeugten Partition 32 MByte, also 32 * 2^20 Byte, nicht ueberschreitet.
Auf der angelegten Partition muessen unbedingt alle Dateien (auch evtl. Versteckte) geloescht werden, so dass die
gesamte Partitionsgroesse als freier Speicher verfuegbar ist.

Anschliessend ruft man das Hilfsprogramm ct-Bot-botfshelper wie folgt auf:
 Linux:   "./ct-Bot-botfshelper ABSOLUTER_PFAD_ZUR_SD-KARTE" also z.B. "./ct-Bot-botfshelper /media/SD"
 Mac:     "./ct-Bot-botfshelper ABSOLUTER_PFAD_ZUR_SD-KARTE" also z.B. "./ct-Bot-botfshelper /Volumes/SD"
 Windows: "ct-Bot-botfshelper.exe ABSOLUTER_PFAD_ZUR_SD-KARTE" also z.B. "ct-Bot-botfshelper.exe e:\"
 
Es bietet sich an, auf der SD-Karte auch eine zweite Partition (Groesse und Typ beliebig) anzulegen und dort eine Kopie
der Datei botfs.img zu speichern. Moechte man einmal das komplette Dateisystem fuer den Bot leeren oder wurde es durch
einen Fehler beschaedigt, kopiert man einfach dieses Backup zurueck auf die erste Partition und muss die obigen Schritte
nicht wiederholen.

Wenn man BotFS-Dateien am PC erzeugen und auf dem echten Bot verwenden moechte, sollte man den Wert von first_block in
pc/botfs-low_pc.c anpassen, damit das Alignment von Dateien (zur Performanzsteigerung z.B. fuer die Map) stimmt.
first_block muss dazu auf den ersten Datensektor der FAT16-Partition gesetzt werden.


-------------------------------------------------


Durchfuehrung Variante 3:

Anlegen der Partition wie unter Variante 2.
Anschliessend ermittelt man die exakte Groesse der Partition (je nach Betriebssystem findet sich diese in den
Eigenschaften / Informationen des Laufwerks) in Byte und notiert sie. Auf der angelegten Partition muessen unbedingt
alle Dateien (auch evtl. Versteckte) geloescht werden, so dass die gesamte Partitionsgroesse als freier Speicher
verfuegbar ist. Die notierte Groesse teil man noch durch 1024 und erhaelt so die gewuenschte Image-Groesse in KByte.
Nun startet man den fuer PC (mit BOT_FS_AVAILABLE) compilierten Bot-Code mit dem Parameter "-f", also "ct-Bot(.exe) -f",
um die BotFS-Verwaltung aufzurufen. Dort gibt man "create volume" ein und bestaetigt das Kommando mit Enter. Als
Dateinamen waehlt man anschliessend einen Namen wie "botfs.img", eine solche Datei darf aber noch nicht existieren.
Eine komplette Pfadangabe ist auch moeglich, ansonsten wird die Datei im aktuellen Arbeitsverzeichnis erstellt.
Als Volume-Name gibt man dann einen beliebigen ein, wie z.B. "BotFS-Volume", als Groesse danach die eben Ermittelte (in
KByte). Jetzt kann die Verwaltung mit dem Kommando 'q' beendet werden und die erzeugte Datei als "botfs.img" (wichtig -
auf der SD-Karte muss die Datei unbedingt "botfs.img" heissen!) auf die SD-Karte (erste Partition) kopiert werden.

Wenn man BotFS-Dateien am PC erzeugen und auf dem echten Bot verwenden moechte, sollte man den Wert von first_block in
pc/botfs-low_pc.c anpassen, damit das Alignment von Dateien (zur Performanzsteigerung z.B. fuer die Map) stimmt.
first_block muss dazu auf den ersten Datensektor der FAT16-Partition gesetzt werden.

