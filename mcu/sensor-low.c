/*! @file 	bot-sens.c  
 * @brief 	Low-Level Routinen für die Sensor Steuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/

#ifdef MCU

#include <avr/io.h>
#include "adc.h" 
#include "global.h"

#include "ena.h"
#include "sensor.h"
//#include "bot-mot.h"

// ADC-PINS
#define SENS_ABST_L	0		///< ADC-PIN Abstandssensor Links	
#define SENS_ABST_R	1		///< ADC-PIN Abstandssensor Rechts
#define SENS_M_L		2		///< ADC-PIN Liniensensor Links
#define SENS_M_R		3		///< ADC-PIN Liniensensor Rechts
#define SENS_LDR_R		4		///< ADC-PIN Lichtsensor Links
#define SENS_LDR_L		5		///< ADC-PIN Lichtsensor Rechts
#define SENS_KANTE_L	6		///< ADC-PIN Kantensensor Links
#define SENS_KANTE_R	7		///< ADC-PIN Kantensensor Rechts


// Sonstige Sensoren
#define SENS_DOOR_PINR 	PIND	///< Port an dem der Klappensensor hängt
#define SENS_DOOR_DDR 		DDRD	///< DDR für den Klappensensor
#define SENS_DOOR			6		///< Pin  an dem der Klappensensor hängt

#define SENS_ENCL_PINR		PINB	///< Port an dem der linke Encoder hängt
#define SENS_ENCL_DDR		DDRB	///< DDR für den linken Encoder 
#define SENS_ENCL			4		///< Pin an dem der linke Encoder hängt

#define SENS_ENCR_PINR		PIND	///< Port an dem der rechte Encoder hängt
#define SENS_ENCR_DDR		DDRD	///< DDR für den rechten Encoder 
#define SENS_ENCR			3		///< Pin an dem der rechte Encoder hängt

#define SENS_ERROR_PINR	PINB	///< Port an dem die Fehlerüberwachung hängt
#define SENS_ERROR_DDR		DDRB	///< DDR für die Fehlerüberwachung
#define SENS_ERROR			2		///< Pin an dem die Fehlerüberwachung hängt

#define SENS_TRANS_PINR	PINB	///< Port an dem die Transportfachueberwachung haengt
#define SENS_TRANS_PORT	PORTB	///< Port an dem die Transportfachueberwachung haengt
#define SENS_TRANS_DDR		DDRB	///< DDR für die Transportfachueberwachung
#define SENS_TRANS			0		///< Pin an dem die Transportfachueberwachung haengt

/*
#define	ENC_ENTPRELL	4
*/




volatile char enc_l=0;		///< Puffer f�r die letzten Encoder-St�nde
volatile char enc_r=0;		///< Puffer f�r die letzten Encoder-St�nde

volatile char enc_l_cnt=0;	///< Entprell-Counter f�r L-Encoder	
volatile char enc_r_cnt=0;	///< Entprell-Counter f�r R-Encoder	



/*!
 * Initialisiere alle Sensoren
 */
void bot_sens_init(void){
	ENA_init();
	adc_init(0xFF);		// Alle ADC-Ports aktivieren
	
	ENA_set(0xFF);		// Alle Sensoren aktivieren
	SENS_DOOR_DDR &= ~ (1<<SENS_DOOR);	// Input
	
	SENS_ENCL_DDR &= ~ (1<<SENS_ENCL);	// Input
	SENS_ENCR_DDR &= ~(1<<SENS_ENCR);	// Input

	SENS_ERROR_DDR &= ~(1<<SENS_ERROR);	// Input

	SENS_TRANS_DDR &= ~(1<<SENS_TRANS);	// Input	
	SENS_TRANS_PORT |= (1<<SENS_TRANS);  // Pullup an
/*	
	BOT_ENC_L_DDR&= ~BOT_ENC_L_PIN;
	BOT_ENC_R_DDR&= ~BOT_ENC_R_PIN;
*/	
	// Reset Encoder-Counter
	sensEncL=0;
	sensEncR=0;

}

/*!
 * Alle Sensoren aktualisieren
 */
void bot_sens_isr(void){
	sensLDRL = adc_read(SENS_LDR_L);
	sensLDRR = adc_read(SENS_LDR_R);

	sensBorderL = adc_read(SENS_KANTE_L);
	sensBorderR = adc_read(SENS_KANTE_R);
	
	sensLineL = adc_read(SENS_M_L);
	sensLineR = adc_read(SENS_M_R);

	//Aktualisiere Distanz-Sensoren
	// TODO Umrechnen in mm
	sensDistL= adc_read(SENS_ABST_L);
	sensDistR= adc_read(SENS_ABST_R);
		
	sensDoor = (SENS_DOOR_PINR >> SENS_DOOR) & 0x01;
	
	sensEncL = (SENS_ENCL_PINR >> SENS_ENCL) & 0x01;
	sensEncR = (SENS_ENCR_PINR >> SENS_ENCR) & 0x01;

	sensError = (SENS_ERROR_PINR >> SENS_ERROR) & 0x01;		
	
	sensTrans = (SENS_TRANS_PINR >> SENS_TRANS) & 0x01;		
	

/*
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
	*/
}
#endif
