rem Aufruf von Avrdude zur Programmierung c't-Bot

rem (Unten sollte zunaechst der Pfad angepasst werden)

rem Batchdatei fuer Programmieradapter STK200 und viele baugleiche Programmer am Parallelport
rem
rem von andreas Staudenmayer (andreas.staudenmayer@t-online.de)
rem angepasst fuer STK200 (rl@loehmer.de)
rem Parameter
rem
rem -p m32  fuer ATMega32
rem -c stk200  Adapter kompatibel zu stk200 
rem -P lpt1  Adapter installiert auf lpt1
rem -u  disable safemode, um fusebits setzen zu koennen
rem -U flash:w:<Datei>:i Programmdatei, die geschrieben werden soll, wird als erster Batch Parameter uebergeben
rem 
rem -U lfuse:w:lfuse.hex:i lowfusebits setzen, Inhalt Bin-Datei lfuse.hex: FF
rem -U hfuse:w:hfuse.hex:i highfusebits setzen, Inhalt Bin-Datei hfuse.hex: D9
rem -U lock:w:lock.hex:i  lockbits setzen, Inhalt Bin-Datei lock.hex: 3F  
rem -F Signatur des Chips lesen
rem -v geschwaetzige Ausgabe
rem -E reset Reset wenn fertig
rem -e Chip loeschen
  
rem Moeglichkeit 1: 
rem Verknuepfung auf avrdude-Batchdatei auf dem Desktop ablegen, 
rem "Fallenlassen" einer Hex-Datei auf diese Verknuepfung startet 
rem den  Programmierprozess

rem Moeglichkeit 2:
rem Nutzung von ECLIPSE 'External Tools'
rem kurze Anleitung :
rem http://www.ctbot.de/forum/fuse-bits-t108-15.html#2372


rem !!!Anfuehrungsstriche sind wichtig bei Pfaden mit Leerstellen!!!
rem Die naechste Zeile ist schon alles! Pfad fuer AVRDUDE anpassen!

"D:\Programme\WinAVR\bin\avrdude" -p m32 -c stk200 -P lpt1 -U flash:w:%1:i -e -F -v -E reset