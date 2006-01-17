/*! @file 	mouse.h 
 * @brief 	Routinen f�r die Ansteuerung eines opt. Maussensors
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/
#include "global.h"


#ifdef MCU 

#include <avr/io.h>
#include "ct-Bot.h"

#ifdef MAUS_AVAILABLE

//int setSensMouseDX=0;	///< X-Koordinate
//int setSensMouseDY=0;	///< Y-Koordinate

#define MAUS_DDR 	DDRB	///< DDR für Maus-SCLK
#define MAUS_PORT 	PORTB	///< PORT für Maus-SCLK
#define MAUS_SCK_PIN	0x80

#define MAUS_SDA_PIN 	PINB
#define MAUS_SDA_BIT 	0x40

#define MAUS_SDA_NR	6

/*!
 * Überträgt ein Byte an den Sensor
 * @param data das Byte
 */
void maus_sens_writeByte(char data){
	int i;
	MAUS_DDR  |= MAUS_SCK_PIN | MAUS_SDA_BIT; 	// SCK auf Output
												// SDA auf Output
	
	for (i=7; i>-1; i--){
		asm("nop");
		// SCK =0 Daten auf der fallenden Flanke vorbereiten 
		MAUS_PORT &= ~MAUS_SCK_PIN;	
		//Daten rausschreiben
		MAUS_PORT = (MAUS_PORT & (~MAUS_SDA_PIN)) | (((data >> i) & 0x01)<<MAUS_SDA_NR);
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		// SCK =1 Sensor �bernimmt auf steigender Flanke
		MAUS_PORT |= MAUS_SCK_PIN;	
	}
	
	for (i=0; i<25; i++){asm("nop");}	// delay at least 10�s Can be removed
}

/*!
 * Liest ein Byte vom Sensor
 * @return das Byte
 */
char maus_sens_readByte(void){
	int i;
	char data=0;
	MAUS_DDR  |= MAUS_SCK_PIN; 	// SCK auf Output
	MAUS_DDR  &= ~MAUS_SDA_BIT; 	// SDA auf Input
	
	for (i=7; i>-1; i--){
		asm("nop");
		// SCK =0 Daten auf der fallenden Flanke vorbereiten 
		MAUS_PORT &= ~MAUS_SCK_PIN;	
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		// SCK =1 Daten lesen  auf steigender Flanke
		MAUS_PORT |= MAUS_SCK_PIN;
		
		//Daten lesen
		data=data<<1;
		data |= (MAUS_SDA_PIN & MAUS_SDA_BIT) >> MAUS_SDA_NR;
	}
	for (i=0; i<25; i++){asm("nop");}	// delay at least 10�s Can be removed
	MAUS_DDR  |= MAUS_SDA_BIT; 	// SDA auf Output
	return data;
}

/*!
 * Überträgt ein write-Kommando an den Sensor
 * @param adr Adresse
 * @param data Datum
 */
void maus_sens_write(char adr, char data){
	int i;
	maus_sens_writeByte(adr);
	maus_sens_writeByte(data);
	for (i=0; i<250; i++){asm("nop");}	// delay at least 100�s
}

/*!
 * Schickt ein Lesekommando an den Sensor
 * und liest ein Byte zurück
 * @param adr die Adresse
 * @return das Datum
 */
int maus_sens_read(char adr){
	int i;
	maus_sens_writeByte(adr);
	for (i=0; i<250; i++){asm("nop");}	// delay at least 100�s
	return maus_sens_readByte();
}

/*! 
 * Initialisiere Maussensor
 */ 
void maus_sens_init(void){
	maus_sens_write(0x00,0x80);	//Reset sensor
	maus_sens_write(0x00,0x01);	//Always on
}


#endif
#endif
