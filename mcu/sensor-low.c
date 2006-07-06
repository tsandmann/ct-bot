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

/*! @file 	sensor-low.c  
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
#include "mouse.h"
#include "motor.h"
#include "timer.h"
#include "sensor_correction.h"

// ADC-PINS
#define SENS_ABST_L	0		/*!< ADC-PIN Abstandssensor Links */
#define SENS_ABST_R	1		/*!< ADC-PIN Abstandssensor Rechts */
#define SENS_M_L		2		/*!< ADC-PIN Liniensensor Links */
#define SENS_M_R		3		/*!< ADC-PIN Liniensensor Rechts */
#define SENS_LDR_L		4		/*!< ADC-PIN Lichtsensor Links */
#define SENS_LDR_R		5		/*!< ADC-PIN Lichtsensor Rechts */
#define SENS_KANTE_L	6		/*!< ADC-PIN Kantensensor Links */
#define SENS_KANTE_R	7		/*!< ADC-PIN Kantensensor Rechts */

// Sonstige Sensoren
#define SENS_DOOR_PINR 	PIND	/*!< Port an dem der Klappensensor hängt */
#define SENS_DOOR_DDR 		DDRD	/*!< DDR für den Klappensensor */
#define SENS_DOOR			6		/*!< Pin  an dem der Klappensensor hängt */

#define SENS_ENCL_PINR		PINB	/*!< Port an dem der linke Encoder hängt */
#define SENS_ENCL_DDR		DDRB	/*!< DDR für den linken Encoder  */
#define SENS_ENCL			4		/*!< Pin an dem der linke Encoder hängt */

#define SENS_ENCR_PINR		PIND	/*!< Port an dem der rechte Encoder hängt */
#define SENS_ENCR_DDR		DDRD	/*!< DDR für den rechten Encoder  */
#define SENS_ENCR			3		/*!< Pin an dem der rechte Encoder hängt */

#define SENS_ERROR_PINR	PINB	/*!< Port an dem die Fehlerüberwachung hängt */
#define SENS_ERROR_DDR		DDRB	/*!< DDR für die Fehlerüberwachung */
#define SENS_ERROR			2		/*!< Pin an dem die Fehlerüberwachung hängt */

#define SENS_TRANS_PINR	PINB	/*!< Port an dem die Transportfachueberwachung haengt */
#define SENS_TRANS_PORT	PORTB	/*!< Port an dem die Transportfachueberwachung haengt */
#define SENS_TRANS_DDR		DDRB	/*!< DDR für die Transportfachueberwachung */
#define SENS_TRANS			0		/*!< Pin an dem die Transportfachueberwachung haengt */

#define ENC_L ((SENS_ENCL_PINR >> SENS_ENCL) & 0x01)	/*!< Abkuerzung zum Zugriff auf Encoder */
#define ENC_R ((SENS_ENCR_PINR >> SENS_ENCR) & 0x01)	/*!< Abkuerzung zum Zugriff auf Encoder */

#define ENC_ENTPRELL	12		/*!< Nur wenn der Encoder ein paar mal den gleichen wert gibt uebernehmen */

volatile uint8 enc_l=0;		/*!< Puffer fuer die letzten Encoder-Staende */
volatile uint8 enc_r=0;		/*!< Puffer fuer die letzten Encoder-Staende */

volatile uint8 enc_l_cnt=0;	/*!< Entprell-Counter fuer L-Encoder	 */
volatile uint8 enc_r_cnt=0;	/*!< Entprell-Counter fuer R-Encoder	 */

/* Zeit der letzten Messung der Distanzsensoren */
uint16 olds_dist;
uint16 oldms_dist;
uint8 measure_count;
int16 distLeft[3];
int16 distRight[3];

/*!
 * Initialisiere alle Sensoren
 */
void bot_sens_init(void){	
	ENA_init();
	adc_init(0xFF);		// Alle ADC-Ports aktivieren
	
	ENA_set(ENA_RADLED);		// Alle Sensoren bis auf die Radencoder deaktivieren
	ENA_on(ENA_ABSTAND);		// Die Abstandssensoren ebenfalls dauerhaft an, da sie fast 50 ms zum booten brauchen
	
	SENS_DOOR_DDR &= ~ (1<<SENS_DOOR);	// Input
	
	SENS_ENCL_DDR &= ~ (1<<SENS_ENCL);	// Input
	SENS_ENCR_DDR &= ~(1<<SENS_ENCR);	// Input

	SENS_ERROR_DDR &= ~(1<<SENS_ERROR);	// Input

	SENS_TRANS_DDR &= ~(1<<SENS_TRANS);	// Input	
	SENS_TRANS_PORT |= (1<<SENS_TRANS);  // Pullup an
	
	SENS_ENCL_DDR &= ~(1<<SENS_ENCL);	// Input	
	SENS_ENCR_DDR &= ~(1<<SENS_ENCR);	// Input	
	
	timer_2_init();
}


/*!
 * Alle Sensoren aktualisieren
 * Derzeit pollt diese Routine alle Sensoren. Insbesondere bei den 
 * analogen dauert das eine Weile. Daher kann man hier einiges
 * an Performance gewinnen, wenn man die Routine aufspaltet und
 * zumindest die analogen Sensoren per Interrupt bearbeitet,
 * denn im Moment blockiert adc_read so lange, bis ein Sensorwert ausgelesen ist.
 * Die digitalen Sensoren liefern ihre Werte dagegen unmittelbar
 * Aber Achtung es lohnt auch nicht, immer alles so schnell als moeglich
 * zu aktualiseren, der Bot braucht auch Zeit zum nachdenken ueber Verhalten
 */
void bot_sens_isr(void){
	
	ENA_on(ENA_KANTLED|ENA_MAUS|ENA_SCHRANKE|ENA_KLAPPLED);

	#ifdef MAUS_AVAILABLE
	 	// Aktualisiere die Position des Maussensors 
		sensMouseDX = maus_sens_read(MOUSE_DELTA_X_REG);	
		sensMouseDY = maus_sens_read(MOUSE_DELTA_Y_REG);
	
	#endif

	// ---------- analoge Sensoren -------------------
	sensLDRL = adc_read(SENS_LDR_L);
	sensLDRR = adc_read(SENS_LDR_R);

	sensBorderL = adc_read(SENS_KANTE_L);
	sensBorderR = adc_read(SENS_KANTE_R);
	ENA_off(ENA_KANTLED);
			
	sensLineL = adc_read(SENS_M_L);
	sensLineR = adc_read(SENS_M_R);
	ENA_off(ENA_MAUS);	

	// Aktualisiere Distanz-Sensoren
	// Die Distanzsensoren sind im Normalfall an, da sie 50 ms zum booten brauchen
	// Abfrage nur alle 100ms
	if (timer_get_ms_since(olds_dist,oldms_dist)>=100){
		// Zeit fuer naechste Messung merken
		olds_dist=timer_get_s();
		oldms_dist=timer_get_ms();
		
		// wenn Kalibrierung gewuenscht, den Part Messen und Korrigieren kommentieren
		// und Kalibrieren auskommentieren
		// Kalibirieren
		//distL=adc_read(SENS_ABST_L);
		//distR=adc_read(SENS_ABST_R);

		// Messwert merken
		distLeft[measure_count]=adc_read(SENS_ABST_L);
		distRight[measure_count]=adc_read(SENS_ABST_R);

		measure_count++;
		if (measure_count==3) measure_count=0;
		
		// Schnittwert bilden
		sensor_abstand((distLeft[0]+distLeft[1]+distLeft[2])/3,(distRight[0]+distRight[1]+distRight[2])/3);
	}

		
	// ------- digitale Sensoren ------------------
	sensDoor = (SENS_DOOR_PINR >> SENS_DOOR) & 0x01;
	sensTrans = (SENS_TRANS_PINR >> SENS_TRANS) & 0x01;		
	ENA_off(ENA_SCHRANKE|ENA_KLAPPLED);	

	sensError = (SENS_ERROR_PINR >> SENS_ERROR) & 0x01;		
	
	sensor_update();	// Weiterverarbeitung der rohen Sensordaten 
}

/*!
 * Kuemmert sich um die Radencoder
 * Das muss schneller gehen als die anderen Sensoren,
 * daher Update per Timer-Interrupt und nicht per Polling
 */
void bot_encoder_isr(void){
	// --------------------- links ----------------------------
	//Rad-Encoder auswerten 
	if ( ENC_L != enc_l){	// uns interesieren nur Veraenderungen
		enc_l=ENC_L;			// neuen wert sichern
		enc_l_cnt=0;			// Counter zuruecksetzen
	} else {			// Zaehlen, wie lange Pegel bleibt
		if (enc_l_cnt <= ENC_ENTPRELL) // Nur bis zur Entprell-Marke
			enc_l_cnt++;				
	}

	if (enc_l_cnt == ENC_ENTPRELL){// wenn lange genug konst
		if (direction.left == DIRECTION_FORWARD)	// Drehrichtung beachten
			sensEncL++;	//vorwaerts
		else 
			sensEncL--;	//rueckwaerts
	}
	
	// --------------------- rechts ----------------------------
	if (ENC_R != enc_r){	// uns interesieren nur Veraenderungen
		enc_r=ENC_R;	// neuen wert sichern
		enc_r_cnt=0;
	} else{ // Zaehlen, wie lange Pegel bleibt
		if (enc_r_cnt <= ENC_ENTPRELL) // Nur bis zur Entprell-Marke
			enc_r_cnt++;	
	}

	if (enc_r_cnt == ENC_ENTPRELL){// wenn lange genug konst
		if (direction.right == DIRECTION_FORWARD)	// Drehrichtung beachten
			sensEncR++;	//vorwaerts
		else 
			sensEncR--;	//rueckwaerts
	}
}
#endif
