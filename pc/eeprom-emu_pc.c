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
 * @file 	eeprom-emu_pc.c  
 * @brief 	Low-Level Routinen fuer den Zugriff auf das emulierte EEPROM des Sim-c't-Bots
 * @author 	Achim Pankalla (achim.pankalla@gmx.de)
 * @date 	07.06.2007
 */

#include "ct-Bot.h"

#ifdef PC

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "log.h"
#include "gui.h"
#include "tcp.h"
#include "timer.h"

#define DEBUG_EEPROM		// Schalter um LOG-Ausgaben anzumachen

#ifndef DEBUG_EEPROM
	#undef LOG_INFO
	#define LOG_INFO(a, ...) {}
#endif

extern uint8 __attribute__ ((section (".s1eeprom"),aligned(1))) _eeprom_start1__;
extern uint8 __attribute__ ((section (".s2eeprom"),aligned(1))) _eeprom_start2__;

/*! Normiert PC EEPROM Adressen*/
#define EEPROM_ADDR(x) ((uint32)x - (uint32)&_eeprom_start2__ - ((uint32)&_eeprom_start2__ - (uint32)&_eeprom_start1__))
/*! Defines zum Mitloggen von EEPROM Zugriffen*/
#define LOG_LOAD 	if(addrconv) {LOG_DEBUG("LOAD: %s : %d", ctab[lastctabi].varname, (uint16)dataline[0] + (uint16)dataline[1]*256);}\
					else {LOG_DEBUG("load-addr=0x%x/%d", address,(uint16)dataline[0] + (uint16)dataline[1]*256);}
#define LOG_STORE 	if(addrconv) {LOG_DEBUG("STORE: %s : %d", ctab[lastctabi].varname, data);}\
					else {LOG_DEBUG("load-addr=0x%x/%d", address, data);}
									
#ifdef __AVR_ATmega644__  // Dieser Prozessor hat 2048 Bytes EEPROM
	#define EE_SIZE 2048
#else
	#define EE_SIZE 1024
#endif

#define EEPROM_FILENAME	"./eeprom.bin" 		/*<! Name und Pfad der EEPROM Datei. Verzeichnis muss existieren. Backslash doppeln!*/
#define MAX_VAR 200  						/*<! Maximale Anszahl von Variablen*/
#define EEMAP_PC  "./eeprom_pc.map"					/*<! Pfad fuer PC-MAP Datei */
#define EEP_PC    "./ct-Bot.eep"					/*<! Pfad fuer PC-EEP Datei */
#ifdef WIN32
	/* Windows */
	#define EEMAP_MCU "../debug-mcu-w32/eeprom_mcu.map"		/*<! Pfade fuer MAP / EEP Dateien */
#else
	/* Linux und OS X */
	#define EEMAP_MCU "../Debug-MCU-Linux/eeprom_mcu.map"	/*<! Pfade fuer MAP / EEP Dateien */
#endif

typedef struct addrtab {
	char varname[20];
	uint32 simaddr;
	uint32 botaddr;
	uint32 size;
	uint32 access;
} AddrCTab_t;								/*<! Spezieller Datentyp fuer Adresskonvertierung */

static AddrCTab_t ctab[MAX_VAR]; 			/*<! Adresskonvertierungstabelle */
static uint16 tsize=0; 						/*<! Anzahl Eintraege in der Tabelle */
static uint8 addrconv = 0;                  /*<! Adresskonvertierung ein-/ausschalten */
static uint16 lastctabi = 0;                /*<! Letzter Zugriffsindex auf ctab */

/*! 
 * Diese Funktion konvertiert die Adressen der EEPROM Variablen des PC so,
 * dass sie den Adressen im realen ct-Bot entsprechen. Dafuer wird mit der Funktion
 * create_ctab ein Adresstabelle angelegt, diese nutzt diese Funktion.
 * @param addr Adresse im EEPROM zwischen 0 und EE_SIZE-1
 * @return Die neue Adresse fuer den realen Bot
 */
static uint32 conv_eeaddr(uint32 addr){
	int8 i;
	int32 adiff;

	if(!addrconv) //Keine Adresskonvertierung
		return(addr);
	for(i=0; i<tsize; i++){
		adiff = addr - ctab[i].simaddr;
		if(adiff < 0)
			return(0xffffffff);
		if(adiff < ctab[i].size){
			lastctabi = i; //Letzer guelten Index merken
			return(ctab[i].botaddr + adiff);
		}
	}
	return(0xffffffff);
}

/*!
 * Diese Funktion uberprueft das vorhanden sein der eeprom.bin und initialisiert sie
 * gegebenenfalls mit der Initwerten der eep Datei.
 * Es wird dabei die Adresskovertierung benutzt, wenn die EEPROM Simulation im MCU-Modus 
 * ist.
 * -----
 *  0 = EEPROM Datei OK
 *  1 = Fehler aufgetreten
 * -----
 * @param initfile EEP-Datei des PC Codes
 * @return Status der Funktion
 */
static uint16 check_eeprom_file(char *initfile, uint8_t eeprom_init){
	FILE *fpr, *fpw; //Filepointer fuer Dateizugriffe
	uint16 i; //Laufvariable
	char data[2]; //Datenspeicher
	
	/*eeprom file vorhanden*/
	if(!(fpw = fopen(EEPROM_FILENAME, "r+b"))){ //Testen, ob Datei vorhanden ist.
		if(!(fpw = fopen(EEPROM_FILENAME, "w+b"))){ //wenn nicht, dann erstellen
			LOG_INFO("->Kann EEPROM-Datei nicht erstellen");
			return(1);
		}
		
		/* EEPROM mit .eeprom-Section des .elf-Files initialisieren */
		uint8_t * ram_dump = (uint8_t *)(&_eeprom_start2__ + (&_eeprom_start2__ - &_eeprom_start1__));
		fwrite(ram_dump, EE_SIZE, 1, fpw);

//		/* alternativ: leeres EEPROM erstellen */	
//		for(i = 0; i < EE_SIZE; i++)
//			fwrite("\377", 1, 1, fpw);

		fclose(fpw);
		LOG_INFO("->Leere EEPROM-Datei erstellt");
 	}
	
	/*Initialsieren der eeprom.bin, wenn gewuenscht*/
	if(eeprom_init){
		if(!(fpr = fopen(initfile, "rb"))){ //Datei oeffnen
			LOG_INFO("->EEP nicht gefunden");
			return(1);
		}
		if(!(fpw = fopen(EEPROM_FILENAME, "rb+"))){ //Datei oeffnen
			LOG_INFO("->EEPROM-Datei kann nicht beschrieben werden");
			return(1);
		}
		
		/*EEP-Datei in EEPROM-Datei kopieren*/
		for(i=0; i<EE_SIZE; i++){
			/*Daten kopieren*/
			if(!fread(data, 1, 1, fpr))
				break;
			if(addrconv){ //EEPROM-Datei im MCU-Modus
				uint32 naddr;//Konvertierte Adresse
				
				naddr = conv_eeaddr((uint32)i);
				if(naddr != 0xffffffff){
					fseek(fpw, (int32)naddr, SEEK_SET);
				}
				else{
					continue; //Nichts schreiben Adresse nicht belegt
				}
			}
			fwrite(data, 1, 1, fpw);
		}
		fclose(fpr);
		fclose(fpw);
		LOG_INFO("->EEPROM-Datei mit Daten init.");
	}
	return(0);
}

/*!
 * Diese Funktion erstellt aus den beiden im Post erstellten MAP Dateien eine Tabelle
 * zum umrechnen der PC-Adressen in die des avr-Compilers. Dadurch kann die EEPROM 
 * Datei in einen zum EEPROM des Bot kompatiblen Format gehalten werden.
 * Wichtige Informationen werden ueber LOG_INFO angezeigt.
 * ----
 * Ergebniscodes der Funktion
 * 0 = Fehlerfrei
 * 1 = Sim Mapfile nicht vorhanden
 * 2 = Bot Mapfile nicht vorhanden
 * 3 = Unterschiedlicher Variablenbestand
 * 4 = Zuviele Variablen
 * 5 = EEPROM voll
 * 6 = Unterschiedliche Variablenanzahl
 * 
 * @param simfile MAP mit den Adressen der EEPROM-Variablen in der PC exe/elf
 * @param botfile MAP mit den Adressen der EEPROM-Variablen im MCU elf
 * @return Statuscode
 */
static uint16 create_ctab(char *simfile, char *botfile){
	FILE *fps, *fpb;
	char sline[250], bline[250]; //Textzeilen aus Dateien
	char d[30], d1[30], d2[30], vname_s[30], vname_b[20]; //Dummy und Variablennamen
	int32 addr_s, addr_b; //Adressen der Variablen
	char saddr[30], ssize[30]; //Stringvarianten fuer die Bot Daten
	uint32 size; //Variablengroesse
	int16 i, ii; //Laufvariablen
	uint16 vc = 0; //Variablenzaehler

	/*Dateien oeffnen*/
	if(!(fps=fopen(simfile,"r"))){
		LOG_INFO("->EEPROM-MAP fuer PC fehlt");
		return(1);
	}
	if(!(fpb=fopen(botfile,"r"))){
		LOG_INFO("->EEPROM-MAP fuer MCU fehlt");		
		return(2);
	}

	/*Anzahl Variablen vergleichen*/
	while(fgets(sline, 249, fps)) vc++;
	while(fgets(sline, 249, fpb)) vc--;
	if(vc){
		fclose(fps);
		fclose(fpb);
		LOG_INFO("->Unterschiedliche Variablenanzahl");
		return(6);
	}
	rewind(fps);
	rewind(fpb);

	/*Tabelle erstellen*/
	while(fgets(sline, 249, fps)){
		rewind(fpb);
		sscanf(&sline[47], "%x %s", &addr_s, vname_s);
		while(fgets(bline, 249, fpb)){
			/*Variablennamen suchen*/
			sscanf(&bline[4], "%s %s %s %s %s %s", d1, d, d, d, d2, vname_b);
			sprintf(saddr, "0x%s", d1);
			sscanf(saddr, "%x", &addr_b);
			sprintf(ssize, "0x%s", &d2[4]);
			sscanf(ssize, "%x", &size);
			if(!strcmp(&vname_s[1], vname_b)){
				/*Daten kopieren*/
				strcpy(ctab[tsize].varname, vname_b);
				ctab[tsize].simaddr = addr_s;
				ctab[tsize].botaddr = addr_b;
				ctab[tsize].size    = size;
				ctab[tsize++].access  = 0;

				/*Fehlerabbrueche*/
				if(tsize == MAX_VAR){
					LOG_INFO("->Mehr als %n Variablen",MAX_VAR);
					return(4);
				}
				if(addr_s > EE_SIZE){
					LOG_INFO("->EEPROM voll");
					return(5);
				}
				
				/*Erfolg markieren*/
				addr_b = -1;
				break;
			}
		}
		if(addr_b > -1){ //Keine passende Variable gefunden
			LOG_INFO("->Unterschiedliche Variablen");
			return(3);
		}
	}

	/*Dateien schliessen*/
	fclose(fps);
	fclose(fpb);
	
	/*Tabelle nach Sim-Adressen sortieren*/
	for(ii=tsize-1; ii > 0; ii--){
		for(i=0; i < ii; i++){
			if(ctab[i].simaddr > ctab[i+1].simaddr){
				long simaddr, botaddr, size;
				char vname[30];
			
				/*Daten umkopieren*/
				strcpy(vname, ctab[i+1].varname);
				simaddr = ctab[i+1].simaddr;
				botaddr = ctab[i+1].botaddr;
				size    = ctab[i+1].size;

				strcpy(ctab[i+1].varname, ctab[i].varname);
				ctab[i+1].simaddr = ctab[i].simaddr;
				ctab[i+1].botaddr = ctab[i].botaddr;
				ctab[i+1].size    = ctab[i].size;

				strcpy(ctab[i].varname, vname);
				ctab[i].simaddr = simaddr;
				ctab[i].botaddr = botaddr;
				ctab[i].size    = size;
			}
		}
	}
	return(0);
}

/*! 
 * Diese Funktion initialisiert die eeprom-emulation. Sie sorgt fuer die Erstellung der
 * eeprom.bin, falls nicht vorhanden und erstellt ueber eine Hilfsfunktion eine Adress-
 * konvertierungstabelle fuer die EEPROM-Adressen, wenn die benoetigten Daten vorliegen.
 * Statusinformationen werden ueber DEBUG_INFO angezeigt.
 * @param init	gibt an, ob das EEPROM mit Hilfer einer eep-Datei initialisiert werden soll (0 nein, 1 ja)
 */
uint8_t init_eeprom_man(uint8_t init) {
	uint16 status; //Status von create_ctab
	uint16 sflag; //Sectionstatus
	
	LOG_INFO("EEPROM-Manager");

	/*Adresskonvertierungstabelle anlegen*/
	if((status=create_ctab(EEMAP_PC, EEMAP_MCU))){
			LOG_INFO("->EEPROM im PC-Modus");
	}
	else {
			LOG_INFO("->EEPROM im MCU-Modus");
			addrconv = 1;
	}
	
	/*Sections ueberpruefen*/
	if((&_eeprom_start2__ - &_eeprom_start1__) > 0 && (&resetsEEPROM - &_eeprom_start2__) >0){
		LOG_INFO("->Sections liegen wohl korrekt");
		LOG_INFO("->Section Abstand 0x%X", (&_eeprom_start2__ - &_eeprom_start1__));
		sflag = 0;
	}
	else{
		LOG_INFO("->Sections liegen falsch");
		sflag = 1;
	}
	
	/*eeprom.bin checken*/ 
	if(sflag || check_eeprom_file(EEP_PC, init)){
		LOG_INFO("EEPROM-Emulation fehlerhaft");
		return 1;
	}
	else{
		LOG_INFO("->EEPROM Groesse %d Bytes", EE_SIZE);
		LOG_INFO("EEPROM-Emulation einsatzbereit");
	}
	return 0;
}

/*! 
 * Traegt die uebergebenen Daten in das simulierte EEPROM ein. Dieses simulierte EEPROM 
 * besteht aus einer Datei. 
 * Es koennen Bytes (uint8 size = 1) und Words (uint16 size = 2) uebergeben werden.
 * @param address Adresse im EEPROM zwischen 0 und EE_SIZE-1
 * @param data Daten die abgelegt werden sollen
 * @param size Groesse der Daten in Byte
 */  
static void store_parameter(uint16 address, uint16 data, uint8 size) {
	FILE  *fpw; 	//Dateizeiger
	uint8 dataline[2];	//Speicher fuer Datenausgabe

//	LOG_STORE
	if(address > (size == 1 ? EE_SIZE-1 : EE_SIZE-2)) //Adresse checken
		return;

	if(!(fpw = fopen(EEPROM_FILENAME, "r+b"))){ //Testen, ob Datei vorhanden ist.
			return;
	}

	//Daten eintragen
	fseek(fpw, address, SEEK_SET); //Schreibzeiger setzen
	dataline[0] = data%256;
	dataline[1] = data/256;
	fwrite(dataline, 1, size, fpw);	//Daten schreiben

	fclose(fpw);
	return;	
}

/*! 
 * Liest die gewuenschten Daten aus eine simulierten EEPROM. Dieses simulierte EEPROM 
 * besteht aus der Datei eeprom.bin. 
 * @param address Adresse im EEPROM zwischen 0 und EE_SIZE-1
 * @param size Groesse der Daten in Byte
 * @return Aus dem EEPROM gelesener Wert
 */  
static uint16 load_parameter(uint16 address, uint8 size) {
	FILE *fpr; 	//Dateizeiger
	uint8 dataline[2] = {0,0};	//String fuer Datenausgabe

	if(address > (size == 1 ? EE_SIZE-1 : EE_SIZE-2)) //Adresse checken
		return((size == 1 ? 0xff : 0xffff));

	if(!(fpr = fopen(EEPROM_FILENAME, "rb"))){ //Datei oeffnen
		return((size == 1 ? 0xff : 0xffff));
	}

	//Daten holen
	fseek(fpr, address, SEEK_SET); //Lesezeiger setzen
	fread(dataline, 1, size, fpr);	//Daten lesen
	fclose(fpr);
//	LOG_LOAD
	return((uint16)dataline[0] + (uint16)dataline[1]*256);	
}

/*! 
 * Laedt ein Byte aus dem EEPROM.
 * @param addr	Adresse im EEPROM zwischen 0 und 1023
 * @return 		Wert der Speicheraddresse im EEPROM
 */ 
uint8_t eeprom_read_byte(const uint8_t * addr) {
	return((uint8)load_parameter((uint16)conv_eeaddr(EEPROM_ADDR(addr)), 1));
}

/*! 
 * Laedt ein Word aus dem EEPROM.
 * @param addr	Adresse im EEPROM zwischen 0 und 1023
 * @return 		Wert der Speicheraddresse im EEPROM
 */
uint16_t eeprom_read_word(const uint16_t * addr) {
	return(load_parameter((uint16)conv_eeaddr(EEPROM_ADDR(addr)), 2));
}

/*!
 * Speichert ein Byte im EEPROM.
 * @param addr	Adresse im EEPROM zwischen 0 und 1023
 * @param value	Das abzulegende Byte
 */   
void eeprom_write_byte(uint8_t * addr, uint8_t value) {
	store_parameter((uint16)conv_eeaddr(EEPROM_ADDR(addr)), (uint16)value, 1);
}

/*!
 * Speichert ein Word im EEPROM.
 * @param addr	Adresse im EEPROM zwischen 0 und 1023
 * @param value	Das abzulegende Word
 */
void eeprom_write_word(uint16_t * addr, uint16_t value) {
	store_parameter((uint16)conv_eeaddr(EEPROM_ADDR(addr)), value, 2);
}

/*! 
 * Kopiert einen Block aus dem EEPROM ins RAM
 * @param pointer_ram		Adresse im RAM
 * @param pointer_eeprom	Adresse im EEPROM
 * @param size				Groesse des Blocks in Byte
 */ 
void eeprom_read_block(void *pointer_ram, const void *pointer_eeprom, size_t size) {
	uint32 i;
	uint8 *ram;
	
	ram = (uint8 *)pointer_ram;
	for(i=0; i< size; i++){
		ram[i]=(uint16)load_parameter((uint16)conv_eeaddr(EEPROM_ADDR(pointer_eeprom)) + i, 1);
	}
}

/*! 
 * Kopiert einen Block vom RAM in das EEPROM
 * @param pointer_ram		Adresse im RAM
 * @param pointer_eeprom	Adresse im EEPROM
 * @param size				Groesse des Blocks in Byte
 */ 
void eeprom_write_block(const void *pointer_ram, void *pointer_eeprom, size_t size) {
	uint32 i;
	uint8 *ram;
	
	ram = (uint8 *)pointer_ram;
	for(i=0; i< size; i++){
		store_parameter((uint16)conv_eeaddr(EEPROM_ADDR(pointer_eeprom)) + i, (uint16)ram[i] ,1);
	}
}

#endif	// PC
