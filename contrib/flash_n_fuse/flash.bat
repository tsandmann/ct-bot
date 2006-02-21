rem Aufruf von Avrdude zur Programmierung c't-Bot
rem Batchdatei für Programmieradapter mysmartUSB
rem
rem von andreas Staudenmayer (andreas.staudenmayer@t-online.de)
rem Parameter
rem
rem -p m32  für ATMega32
rem -c avr910  Adapter kompatibel zu avr910
rem -P com5  Adapter installiert auf com5
rem -u  disable safemode, um fusebits setzen zu können
rem -U flash:w:<Datei>:i Programmdatei, die geschrieben werden soll, wird als erster Batch Parameter übergeben
rem 
rem -U lfuse:w:lfuse.hex:i lowfusebits setzen, Inhalt Bin-Datei lfuse.hex: FF
rem -U hfuse:w:hfuse.hex:i highfusebits setzen, Inhalt Bin-Datei hfuse.hex: D9
rem -U lock:w:lock.hex:i  lockbits setzen, Inhalt Bin-Datei lock.hex: 3F    

rem Verknüpfung auf avrdud-Batchdatei auf dem Desktop ablegen, 
rem "Fallenlassen" einer Hex-Datei auf diese Verknüfpung startet 
rem den  Programmierprozess

avrdude -p m32 -c avr910 -P com5 -U flash:w:%1:i
