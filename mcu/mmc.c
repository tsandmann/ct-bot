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

/*! @file 	mmc.c
 * @brief 	Routinen zum Auslesen/Schreiben einer MMC-Karte
 * @author 	Benjamin Benz (bbe@heise.de)
 * @author  Ulrich Radig (mail@ulrichradig.de) www.ulrichradig.de
 * @date 	07.11.06
*/

#include "ct-Bot.h"

#ifdef MCU
#ifdef MMC_AVAILABLE

#include "mmc.h"
#include <avr/io.h>
#include "ena.h"

#include <avr/interrupt.h>
#ifndef NEW_AVR_LIB
	#include <avr/signal.h>
#endif

//#define SPI_Mode				//1 = Hardware SPI | 0 = Software SPI

//set MMC_Chip_Select to high (MMC/SD-Karte Inaktiv)
#define MMC_Disable() ENA_off(ENA_ERW2)

//set MMC_Chip_Select to low (MMC/SD-Karte Aktiv)
#define MMC_Enable() ENA_on(ENA_ERW2);

#define nop()  __asm__ __volatile__ ("nop" ::)

#ifndef SPI_Mode
	#define MMC_PORT_OUT		PORTB	//Port an der die MMC/SD-Karte angeschlossen ist also des SPI 
	#define MMC_PORT_IN		PINB
	#define MMC_DDR			DDRB	
	#define SPI_DI				6		//Port Pin an dem Data Output der MMC/SD-Karte angeschlossen ist 
	#define SPI_DO				5		//Port Pin an dem Data Input der MMC/SD-Karte angeschlossen ist
	
	#define MMC_CLK_DDR		DDRB
	#define MMC_CLK_PORT		PORTB
	#define SPI_CLK			7		//Port Pin an dem die Clock der MMC/SD-Karte angeschlossen ist (clk)
	
	//Set Output Low
	#define MMC_DO_LOW()	 MMC_PORT_OUT &= ~(1<<SPI_DO)
	//Set Output High  
	#define MMC_DO_HIGH()	 MMC_PORT_OUT |= (1<<SPI_DO)
	
	#define MMC_CLK_LOW() 	MMC_CLK_PORT &= ~(1<<SPI_CLK)
	#define MMC_CLK_HIGH() MMC_CLK_PORT |= (1<<SPI_CLK)
	
	#define MMC_CLK() 		 {MMC_CLK_LOW();	MMC_CLK_HIGH();} 
	
	#define MMC_prepare()	{ MMC_DDR &=~(1<<SPI_DI);	 MMC_DDR |= (1<<SPI_DO); } 
#else
	#define MMC_prepare() 	SPCR |= _BV(MSTR)
#endif


/*! 
 * Schreibt ein Byte an die Karte
 * @param data das Byte
 */
inline void mmc_write_byte (uint8 data) {
	#ifdef SPI_Mode		//Routine fuer Hardware SPI
		SPDR = data; 	//Sendet ein Byte
		while(!(SPSR & (1<<SPIF))) {}//Wartet bis Byte gesendet wurde
	#else			//Routine fr Software SPI
		uint8 a=0x80;
		while (a >0) {
			if ( (data & a) >0)	//Ist Bit a in Data gesetzt
				MMC_DO_HIGH();
			else
				MMC_DO_LOW(); 
				
			MMC_CLK();
			a = a>>1;
		}
		MMC_DO_HIGH();	//setzt Output wieder auf High	
	#endif
}

/*!
 * Liest ein Byte von der karte
 * @return das Byte
 */
inline uint8 mmc_read_byte (void){
	uint8 data = 0;
	#ifdef SPI_Mode	//Routine fr Hardware SPI
		SPDR = 0xff;
		while(!(SPSR & (1<<SPIF))){};
		data = SPDR;
	
	#else			//Routine fr Software SPI
		uint8 a;
		for (a=8; a>0; a--){ //das Byte wird Bitweise nacheinander Empangen MSB First
			MMC_CLK_LOW(); //erzeugt ein Clock Impuls (Low) 
			nop();nop();
			if ( (MMC_PORT_IN & _BV(SPI_DI)) > 0) //Lesen des Pegels von MMC_DI
				data |= (1<<(a-1));
			else
				data &=~(1<<(a-1));
				
			MMC_CLK_HIGH(); //setzt Clock Impuls wieder auf (High)		
			nop();nop();
		}
	#endif
	return data;
}


/*!
 * Schickt ein Kommando an die Karte
 * @param cmd ein Zeiger auf das Kommando
 * @return Die Antwort der Karte oder 0xFF im Fehlerfall
 */
uint8 mmc_write_command (uint8 *cmd){
	uint8 result = 0xff;
	uint16 Timeout = 0;

	//set MMC_Chip_Select to high (MMC/SD-Karte Inaktiv) 
	MMC_Disable();

	// Da ein paar Leitungen noch von anderer Hardware mitbenutzt werden: reinit
	MMC_prepare();

	//sendet 8 Clock Impulse
	mmc_write_byte(0xFF);

	//set MMC_Chip_Select to low (MMC/SD-Karte Aktiv)
	MMC_Enable();

	//sendet 6 Byte Commando
	uint8 a;
	for (a = 0; a < 6; a++) //sendet 6 Byte Commando zur MMC/SD-Karte
		mmc_write_byte(*cmd++);

	//Wartet auf ein gueltige Antwort von der MMC/SD-Karte
	while (result == 0xff)	{
		result = mmc_read_byte();
		if (Timeout++ > 500)
			break; //Abbruch da die MMC/SD-Karte nicht Antwortet
	}
	
	return result;
}


/*! 
 * Initialisiere die SD/MMC-Karte
 * @return 0 wenn allles ok, sonst nummer des Kommandos bei dem abgebrochen wurde
 */
uint8 mmc_init (void) {
	uint8 Timeout = 0;
	uint8 a;

	#ifdef SPI_Mode
		//Aktiviren des SPI - Bus, Clock = Idel LOW
		//SPI Clock teilen durch 128
		SPCR = (1<< SPE)  |	// Enable SPI
	 		   (1<< MSTR) |	// Master-Mode
	 		   (1<< SPIE) |  // Interrupt an
  		   	   (1<< SPR0) |  (1<<SPR1); // Langsame Speed zum initialisieren fosc/128
		SPSR &= ~(1<<SPI2X);		// Doppeltspeed-Mode aus
	#else
		//Konfiguration des Ports an der die MMC/SD-Karte angeschlossen wurde
		MMC_CLK_DDR |= _BV(SPI_CLK);				//Setzen von Pin MMC_Clock auf Output
	#endif
	MMC_prepare();

	MMC_Disable();
	
	//Initialisiere MMC/SD-Karte in den SPI-Mode
	for (a = 0;a<0x0f;a++) //Sendet min 74+ Clocks an die MMC/SD-Karte
		mmc_write_byte(0xff);
	
	//Sendet Commando CMD0 an MMC/SD-Karte
	uint8 CMD[] = {0x40,0x00,0x00,0x00,0x00,0x95};
	while(mmc_write_command (CMD) !=1)	{
		if (Timeout++ > 200){
			MMC_Disable();
			return(1); //Abbruch bei Commando1 (Return Code1)
		}
	}
	//Sendet Commando CMD1 an MMC/SD-Karte
	Timeout = 0;
	CMD[0] = 0x41;//Commando 1
	CMD[5] = 0xFF;
	while( mmc_write_command (CMD) !=0)	{
		if (Timeout++ > 100){
			MMC_Disable();
			return(2); //Abbruch bei Commando2 (Return Code2)
		}
	}
	#ifdef SPI_Mode
		//SPI Bus auf max Geschwindigkeit (fosc/2)
		SPCR &= ~((1<<SPR0) | (1<<SPR1));
		SPSR |= (1<<SPI2X);
	#endif	
	
	//set MMC_Chip_Select to high (MMC/SD-Karte Inaktiv)
	MMC_Disable();
	return(0);
}

/*! Schreibt einen 512-Byte Sektor auf die Karte
 * @param addr Nummer des 512-Byte Blocks
 * @param Buffer zeiger auf den Puffer
 * @return 0 wenn alles ok
 */
uint8 mmc_write_sector (uint32 addr,uint8 *Buffer){
	uint8 tmp;
	//Commando 24 zum schreiben eines Blocks auf die MMC/SD - Karte
	uint8 cmd[] = {0x58,0x00,0x00,0x00,0x00,0xFF}; 
	
	/*Die Adressierung der MMC/SD-Karte wird in Bytes angegeben,
	  addr wird von Blocks zu Bytes umgerechnet danach werden 
	  diese in das Commando eingefgt*/
	  
	addr = addr << 9; //addr = addr * 512
	
	cmd[1] = ((addr & 0xFF000000) >>24 );
	cmd[2] = ((addr & 0x00FF0000) >>16 );
	cmd[3] = ((addr & 0x0000FF00) >>8 );

	//Sendet Commando cmd24 an MMC/SD-Karte (Write 1 Block/512 Bytes)
	tmp = mmc_write_command (cmd);
	if (tmp != 0)
		return(tmp);
			
	uint8 a;
	//Wartet einen Moment und sendet einen Clock an die MMC/SD-Karte
	for (a=0;a<100;a++)	{
		mmc_read_byte();
	}
	
	//Sendet Start Byte an MMC/SD-Karte
	mmc_write_byte(0xFE);	
	
	uint16 b;
	//Schreiben des Bolcks (512Bytes) auf MMC/SD-Karte
	for (b=0;b<512;b++)	{
		mmc_write_byte(*Buffer++);
	}
	
	//CRC-Byte schreiben
	mmc_write_byte(0xFF); //Schreibt Dummy CRC
	mmc_write_byte(0xFF); //CRC Code wird nicht benutzt
	
	//Wartet auf MMC/SD-Karte Bussy
	// TODO Gefahr einer Entdlosschleife
	while (mmc_read_byte() != 0xff){};
	
	//set MMC_Chip_Select to high (MMC/SD-Karte Inaktiv)
	MMC_Disable();
	
	return(0);
}

/*!
 * Liest einen Block von der Karte
 * @param cmd Zeiger auf das Kommando, das erstmal an die Karte geht
 * @param Buffer ein Puffer mit mindestens count Bytes
 * @param count Anzahl der zu lesenden Bytes
 */
void mmc_read_block(uint8 *cmd,uint8 *Buffer,uint16 count){	
	//Sendet Commando cmd an MMC/SD-Karte
	if (mmc_write_command (cmd) != 0)
		 return;

	//Wartet auf Start Byte von der MMC/SD-Karte (FEh/Start Byte)
	
	while (mmc_read_byte() != 0xfe){};

	uint16 a;
	//Lesen des Bolcks (normal 512Bytes) von MMC/SD-Karte
	for (a=0;a<count;a++)
		*Buffer++ = mmc_read_byte();

	//CRC-Byte auslesen
	mmc_read_byte();//CRC - Byte wird nicht ausgewertet
	mmc_read_byte();//CRC - Byte wird nicht ausgewertet
	
	//set MMC_Chip_Select to high (MMC/SD-Karte Inaktiv)
	MMC_Disable();
}

/*!
 * Liest einen Block von der Karte
 * @param addr Nummer des 512-Byte Blocks
 * @param Buffer Puffer von mindestens 512 Byte
 */
void mmc_read_sector (uint32 addr,uint8 *Buffer){	
	//Commando 16 zum lesen eines Blocks von der MMC/SD - Karte
	uint8 cmd[] = {0x51,0x00,0x00,0x00,0x00,0xFF}; 
	
	/*Die Adressierung der MMC/SD-Karte wird in Bytes angegeben,
	  addr wird von Blocks zu Bytes umgerechnet danach werden 
	  diese in das Commando eingefgt*/
	  
	addr = addr << 9; //addr = addr * 512

	cmd[1] = ((addr & 0xFF000000) >>24 );
	cmd[2] = ((addr & 0x00FF0000) >>16 );
	cmd[3] = ((addr & 0x0000FF00) >>8 );

    mmc_read_block(cmd,Buffer,512);
}

#ifdef SPI_Mode
	/*!
	 *  Interrupt Handler fuer den Datenempfang per UART
	 */
	SIGNAL (SIG_SPI){
		// Wenn der SS-Pin auf Low geht, schaltet die SPI sich in den Slave-Mode, das wollen wir nicht
		if ((SPCR & _BV(MSTR))==0)
			MMC_prepare();	// also Master wieder an
	}
#endif

#ifdef MMC_INFO_AVAILABLE
/*!
 * Liest das CID-Register (16 Byte) von der Karte
 * @param Buffer Puffer von mindestens 16 Byte
 */
void mmc_read_cid (uint8 *Buffer){
	//Commando zum lesen des CID Registers
	uint8 cmd[] = {0x4A,0x00,0x00,0x00,0x00,0xFF}; 
	mmc_read_block(cmd,Buffer,16);
}

/*!
 * Liest das CSD-Register (16 Byte) von der Karte
 * @param Buffer Puffer von mindestens 16 Byte
 */
void mmc_read_csd (uint8 *Buffer){	
	//Commando zum lesen des CSD Registers
	uint8 cmd[] = {0x49,0x00,0x00,0x00,0x00,0xFF};
	mmc_read_block(cmd,Buffer,16);
}

/*!
 * Liefert die Groesse der Karte zurueck
 * @return Groesse der Karte in Byte. Bei einer 4 GByte-Karte kommt 0xFFFFFFFF zurueck
 */
uint32 mmc_get_size(void){
	uint8 csd[16];
	
	mmc_read_csd(csd);
	
	uint32 size = (csd[8]>>6) + (csd[7] << 2) + ((csd[6] & 0x03) << 10); // c_size
	size +=1;		// Fest in der Formel drin
	
	uint8 shift = 2;	// eine 2 ist fest in der Formel drin
	shift += (csd[10]>>7) + ((csd[9] & 0x03) <<1); // c_size_mult beruecksichtigen
	shift += csd[5] & 0x0f;	// Block groesse beruecksichtigen

	size = size << shift;
		
	return size;
}


#endif //MMC_INFO_AVAILABLE

#ifdef MMC_WRITE_TEST_AVAILABLE
	/*! Testet die MMC-Karte. Schreibt nacheinander 2 Sektoren a 512 Byte mit testdaten voll und liest sie wieder aus
	 * !!! Achtung loescht die Karte
	 * @return 0, wenn alles ok
	 */
	uint8 mmc_test(void){
		uint8 buffer[512];
		uint16 i;
		
		// Puffer vorbereiten
		for (i=0; i< 512; i++)	buffer[i]= (i & 0xFF);
		// und schreiben
		mmc_write_sector(0,buffer);
		
		// Puffer vorbereiten
		for (i=0; i< 512; i++)	buffer[i]= 255 - (i & 0xFF);	
		// und schreiben
		mmc_write_sector(512,buffer);	
	
		// Puffer lesen	
		mmc_read_sector(0,buffer);	
		// und vergleichen
		for (i=0; i< 512; i++)
			if (buffer[i] != (i & 0xFF))
				return 1;
	
	
		// Puffer lesen	
		mmc_read_sector(512,buffer);
		// und vergleichen
		for (i=0; i< 512; i++)
			if (buffer[i] != (255- (i & 0xFF)))
				return 1;
		
		// hierher kommen wir nur, wenn alles ok ist
		return 0;
	}
#endif //MMC_WRITE_TEST_AVAILABLE

#endif
#endif
