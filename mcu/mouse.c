/*! @file 	mouse.h 
 * @brief 	Routinen f�r die Ansteuerung eines opt. Maussensors
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/


#ifdef MCU 

#include <avr/io.h>
#include "ct-Bot.h"

#ifdef MAUS_AVAILABLE


int maus_x=0;	///< X-Koordinate
int maus_y=0;	///< Y-Koordinate

#define MAUS_SCK_DDR 	DDRB
#define MAUS_SCK_PORT 	PORTB
#define MAUS_SCK_BIT	0x80

#define MAUS_SDA_DDR 	DDRB
#define MAUS_SDA_PORT 	PORTB
#define MAUS_SDA_PIN 	PINB
#define MAUS_SDA_BIT 	0x40
#define MAUS_SDA_NR	6

/*!
 * Überträgt ein Byte an den Sensor
 * @param data das Byte
 */
void maus_sens_writeByte(char data){
	int i;
	MAUS_SCK_DDR  |= MAUS_SCK_BIT; 	// SCK auf Output
	MAUS_SDA_DDR  |= MAUS_SDA_BIT; 	// SDA auf Output
	
	for (i=7; i>-1; i--){
		asm("nop");
		// SCK =0 Daten auf der fallenden Flanke vorbereiten 
		MAUS_SCK_PORT &= ~MAUS_SCK_BIT;	
		//Daten rausschreiben
		MAUS_SDA_PORT = (MAUS_SDA_PORT & (~MAUS_SDA_BIT)) | (((data >> i) & 0x01)<<MAUS_SDA_NR);
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		// SCK =1 Sensor �bernimmt auf steigender Flanke
		MAUS_SCK_PORT |= MAUS_SCK_BIT;	
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
	MAUS_SCK_DDR  |= MAUS_SCK_BIT; 	// SCK auf Output
	MAUS_SDA_DDR  &= ~MAUS_SDA_BIT; 	// SDA auf Input
	
	for (i=7; i>-1; i--){
		asm("nop");
		// SCK =0 Daten auf der fallenden Flanke vorbereiten 
		MAUS_SCK_PORT &= ~MAUS_SCK_BIT;	
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		// SCK =1 Daten lesen  auf steigender Flanke
		MAUS_SCK_PORT |= MAUS_SCK_BIT;
		
		//Daten lesen
		data=data<<1;
		data |= (MAUS_SDA_PIN & MAUS_SDA_BIT) >> MAUS_SDA_NR;
	}
	for (i=0; i<25; i++){asm("nop");}	// delay at least 10�s Can be removed
	MAUS_SDA_DDR  |= MAUS_SDA_BIT; 	// SDA auf Output
	return data;
}

/*!
 * Überträgt ein write-Kommando an den Sensor
 * @param adr Adresse
 * @param data Datum
 */
void maus_sens_write(char adr, char data){
	maus_sens_writeByte(adr);
	maus_sens_writeByte(data);
	for (int i=0; i<250; i++){asm("nop");}	// delay at least 100�s
}

/*!
 * Schickt ein Lesekommando an den Sensor
 * und liest ein Byte zurück
 * @param adr die Adresse
 * @return das Datum
 */
char maus_sens_read(char adr){
	maus_sens_writeByte(adr);
	for (int i=0; i<250; i++){asm("nop");}	// delay at least 100�s
	return maus_sens_readByte();
}

/*! 
 * Initialisiere Maussensor
 */ 
void maus_sens_init(void){
	maus_sens_write(0x00,0x80);	//Reset sensor
	maus_sens_write(0x00,0x01);	//Always on
}


/*! 
 * Aktualisiere die Position des Maussensors
 */
void maus_sens_pos(void){
	int8 data;

	data=maus_sens_read(0x02);
	maus_y+=data;
	data=maus_sens_read(0x03);
	maus_x+=data;	
}
#endif
#endif
