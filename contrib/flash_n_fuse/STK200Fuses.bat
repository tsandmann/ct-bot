rem Aufruf von Avrdude zur Programmierung c't-Bot
rem Setzen der Fuses/Locks ist EINMALIG vor der ersten Programmierung NOTWENDIG
rem weiteres hier: http://www.heise.de/ct/ftp/projekte/ct-bot/faq/f.shtml

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
rem -U flash:w:<Datei>:i Programmdatei, die geschrieben werden soll, wird als erster Batch Parameter übergeben
rem 
rem -U lfuse:w:lfuse.hex:i lowfusebits setzen, Inhalt Bin-Datei lfuse.hex: FF
rem -U hfuse:w:hfuse.hex:i highfusebits setzen, Inhalt Bin-Datei hfuse.hex: D9
rem -U lock:w:lock.hex:i  lockbits setzen, Inhalt Bin-Datei lock.hex: 3F    

rem auch hier den Pfad setzen
"D:\Programme\WinAVR\bin\avrdude" -p m32 -c stk200 -P lpt1 -u -U lfuse:w:lfuse.hex:i -U hfuse:w:hfuse.hex:i -U lock:w:lock.hex:i