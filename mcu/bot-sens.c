/*! @file 	bot-sens.c  
 * @brief 	Low-Level Routinen für die Sensor Steuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/

#ifdef MCU

#include <avr/io.h>
#include "adc.h" 
#include "global.h"

#include "bot-mot.h"
//IR-Sensoren
//IO9
#define BOT_IR_L   3
#define BOT_IR_L_PIN (1<<BOT_IR_L)	

//IO11
#define BOT_IR_R   5
#define BOT_IR_R_PIN (1<<BOT_IR_R)	

// Encoder - Rad�berwachung
#define BOT_ENC_L_PIN 0x10	// IO4
#define BOT_ENC_L_PORT PORTC
#define BOT_ENC_L_IN PINC
#define BOT_ENC_L_DDR DDRC

#define BOT_ENC_R_PIN 0x20	// IO5
#define BOT_ENC_R_PORT PORTC
#define BOT_ENC_R_IN PINC
#define BOT_ENC_R_DDR DDRC

#define	ENC_ENTPRELL	4

//Encoder Linker/Rechter Motor
#define ENC_L ((BOT_ENC_L_IN & BOT_ENC_L_PIN) / BOT_ENC_L_PIN)
#define ENC_R ((BOT_ENC_R_IN & BOT_ENC_R_PIN) / BOT_ENC_R_PIN)


volatile int16 sensDistL=0;		///< Distanz linker IR-Sensor
volatile int16 sensDistR=0;	///< Distanz rechter IR-Sensor

volatile int16 sensBorderL=0;	///< Abgrundsensor links
volatile int16 sensBorderR=0;	///< Abgrundsensor rechts

volatile int16 sensLineL=0;		///< Lininensensor links
volatile int16 sensLlineR=0;	///< Lininensensor rechts

volatile int16 sensLdrL=0;		///< Helligkeitssensor links
volatile int16 sensLdrR=0;		///< Helligkeitssensor links

volatile char sensTrans=0;		///< Sensor Überwachung Transportfach

volatile char sensDoor=0;		///< Sensor Überwachung Klappe

volatile char sensError=0;		///< Überwachung Motor oder Batteriefehler

volatile int encoderL=0;	///< Encoder linker Motor
volatile int encoderR=0;	///< Encoder rechter Motor

volatile char enc_l=0;		///< Puffer f�r die letzten Encoder-St�nde
volatile char enc_r=0;		///< Puffer f�r die letzten Encoder-St�nde

volatile char enc_l_cnt=0;	///< Entprell-Counter f�r L-Encoder	
volatile char enc_r_cnt=0;	///< Entprell-Counter f�r R-Encoder	

/*!
 * Initialisiere alle Sensoren
 */
void bot_sens_init(void){
	adc_init(BOT_IR_L_PIN|BOT_IR_R_PIN);
	
	BOT_ENC_L_DDR&= ~BOT_ENC_L_PIN;
	BOT_ENC_R_DDR&= ~BOT_ENC_R_PIN;
	
	// Reset Encoder-Counter
	encoderL=0;
	encoderR=0;
}

/*!
 * Alle Sensoren aktualisieren
 */
void bot_sens_isr(void){
	//Aktualisiere Distanz-Sensoren
	// TODO Umrechnen in mm
	sensDistL=adc_read(BOT_IR_L);
	sensDistR=adc_read(BOT_IR_R);
	
	// --------------------- links ----------------------------
	//Rad-Encoder auswerten 
	if (ENC_L != enc_l){	// uns interesieren nur Ver�nderungen
		enc_l=ENC_L;	// neuen wert sichern
		enc_l_cnt=0;
	} else {// Z�hlen, wie lange Pegel bleibt
		if (enc_l_cnt <= ENC_ENTPRELL) // Nur bis zur Entprell-Marke
			enc_l_cnt++;	
	}

	if (enc_l_cnt == ENC_ENTPRELL){// wenn lange genug konst
		if (mot_l_dir==1)	// Drehrichtung beachten
			encoderL++;	//vorw�rts
		else 
			encoderL--;	//r�ckw�rts
	}
	// --------------------- rechts ----------------------------
	if (ENC_R != enc_r){	// uns interesieren nur Ver�nderungen
		enc_r=ENC_R;	// neuen wert sichern
		enc_r_cnt=0;
	} else{ // Z�hlen, wie lange Pegel bleibt
		if (enc_r_cnt <= ENC_ENTPRELL) // Nur bis zur Entprell-Marke
			enc_r_cnt++;	
	}

	if (enc_r_cnt == ENC_ENTPRELL){// wenn lange genug konst
		if (mot_r_dir==1)	// Drehrichtung beachten
			encoderR++;	//vorw�rts
		else 
			encoderR--;	//r�ckw�rts
	}
}
#endif
