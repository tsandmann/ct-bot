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

//	Post-Build AVR:
//	avr-objcopy -O ihex -R .eeprom ct-Bot.elf ct-Bot.hex; avr-objcopy -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O ihex ct-Bot.elf ct-Bot.eep; avr-size ct-Bot.elf; avr-objdump -t ct-Bot.elf | grep "O \.eeprom" >  eeprom_mcu.map

//	Post-Build Linux:
//	objcopy -j .eeprom --change-section-lma .eeprom=0 -O binary ct-Bot.elf ct-Bot.eep; objdump -t -j .eeprom -C ct-Bot.elf | grep "g" > eeprom_pc.map

//	Post-Build Mac OS X: 
//	objcopy -j LC_SEGMENT.__DATA..eeprom --change-section-lma LC_SEGMENT.__DATA..eeprom=0 -O binary ct-Bot ct-Bot.eep; objdump -t -j LC_SEGMENT.__DATA..s2eeprom -C ct-Bot | grep "g" > eeprom_pc.map

//	Post-Build Windows:
//	objcopy -j .eeprom --change-section-lma .eeprom=0 -O binary ct-Bot.exe ct-Bot.eep;objdump -t ct-Bot.exe | grep "(sec  5)" | grep "(nx 0)" > eeprom_pc.map


#include "ct-Bot.h"

#ifdef PC

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "log.h"
#include "gui.h"

#define DEBUG_EEPROM		// Schalter um LOG-Ausgaben anzumachen

#ifndef DEBUG_EEPROM
	#undef LOG_INFO
	#define LOG_INFO(a, ...) {}
#endif

extern uint8 __attribute__ ((section (".s1eeprom"),aligned(1))) _eeprom_start1__;
extern uint8 __attribute__ ((section (".s2eeprom"),aligned(1))) _eeprom_start2__;

/*! Normiert PC EEPROM Adressen*/
#define EEPROM_ADDR(x) ((uint32)x - (uint32)&_eeprom_start2__ - ((uint32)&_eeprom_start2__ - (uint32)&_eeprom_start1__))
/*! Makros zum Mitloggen von EEPROM Zugriffen*/
#define LOG_LOAD 	if(addrconv) {LOG_DEBUG("LOAD:%s : %u", ctab[lastctabi].varname, eeprom[address]);}\
					else {LOG_DEBUG("load-addr=0x%x/%u", address, eeprom[address]);}
#define LOG_STORE 	if(addrconv) {LOG_DEBUG("STORE:%s : %d", ctab[lastctabi].varname, value);}\
					else {LOG_DEBUG("load-addr=0x%x/%d", address, value);}

/*! Positionen der Daten in den Map-Dateien */
#ifdef WIN32
	#define SADDR_POS 54	/*!< Pos. der Adresse in PC Map Datei */
	#define SNAME_POS 60	/*!< Pos. des Variablennamen ohne _ */
	#define BADDR_POS  0	/*!< Pos. der Adresse in MCU Map Datei */
	#define BNAME_POS 34	/*!< Pos. des Variablennamen in MCU Map */
	#define BSIZE_POS 28	/*!< Pos. der Variablengroesse in MCU Map */
#endif					
#ifdef __linux__
	#define SADDR_POS  0	/*!< Pos. der Adresse in PC Map Datei */
	#define SNAME_POS 46	/*!< Pos. des Variablennamen ohne _ */
	#define BADDR_POS  0	/*!< Pos. der Adresse in MCU Map Datei */
	#define BNAME_POS 34	/*!< Pos. des Variablennamen in MCU Map */
	#define BSIZE_POS 28	/*!< Pos. der Variablengroesse in MCU Map */
#endif					
#ifdef __APPLE__
	#define SADDR_POS  0	/*!< Pos. der Adresse in PC Map Datei */
	#define SNAME_POS 45	/*!< Pos. des Variablennamen ohne _ */
	#define BADDR_POS  0	/*!< Pos. der Adresse in MCU Map Datei */
	#define BNAME_POS 34	/*!< Pos. des Variablennamen in MCU Map */
	#define BSIZE_POS 28	/*!< Pos. der Variablengroesse in MCU Map */
#endif					
				
#ifdef __AVR_ATmega644__  // Dieser Prozessor hat 2048 Bytes EEPROM
	#define EE_SIZE 2048
#else
	#define EE_SIZE 1024
#endif

#define MCU_EEPROM_FN	"./mcu_eeprom.bin" 		/*!< Name und Pfad der EEPROM Datei fuer MCU-Modus*/
#define PC_EEPROM_FN	"./pc_eeprom.bin" 		/*!< Name und Pfad der EEPROM Datei fuer PC-Modus*/
#define MAX_VAR 200  						/*!< Maximale Anzahl von Variablen*/
#define EEMAP_PC  "./eeprom_pc.map"			/*!< Pfad fuer PC-MAP Datei */
#define EEP_PC    "./ct-Bot.eep"			/*!< Pfad fuer PC-EEP Datei */
#ifdef WIN32
	/* Windows */
	#define EEMAP_MCU "../debug-mcu-w32/eeprom_mcu.map"		/*!< Pfad fuer MCU MAP Datei */
#else
	/* Linux und OS X */
	#define EEMAP_MCU "../Debug-MCU-Linux/eeprom_mcu.map"	/*!< Pfad fuer MCU MAP Datei */
#endif

typedef struct addrtab {
	char varname[30];
	size_t simaddr;
	size_t botaddr;
	uint16_t size;
} AddrCTab_t;								/*!< Spezieller Datentyp fuer Adresskonvertierung */

static AddrCTab_t ctab[MAX_VAR]; 			/*!< Adresskonvertierungstabelle */
static uint16 tsize=0; 						/*!< Anzahl Eintraege in der Tabelle */
static uint16 esize=0;						/*!< Summe der EEPROM Variablen */
static uint8 addrconv = 0;                  /*!< Adresskonvertierung ein-/ausschalten */
static uint16 lastctabi = 0;                /*!< Letzter Zugriffsindex auf ctab */
static uint8 eeprom[EE_SIZE];               /*!< EEPROM Speicher im RAM */
static FILE * ee_file;						/*!< Zeiger auf EEPROM-Datei */

/*! 
 * Diese Funktion konvertiert die Adressen der EEPROM Variablen des PC so,
 * dass sie den Adressen im realen ct-Bot entsprechen. Dafuer wird mit der Funktion
 * create_ctab ein Adresstabelle angelegt, diese nutzt diese Funktion.
 * @param addr	Adresse im EEPROM zwischen 0 und EE_SIZE-1
 * @return 		Die neue Adresse fuer den realen Bot
 */
static uint32 conv_eeaddr(uint32 addr){
	int8 i;
	int32 adiff;

	if(!addrconv) //Keine Adresskonvertierung
		return(addr);
	for(i=0; i<tsize; i++){
		adiff = addr - ctab[i].simaddr;
		if(adiff < 0)
			return(0xfffffffe);
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
 * @param initfile		EEP-Datei des PC Codes
 * @param eeprom_init	Flag fuer Initialisierung (1 ja, 0 nein)
 * @param fn			Dateiname der EEPROM-Datei
 * @return 				Status der Funktion
 */
static uint16 check_eeprom_file(char *initfile, uint8_t eeprom_init, char *fn){
	FILE *fpr, *fpw; //Filepointer fuer Dateizugriffe
	uint16 i; //Laufvariable
	char data[2]; //Datenspeicher
	
	/*eeprom file vorhanden*/
	if(!(fpw = fopen(fn, "r+b"))){ //Testen, ob Datei vorhanden ist.
		if(!(fpw = fopen(fn, "w+b"))){ //wenn nicht, dann erstellen
			LOG_INFO("->Kann EEPROM-Datei nicht erstellen");
			return(1);
		}
		if(!addrconv) {
			/* EEPROM mit .eeprom-Section des .elf-Files initialisieren, wenn PC Modus */
			uint8_t * ram_dump = (uint8_t *)(&_eeprom_start2__ + (&_eeprom_start2__ - &_eeprom_start1__));
			fwrite(ram_dump, EE_SIZE, 1, fpw);
			LOG_INFO("->Initialsierte EEPROM-Datei erstellt");
		} else {
			/* alternativ: leeres EEPROM erstellen und init setzen bei MCU Modus*/	
			for(i = 0; i < EE_SIZE; i++)
				fwrite("\377", 1, 1, fpw);
			eeprom_init = 1;
			LOG_INFO("->Leere EEPROM-Datei erstellt");
		}

		fclose(fpw);
 	}
	
	/*Initialsieren der eeprom.bin, wenn gewuenscht*/
	if(eeprom_init){
		if(!(fpr = fopen(initfile, "rb"))){ //Datei oeffnen
			LOG_INFO("->EEP nicht gefunden");
			return(1);
		}
		if(!(fpw = fopen(fn, "rb+"))){ //Datei oeffnen
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
				if(naddr < 0xfffffffe){
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
 * @param simfile	MAP mit den Adressen der EEPROM-Variablen in der PC exe/elf
 * @param botfile	MAP mit den Adressen der EEPROM-Variablen im MCU elf
 * @return 			Statuscode
 */
static uint16 create_ctab(char *simfile, char *botfile){
	FILE *fps, *fpb;
	char sline[250], bline[250]; //Textzeilen aus Dateien
	char vname_s[30], vname_b[30]; //Variablennamen
	size_t addr_s, addr_b; //Adressen der Variablen
	uint16_t size; //Variablengroesse
	uint16 vc = 0; //Variablenzaehler
	size_t first_botaddr = 0xffffffff; //Erste ct-bot EEPROM Adressse

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
		sscanf(&sline[SNAME_POS], "%s", vname_s);
//		printf("vname_s=%s\n", vname_s);
		addr_s = strtol(&sline[SADDR_POS], NULL, 16);
//		printf("addr_s=0x%lx\n", addr_s);
		while(fgets(bline, 249, fpb)){
			/*Variablennamen suchen*/
			sscanf(&bline[BNAME_POS], "%s", vname_b);
//			printf("vname_b=%s\n", vname_b);
			addr_b = strtol(&bline[BADDR_POS], NULL, 16);
//			printf("addr_b=0x%lx\n", addr_b);
			if(addr_b < first_botaddr)//Kleinste ct-bot Adresse bestimmen
				first_botaddr = addr_b; 
			size = strtol(&bline[BSIZE_POS], NULL, 16);
//			printf("size=0x%x\n", size);
			
			if(!strcmp(vname_s, vname_b)){
				/*Daten kopieren*/
				strcpy(ctab[tsize].varname, vname_b);
				ctab[tsize].simaddr = addr_s;
				ctab[tsize].botaddr = addr_b;
				ctab[tsize++].size    = size;
				//ctab[tsize++].access  = 0;	// ??? wird nie ausgewertet
				esize += size;
				/*Fehlerabbrueche*/
				if(tsize == MAX_VAR){
					LOG_INFO("->Mehr als %n Variablen",MAX_VAR);
					return(4);
				}
				if(esize > EE_SIZE){
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
	uint16_t i, j;
	for(j=tsize-1; j > 0; j--){
		for(i=0; i < j; i++){
			if(ctab[i].simaddr > ctab[i+1].simaddr){
				size_t simaddr, botaddr, size;
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
	
	/*Die EEPROM-Startadressen vom ct-bot und/oder Ct-Sim auf Null normieren*/
	size_t first_simaddr = ctab[0].simaddr; 
	if(first_simaddr || first_botaddr) {
		LOG_INFO("->Adressen werden normiert");
		for(i=0; i<tsize; i++){
			if(first_simaddr) ctab[i].simaddr -= first_simaddr;
			if(first_botaddr) ctab[i].botaddr -= first_botaddr;
//			printf("addr_b=0x%x addr_b=0x%x\n", ctab[i].botaddr, ctab[i].simaddr);
		}
	}

//	for (i=0; i<tsize; i++) {
//		printf("i=%u\n", i);
//		printf("varname=%s\n", ctab[i].varname);
//		printf("simaddr=0x%lx\n", ctab[i].simaddr);
//		printf("botaddr=0x%lx\n", ctab[i].botaddr);
//		printf("size=0x%x\n\n", ctab[i].size);		
//	}	
	return(0);
}

/*! 
 * Diese Funktion initialisiert die eeprom-emulation. Sie sorgt fuer die Erstellung der
 * eeprom.bin, falls nicht vorhanden und erstellt ueber eine Hilfsfunktion eine Adress-
 * konvertierungstabelle fuer die EEPROM-Adressen, wenn die benoetigten Daten vorliegen.
 * Statusinformationen werden ueber DEBUG_INFO angezeigt.
 * @param init	gibt an, ob das EEPROM mit Hilfer einer eep-Datei initialisiert werden soll (0 nein, 1 ja)
 * @return		0: alles ok, 1: Fehler
 */
uint8_t init_eeprom_man(uint8_t init) {
	uint16 status;	//Status von create_ctab
	uint16 sflag;	//Sectionstatus
	char fn[30] = "";
	
	LOG_INFO("EEPROM-Manager");

	/*Adresskonvertierungstabelle anlegen*/
	if((status=create_ctab(EEMAP_PC, EEMAP_MCU))){
			LOG_INFO("->EEPROM im PC-Modus");
			strcat(fn, PC_EEPROM_FN);
			remove(MCU_EEPROM_FN);
	}
	else {
			LOG_INFO("->EEPROM im MCU-Modus");
			strcat(fn, MCU_EEPROM_FN);
			remove(PC_EEPROM_FN);
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
	if(sflag || check_eeprom_file(EEP_PC, init, fn)){
		LOG_INFO("EEPROM-Emulation fehlerhaft");
		return 1;
	}
	else{
		LOG_INFO("->EEPROM Groesse: %d Bytes", EE_SIZE);
		if(addrconv)
			LOG_INFO("->Belegter EEPROM Speicher: %d Bytes", esize);
		LOG_INFO("EEPROM-Emulation einsatzbereit");
	}
	
	if((ee_file = fopen(fn, "r+b")) == NULL) return 1;
	if (fread(eeprom, 1, EE_SIZE, ee_file) != EE_SIZE) return 1;
	return 0;
}

/*!
 * Schreibt den kompletten Inhalt des EEPROM-Caches in die Datei zurueck
 */
static inline void flush_eeprom_cache(void) {
	fseek(ee_file, 0, SEEK_SET);
	fwrite(eeprom, 1, EE_SIZE, ee_file);
	fflush(ee_file);
}

/*! 
 * Laedt ein Byte aus dem EEPROM.
 * @param addr	Adresse im EEPROM zwischen 0 und 1023
 * @return 		Wert der Speicheraddresse im EEPROM
 */ 
uint8_t eeprom_read_byte(const uint8_t * addr) {
	uint16_t address = conv_eeaddr(EEPROM_ADDR(addr));
	if (address >= EE_SIZE) return 0xff;
	return eeprom[address];
}

/*! 
 * Laedt ein Word aus dem EEPROM.
 * @param addr	Adresse im EEPROM zwischen 0 und 1023
 * @return 		Wert der Speicheraddresse im EEPROM
 */
uint16_t eeprom_read_word(const uint16_t * addr) {
	uint16_t address = conv_eeaddr(EEPROM_ADDR(addr));
	if (address >= EE_SIZE-1) return 0xffff;
	uint8_t * ptr = eeprom + address;
	return *(uint16_t *)ptr;
}

/*! 
 * Kopiert einen Block aus dem EEPROM ins RAM
 * @param pointer_ram		Adresse im RAM
 * @param pointer_eeprom	Adresse im EEPROM
 * @param size				Groesse des Blocks in Byte
 */ 
void eeprom_read_block(void *pointer_ram, const void *pointer_eeprom, size_t size) {
	uint16_t address = conv_eeaddr(EEPROM_ADDR(pointer_eeprom));
	if (address+size >= EE_SIZE) return;
	uint8_t * ptr = eeprom + address;
	memcpy(pointer_ram, ptr, size);
}

/*!
 * Speichert ein Byte im EEPROM.
 * @param addr	Adresse im EEPROM zwischen 0 und 1023
 * @param value	Das abzulegende Byte
 */   
void eeprom_write_byte(uint8_t * addr, uint8_t value) {
	uint16_t address = conv_eeaddr(EEPROM_ADDR(addr));
	if (address >= EE_SIZE) return;
	eeprom[address] = value;
	flush_eeprom_cache();
}

/*!
 * Speichert ein Word im EEPROM.
 * @param addr	Adresse im EEPROM zwischen 0 und 1023
 * @param value	Das abzulegende Word
 */
void eeprom_write_word(uint16_t * addr, uint16_t value) {
	uint16_t address = conv_eeaddr(EEPROM_ADDR(addr));
	if (address >= EE_SIZE-1) return;
	uint8_t * ptr = eeprom + address;
	*(uint16_t *)ptr = value;	
	flush_eeprom_cache();
}

/*! 
 * Kopiert einen Block vom RAM in das EEPROM
 * @param pointer_ram		Adresse im RAM
 * @param pointer_eeprom	Adresse im EEPROM
 * @param size				Groesse des Blocks in Byte
 */ 
void eeprom_write_block(const void *pointer_ram, void *pointer_eeprom, size_t size) {
	uint16_t address = conv_eeaddr(EEPROM_ADDR(pointer_eeprom));
	if (address+size >= EE_SIZE) return;
	uint8_t * ptr = eeprom + address;
	memcpy(ptr, pointer_ram, size);
	flush_eeprom_cache();
}

#endif	// PC
