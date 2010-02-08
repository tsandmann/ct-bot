/*
 * c't-Bot
 * 
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your
 * option) any later version. 
 * This program is distributed in the hope that it will be 
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
 * PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public 
 * License along with this program; if not, write to the Free 
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 * 
 */

/*! 
 * @file 	sp03.c
 * @brief 	Ansteuerung des Sprachmoduls SP03 Text to Speech mit TWI aka I2C.
 * 
 * Weitere Dokumentation unter Documentation/sp03.pdf
 * 
 * Ein sprechender Roboter ist die Wunschvorstellung vieler Roboterkonstrukteure.
 * Das hier vorgestellte SP03 Modul von Devantech Inc. ermoeglicht die direkte Umwandlung
 * von ASCII-Zeichen in Sprache, ein moderner Sprachsynthesizer von Winbond machts moeglich.
 * Das Board verfuegt bereits ueber Verstaerker und Lautsprecher. Die Ansteuerung erfolgt
 * ueber I2C.
 * Zusaetzlich koennen bis zu 30 gespeicherte Saetze auf Knopfdruck abgerufen werden.
 * 
 * So geht's: Die Parameter und der Text werden separat geschickt, gefolgt von einem
 * "Sprich-jetzt!-Befehl" Dies sieht ungefaehr so aus:
 * 
 * --- BEGIN I2C CONTAINER ---			// Stimmenparameter werden gesendet
 *	 # 0xc4 	I2C Adresse
 *	 0 0x0		Kommandoregister
 *	 1 0x0		NOP - Register
 *	 2 0x1 		Volume					//(0-7) 0=am lautesten
 *	 3 0x3 		Pitch					//(0-7) 0=am hoechsten
 *	 4 0x5		Speed					//(0-3) 0=am langsamsten
 * --- END I2C CONTAINER ---
 * 
 * --- BEGIN I2C CONTAINER ---			// Text wird in Buffer geladen
 *	 # 0xc4 	I2C Adresse
 *	 0 0x0		Kommandoregister
 *	 1 0x0		NOP - Register
 *	 2 0x48 H
 *	 3 0x65 e
 *	 4 0x6c l
 *	 5 0x6c l
 *	 6 0x6f o
 *	 7 0x0		NOP - Register
 * --- END I2C CONTAINER ---
 * 
 * --- BEGIN I2C CONTAINER ---			// Sprachausgabe
 *	 # 0xc4 	I2C Adresse
 *	 0 0x0		Kommandoregister
 *   1 0x40		Speak Buffer
 * --- END I2C CONTAINER --- 
 * 
 * 
 * Das Benutzerinterface funktioniert wie folgt:
 * 
 * # include "sp03.h"
 * ...
 * //Mit Stimmenparameter
 * sp03_set_voice(2,3,1);
 * sp03_speakf("I am %d", sensError);
 * 
 * //Kurzfassung, mit Defaultwerten
 * sp03_say("I am %d", sensError);
 * 
 * 
 * 
 * @author 	Harald W. Leschner (hari@h9l.net)
 * @date 	29.03.08
 */

/*! @todo
 *  - Abfragen von STATUS bevor neues Sprechen
 * 	- Ein eigenes Behaviour, LOG_SAY und RemoteCall anlegen ...
 *  - Fehler abfangen und evtl. sprachlich ausgeben
 *  - testen wirklich langer und komplizierter Texte ...
 */

#ifdef MCU 
#include "ct-Bot.h"
#ifdef SP03_AVAILABLE 
#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include "i2c.h"
#include "sp03.h"
#include "delay.h"
#include "log.h"

/*! Puffergroesse fuer einen Satz in Bytes */
#define SP03_BUFFER_SIZE	(SP03_MAX_TEXT_SIZE+3)

/*!
 * @brief			Laden eines Strings in den SP03, der im Flash gespeichert ist.
 * Schreibt die formatierte Ausgabe der zu sprechenden Zeichen in einen Puffer und
 * uebertraegt sie per I2C an das Sprachmodul. Die eigentliche Aussprache des
 * Textes erfolgt im Macro sp03_say() oder sp03_speakf().
 * 
 * @param format 	Format, wie beim printf
 * @param ... 		Variable Argumentenliste, wie beim printf
 */
void sp03_flash_speakf(const char * format, ...) {

	char sp03_buf[SP03_BUFFER_SIZE];		 // Pufferstring fuer Sprachausgabe
	va_list	args;
	uint8_t state = TW_NO_INFO;
	uint8_t length = 0;						// Laenge Text
	
	//Zuerst die Registerbefehle einfuegen
	sp03_buf[0] = SP03_COMMAND_REG; 		// Befehlsregister 
	sp03_buf[1] = SP03_COMMAND_NOP; 		// NOP: Load buffer - Beginn Text 

	//Formatierte Ausgabe der Argumente
	va_start(args, format);
	length = vsnprintf_P(sp03_buf + 2, SP03_MAX_TEXT_SIZE, format, args);	//Kopiert max. 81 Zeichen in sp03_buf
	va_end(args);

	sp03_buf[(length+2)] = SP03_COMMAND_NOP;		// NOP: Load buffer - Ende Text
	
	//Telegramm fertig, ab zum Treiber...
	i2c_write(SP03_TWI_ADDRESS, sp03_buf, (length+3));
	state = i2c_wait();
	//LOG_DEBUG("Status buffer set:%x",state);
}

/*!
 * SP03 Steuercodes fuer Lautstaerke, Speed und Pitch an Synth senden
 * @param sp03_volume Lautstaerke
 * @param sp03_pitch Geschwindigkeit
 * @param sp03_speed Stimmlage
 */
void sp03_set_voice(uint8_t sp03_volume, uint8_t sp03_pitch, uint8_t sp03_speed) {

	uint8_t bufvox[5];
	uint8_t state = TW_NO_INFO;

	bufvox[0] = SP03_COMMAND_REG;
	bufvox[1] = SP03_COMMAND_NOP;
	bufvox[2] = sp03_volume;
	bufvox[3] = sp03_pitch;
	bufvox[4] = sp03_speed;

	//Telegramm fertig, ab zum Treiber...
	i2c_write(SP03_TWI_ADDRESS, bufvox, 5);
	state = i2c_wait();
	//LOG_DEBUG("Status voice set:%x",state);
}

/*!
 * SP03 Steuercode fuer Sprechen senden
 */
void sp03_cmd_speak(void) {

	uint8_t bufspk[2];
	uint8_t state = TW_NO_INFO;

	bufspk[0] = SP03_COMMAND_REG;
	bufspk[1] = SP03_SPEAK_BUFFER;

	//Telegramm fertig, ab zum Treiber...
	i2c_write(SP03_TWI_ADDRESS, bufspk, 2);
	state = i2c_wait();
	//LOG_DEBUG("Status speak:%x",state);
}

/*!
 * SP03 Vordefinierte Saetze abrufen 1-30 oder 0x01-0x1E
 * @param sp03_pre_id Vordefinierte Satz-ID-Nr
 */
void sp03_speak_phrase(uint8_t sp03_pre_id) {
	
	if (sp03_pre_id>SP03_MAXNUMPHRASES) {
		if (sp03_pre_id<1) {
			sp03_pre_id=SP03_DEFAULT_PHRASE;
		}
	}

	uint8_t bufpid[3];
	uint8_t state = TW_NO_INFO;

	bufpid[0] = SP03_COMMAND_REG; //SP03 Befehlsregister
	bufpid[1] = sp03_pre_id; //SP03 Vordefinierter Satz ID
	bufpid[2] = SP03_COMMAND_NOP;

	//Telegramm fertig, ab zum Treiber...
	i2c_write(SP03_TWI_ADDRESS, bufpid, 3);
	state = i2c_wait();
	//LOG_DEBUG("Status preid:%x",state);
}

/*!
 * SP03 Firmwareversion auslesen
 */
void sp03_get_version(void) {
	
	uint8_t version = 0;
	uint8_t state = TW_NO_INFO;

	// Liest Register 0x01 mit Versionsnummer
	i2c_read(SP03_TWI_ADDRESS, SP03_VERSION_REG, &version, 1);

	// Checkung ob alles OK
	state = i2c_wait();
	if (state == 0xf8) {
		LOG_INFO("SP03 Rev:1.%d", version);
		sp03_say("Voice Module Version 1.%d", version);
	} else {
		LOG_INFO("I2C Error:%x", state);
	}
	
}



// VERSION 1.0
#ifdef SP03_V1
/*!
 * OBSOLET: SP03 Direktes Sprechen von ASCII Text max. 81 Zeichen. Hier wird in einer Funktion alles uebergeben.
 * @param sp03_volume Lautstaerke
 * @param sp03_speed Geschwindigkeit
 * @param sp03_pitch Stimmlage
 * @param *sp03_text Der zu sprechende Text
 */
void sp03_speak_string(uint8_t sp03_volume, uint8_t sp03_pitch, uint8_t sp03_speed, char * sp03_text) {
	
	sp03_set_voice(sp03_volume, sp03_pitch, sp03_speed);
	sp03_set_buffer(sp03_text);
	sp03_cmd_speak();
}

/*!
 * OBSOLET: SP03 Text in den Buffer laden, max. 81 ASCII Zeichen
 * @param *sp03_textb Textbuffer
 */
void sp03_set_buffer(const char * sp03_textb) {

	// Stringlaenge der Texteingabe berechnen
	uint8_t len = strlen(sp03_textb);
	//LOG_DEBUG("Length string:%d",len);

	// Falls Text zu lang, dann Defaultspruch
	if ((len>SP03_MAX_TEXT_SIZE) || (len<1)) {
		sp03_textb=SP03_DEFAULT_STRING;
		len=SP03_DEFAULT_LEN;
	}

	// Container fuer TWI Treiber anlegen 
	uint8_t buftxt[(len+3)];
	// Speicherplatz abhaengig von Textlaenge
	uint8_t state = TW_NO_INFO;

	buftxt[0] = SP03_COMMAND_REG; 		// Befehlsregister 
	buftxt[1] = SP03_COMMAND_NOP; 		// NOP: Load buffer 

	// Char-String in einzelne Buchstaben aufloesen 
	uint8_t run = 0;
	while (sp03_textb[run] !='\0') { // Fuer jeden Buchstaben bis Ende einen Eintrag generieren 
		buftxt[(run+2)] = sp03_textb[run]; // '\0' wird nichtmehr durchgefuehrt, 
		//LOG_DEBUG("%d %c",run, sp03_textb[run]);
		++run;
	}
	buftxt[(run+2)] = SP03_COMMAND_NOP;

	//Telegramm fertig, ab zum Treiber...
	i2c_write(SP03_TWI_ADDRESS, buftxt, (len+3));
	state = i2c_wait();
	//LOG_DEBUG("Status buffer:%x",state);
}
#endif	// SP03_V1
#endif	// SP03_AVAILABLE 
#endif	// MCU
