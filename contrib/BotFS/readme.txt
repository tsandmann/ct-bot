c't-Bot BotFS Helper

Das Programm legt auf einer maximal 32 MB grossen FAT16-Partition einer SD-Karte ein BotFS-Image an. 
Usage:
 Linux:   "./ct-Bot-botfshelper ABSOLUTER_PFAD_ZUR_SD-KARTE" also z.B. "./ct-Bot-botfshelper /media/SD"
 Mac:     "./ct-Bot-botfshelper ABSOLUTER_PFAD_ZUR_SD-KARTE" also z.B. "./ct-Bot-botfshelper /Volumes/SD"
 Windows: "ct-Bot-botfshelper ABSOLUTER_PFAD_ZUR_SD-KARTE"   also z.B. "ct-Bot-botfshelper e:\"

Der Sourcecode des Hilfsprogramms ist im ct-Bot Repository unter other/ct-Bot-botfshelper zu finden.

 
Um BotFS auf MCU mit MMC / SD-Karte nutzen zu koennen, muss die Karte einmalig dafuer vorbereitet werden.
Zunaechst legt man auf der Karte eine FAT16-Partition an, die maximal 32 MByte gross ist. Diese muss die erste
Partition auf der Karte sein, weitere Partitionen koennen problemlos folgen. Auf der SD-Karte muss unbedingt
eine MBR-Partitionstabelle verwendet werden, GPT wird nicht unterstuetzt! Um durch Rechenweise und Aufrundungen
des verwendeten Partitionstools nicht die 32 MByte-Grenze zu ueberschreiten, empfiehlt es sich, eine Partition von
30 MByte anzulegen - wichtig ist, dass die Groesse der erzeugten Partition 32 MByte, also 32 * 2^20 Byte, nicht ueberschreitet.
Auf der angelegten Partition muessen unbedingt alle Dateien (auch evtl. Versteckte) geloescht werden, so dass die
gesamte Partitionsgroesse als freier Speicher verfuegbar ist.

Anschliessend ruft man das Hilfsprogramm aus ct-Bot-botfshelper wie folgt auf:
 Linux:   "./ct-Bot-botfshelper ABSOLUTER_PFAD_ZUR_SD-KARTE" also z.B. "./ct-Bot-botfshelper /media/SD"
 Mac:     "./ct-Bot-botfshelper ABSOLUTER_PFAD_ZUR_SD-KARTE" also z.B. "./ct-Bot-botfshelper /Volumes/SD"
 Windows: "ct-Bot-botfshelper ABSOLUTER_PFAD_ZUR_SD-KARTE"   also z.B. "ct-Bot-botfshelper e:\"
 
Es bietet sich an, auf der SD-Karte auch eine zweite Partition (Groesse und Typ beliebig) anzulegen und dort eine Kopie
der Datei botfs.img zu speichern. Moechte man einmal das komplette Dateisystem fuer den Bot leeren oder wurde es durch
einen Fehler beschaedigt, kopiert man einfach dieses Backup zurueck auf die erste Partition und muss die obigen Schritte
nicht wiederholen.
 