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
 * @file 	sensor.c  
 * @brief 	Architekturunabhaengiger Teil der Sensorsteuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	15.01.05
 */
#include <stdio.h>
#include "ct-Bot.h"
#include "timer.h"
#include "bot-local.h"
#include "math.h"
#include "sensor_correction.h"
#include "ui/available_screens.h"
#include "display.h"
#include "sensor.h"
#include "mouse.h"
#include "log.h"
#include "srf10.h"

#define HEADING_START		0			// Blickrichtung, mit der der Bot sich initialisiert

#ifdef MCU
	#include <avr/eeprom.h>
#else
	#include "eeprom-emu.h"
#endif

// Defines einiger, haeufiger benoetigter Konstanten
#define DEG2RAD (2*M_PI/360)	/*!< Umrechnung von Grad nach Bogenmass */ 


int16 sensLDRL=0;		/*!< Lichtsensor links */
int16 sensLDRR=0;		/*!< Lichtsensor rechts */

int16 sensDistL=1023;		/*!< Distanz linker IR-Sensor in [mm], wenn korrekt umgerechnet wird */
int16 sensDistR=1023;		/*!< Distanz rechter IR-Sensor in [mm], wenn korrekt umgerechnet wird */
uint8_t sensDistLToggle=0;	/*!< Toggle-Bit des linken IR-Sensors */
uint8_t sensDistRToggle=0;	/*!< Toggle-Bit des rechten IR-Sensors */
/*! Zeiger auf die Auswertungsfunktion fuer die Distanzsensordaten, const. solange sie nicht kalibriert werden */
void (* sensor_update_distance)(int16* const p_sens, uint8* const p_toggle, const distSens_t* ptr, int16 volt) = sensor_dist_lookup;

distSens_t EEPROM sensDistDataL[] = SENSDIST_DATA_LEFT;		/*!< kalibrierte Referenzdaten fuer linken IR-Sensor */
distSens_t EEPROM sensDistDataR[] = SENSDIST_DATA_RIGHT;	/*!< kalibrierte Referenzdaten fuer rechten IR-Sensor */
uint8_t EEPROM sensDistOffset = SENSDIST_OFFSET;			/*!< Spannungs-Offset IR-Sensoren */

int16 sensBorderL=0;	/*!< Abgrundsensor links */
int16 sensBorderR=0;	/*!< Abgrundsensor rechts */

int16 sensLineL=0;	/*!< Lininensensor links */
int16 sensLineR=0;	/*!< Lininensensor rechts */

uint8 sensTrans=0;		/*!< Sensor Ueberwachung Transportfach */

uint8 sensDoor=0;		/*!< Sensor Ueberwachung Klappe */

uint8 sensError=0;		/*!< Ueberwachung Motor oder Batteriefehler */

#ifdef MAUS_AVAILABLE
	int8 sensMouseDX;		/*!< Maussensor Delta X, positive Werte zeigen querab der Fahrtrichtung nach rechts */
	int8 sensMouseDY;		/*!< Maussensor Delta Y, positive Werte zeigen in Fahrtrichtung */
	
	int16 sensMouseX;		/*!< Mausposition X, positive Werte zeigen querab der Fahrtrichtung nach rechts */
	int16 sensMouseY;		/*!< Mausposition Y, positive Werte zeigen in Fahrtrichtung  */
#endif

volatile int16 sensEncL=0;		/*!< Encoder linkes Rad */
volatile int16 sensEncR=0;		/*!< Encoder rechtes Rad */
float heading_enc=HEADING_START;	/*!< Blickrichtung aus Encodern */
float x_enc=0;		/*!< X-Koordinate aus Encodern [mm] */
float y_enc=0;		/*!< Y-Koordinate aus Encodern [mm] */
float v_enc_left=0;	/*!< Abrollgeschwindigkeit des linken Rades in [mm/s] [-128 bis 127] relaisitisch [-50 bis 50] */
float v_enc_right=0;	/*!< Abrollgeschwindigkeit des linken Rades in [mm/s] [-128 bis 127] relaisitisch [-50 bis 50] */
float v_enc_center=0;	/*!< Schnittgeschwindigkeit ueber beide Raeder */

#ifdef PC
	uint16 simultime=0;	/*! Simulierte Zeit */
#endif

#ifdef MEASURE_MOUSE_AVAILABLE
	float heading_mou=HEADING_START;		/*!< Aktuelle Blickrichtung relativ zur Startposition aus Mausmessungen */
	float x_mou=0;			/*!< Aktuelle X-Koordinate in mm relativ zur Startposition aus Mausmessungen */
	float y_mou=0;			/*!< Aktuelle Y-Koordinate in mm relativ zur Startposition aus Mausmessungen */
	float v_mou_center=0;		/*!< Geschwindigkeit in mm/s ausschliesslich aus den Maussensorwerten berechnet */
	float v_mou_left=0;		/*!< ...aufgeteilt auf linkes Rad */
	float v_mou_right=0;		/*!< ...aufgeteilt auf rechtes Rad */
#endif

float heading=HEADING_START;			/*!< Aktuelle Blickrichtung aus Encoder-, Maus- oder gekoppelten Werten */
float x_pos=0;			/*!< Aktuelle X-Position aus Encoder-, Maus- oder gekoppelten Werten */
float y_pos=0;			/*!< Aktuelle Y-Position aus Encoder-, Maus- oder gekoppelten Werten */
float v_left=0;			/*!< Geschwindigkeit linkes Rad aus Encoder-, Maus- oder gekoppelten Werten */
float v_right=0;			/*!< Geschwindigkeit rechtes Rad aus Encoder-, Maus- oder gekoppelten Werten */
float v_center=0;			/*!< Geschwindigkeit im Zentrum des Bots aus Encoder-, Maus- oder gekoppelten Werten */

#ifdef SRF10_AVAILABLE
	uint16 sensSRF10;	/*!< Messergebniss Ultraschallsensor */
#endif

#ifdef CMPS03_AVAILABLE
	cmps03_t sensCmps03 = {{0}};	/*!< Lage laut CMPS03-Kompass */
#endif
	
/*! 
 * @brief		Interpoliert linear zwischen zwei gegebenen Wertepaaren
 * @param x1	groesere Abszisse
 * @param y1	Ordinate zu x1, f(x1)
 * @param x2	kleinere Abszisse
 * @param y2	Ordinate zu x2, f(x2)
 * @param xs	Abzisse des zu interpolierenden Punktes
 * @return		f(xs)
 * @author 		Timo Sandmann (mail@timosandmann.de)
 * @date 		27.01.2007
 * Gibt den Funktionswert einer Stelle auf der errechneten Geraden durch die zwei Punkte zurueck.
 * Achtung, die Funktion rechnet so weit wie moeglich in 8 Bit, das Ergebnis ist nur korrekt, 
 * wenn x1 >= xs >= x2, y2 >= y1, x1 != x2 erfuellt ist!
 */
static inline uint8_t lin_interpolate(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t xs) {
	if (x1 == x2) return -1;
	uint16_t m = ((uint16_t)(y2-y1)<<8) / (uint8_t)(x1-x2);	// m >= 0
	uint8_t x_diff = x1 - xs;
	return (uint8_t)((x_diff*m)>>8) + y1;	// m war kuenstlich um 8 Bit hochskaliert
}

/*! 
 * @brief			Errechnet aus den rohren Distanzsensordaten die zugehoerige Entfernung
 * @param p_sens	Zeiger auf den (Ziel-)Sensorwert
 * @param p_toggle	Zeiger auf die Toggle-Variable des Zielsensors
 * @param ptr		Zeiger auf auf Sensorrohdaten im EEPROM fuer p_sens
 * @param volt_16	Spannungs-Ist-Wert, zu dem die Distanz gesucht wird (in 16 Bit) 
 * @author 			Timo Sandmann (mail@timosandmann.de)
 * @date 			21.04.2007
 */
void sensor_dist_lookup(int16_t *const p_sens, uint8_t *const p_toggle, const distSens_t *ptr, int16_t volt_16) {
	if (sizeof(sensDistDataL) != sizeof(sensDistDataR)) {
		/* sensDistDataL und sensDistDataR muessen gleich gross sein! */
		LOG_ERROR("sensDistData unzulaessig");
		return;
	}
	uint8_t i;
	uint8_t n = sizeof(sensDistDataL)/sizeof(distSens_t)/2;
	uint8_t volt;
	/* Offset einlesen und Messwerte pruefen */
	uint8_t offset = eeprom_read_byte(&sensDistOffset);
	if (volt_16 > 255*8 + offset) volt = 255; 
	else  volt = (volt_16 >> 3) - offset;
	
	/* Spannung in LT-Table suchen */
	uint8 pivot = eeprom_read_byte(&ptr[n-1].voltage);	// in welcher Region muessen wir suchen?
	if (volt > pivot) {
		/* in unterer Haelfte suchen */
		i = 0;
	} else {
		/* in oberer Haelfte suchen */
		i = n;
		ptr += n;
		n = sizeof(sensDistDataL)/sizeof(distSens_t);  
	}
	uint8_t tmp=0;
	for (; i<n; i++) {
		tmp = eeprom_read_byte(&ptr->voltage);
		if (volt > tmp)	// aufsteigend suchen, damit der kritische Fall (kleine Entfernung) schneller gefunden wird
			break;	// ptr zeigt jetzt auf die naechst kleinere Spannung
		ptr++; 
	}
	if (i == 0) {
		/* kleinste Entfernung annehmen, falls reale Entfernung < kleinste bekannte Entfernung */
		*p_sens = SENS_IR_MIN_DIST;	// SENS_IR_INFINITE waere eigentlich besser, das mag aber maze nicht
		/* Sensorupdate-Info toggeln und beenden */
		*p_toggle = ~*p_toggle;
		return;
	} 
	
	/* Entfernung berechnen und speichern */
	uint8 distance = lin_interpolate(eeprom_read_byte(&(ptr-1)->voltage), eeprom_read_byte(&(ptr-1)->dist), tmp, eeprom_read_byte(&ptr->dist), volt);
	*p_sens = distance >= SENS_IR_MAX_DIST/5 ? SENS_IR_INFINITE : distance * 5;	// Distanz ist gefuenftelt in den Ref.-Daten;
	
	/* Sensorupdate-Info toggeln */
	*p_toggle = ~*p_toggle;
}

/*! 
 * Kuemmert sich um die Weiterverarbeitung der rohen Sensordaten 
 */
void sensor_update(void){
	static uint8_t old_pos=0;			/*!< Ticks fuer Positionsberechnungsschleife */
	static uint16 old_speed=0;			/*!< Ticks fuer Geschwindigkeitsberechnungsschleife */
	#ifdef MEASURE_MOUSE_AVAILABLE
		static int16 lastMouseX=0;		/*!< letzter Mauswert X fuer Positionsberechnung */
		static int16 lastMouseY=0;		/*!< letzter Mauswert Y fuer Positionsberechnung */
		static float lastDistance=0;	/*!< letzte gefahrene Strecke */
		static float lastHead=HEADING_START;		/*!< letzter gedrehter Winkel */
		static float oldHead=HEADING_START;		/*!< Winkel aus dem letzten Durchgang */
		static float old_x=0;			/*!< Position X aus dem letzten Durchgang */
		static float old_y=0;			/*!< Position Y aus dem letzten Durchgang */
		float radius=0;				/*!< errechneter Radius des Drehkreises */
		float s1=0;					/*!< Steigung der Achsengerade aus dem letzten Durchgang */
		float s2=0;					/*!< Steigung der aktuellen Achsengerade */
		float a1=0;					/*!< Y-Achsenabschnitt der Achsengerade aus dem letzten Durchgang */
		float a2=0;					/*!< Y-Achsenabschnitt der aktuellen Achsengerade */
		float xd=0;					/*!< X-Koordinate Drehpunkt */
		float yd=0;					/*!< Y-Koordinate Drehpunkt */
		float right_radius=0;			/*!< Radius des Drehkreises des rechten Rads */
		float left_radius=0;			/*!< Radius des Drehkreises des linken Rads */
	#endif	// MEASURE_MOUSE_AVAILABLE
	static int16 lastEncL =0;		/*!< letzter Encoderwert links fuer Positionsberechnung */
	static int16 lastEncR =0;		/*!< letzter Encoderwert rechts fuer Positionsberechnung */
	static int16 lastEncL1=0;		/*!< letzter Encoderwert links fuer Geschwindigkeitsberechnung */
	static int16 lastEncR1=0;		/*!< letzter Encoderwert rechts fuer Geschwindigkeitsberechnung */
	float dHead=0;					/*!< Winkeldifferenz aus Encodern */
	float deltaY=0;				/*!< errechneter Betrag Richtungsvektor aus Encodern */
	int16 diffEncL;					/*!< Differenzbildung linker Encoder */
	int16 diffEncR;					/*!< Differenzbildung rechter Encoder */
	float sl;						/*!< gefahrene Strecke linkes Rad */
	float sr;						/*!< gefahrene Strecke rechtes Rad */
	#ifdef MEASURE_MOUSE_AVAILABLE
		int16 dX;						/*!< Differenz der X-Mauswerte */
		int16 dY;						/*!< Differenz der Y-Mauswerte */
		int8 modifiedAngles=False;		/*!< Wird True, wenn aufgrund 90 Grad oder 270 Grad die Winkel veraendert werden mussten */
	
		sensMouseY += sensMouseDY;		/*!< Mausdelta Y aufaddieren */
		sensMouseX += sensMouseDX;		/*!< Mausdelta X aufaddieren */
	#endif	// MEASURE_MOUSE_AVAILABLE
	
	if (timer_ms_passed(&old_pos, 10)) {
		/* Gefahrene Boegen aus Encodern berechnen */
		#ifdef MCU
			uint8 sreg = SREG;
			cli();
		#endif
		/* <CS> */
		register int16 sensEncL_tmp = sensEncL;
		register int16 sensEncR_tmp = sensEncR;
		/* </CS> */
		#ifdef MCU
			SREG = sreg;
		#endif
		diffEncL=sensEncL_tmp-lastEncL;
		diffEncR=sensEncR_tmp-lastEncR;
		lastEncL=sensEncL_tmp;
		lastEncR=sensEncR_tmp;
		sl=(float)diffEncL*((float)WHEEL_PERIMETER/ENCODER_MARKS);
		sr=(float)diffEncR*((float)WHEEL_PERIMETER/ENCODER_MARKS);
		/* Winkel berechnen */
		dHead=(float)(sr-sl)/(WHEEL_TO_WHEEL_DIAMETER);
		/* Winkel ist hier noch im Bogenmass */
		/* Position berechnen */
		/* dazu Betrag des Vektors berechnen */
		if (dHead==0) {
			/* Geradeausfahrt, deltaY=diffEncL=diffEncR */
			deltaY=sl;
		} else {
			/* Laenge berechnen aus alpha/2 */
			deltaY=(sl+sr)*sin(dHead/2)/dHead;
		}
		/* Winkel in Grad umrechnen */
		dHead=dHead/DEG2RAD;
		
		/* neue Positionen berechnen */
		heading_enc+=dHead;
		while (heading_enc>=360) heading_enc=heading_enc-360;
		while (heading_enc<0) heading_enc=heading_enc+360;
		
		x_enc+=(float)deltaY*cos(heading_enc*DEG2RAD);
		y_enc+=(float)deltaY*sin(heading_enc*DEG2RAD);	
		#ifdef MEASURE_MOUSE_AVAILABLE
			dX=sensMouseX-lastMouseX;
			/* heading berechnen */
			dHead=(float)dX*(360.0/(float)MOUSE_FULL_TURN);
			heading_mou+=dHead;
			lastHead+=dHead;
			if (heading_mou>=360) heading_mou=heading_mou-360;
			if (heading_mou<0) heading_mou=heading_mou+360;
			/* x/y pos berechnen */
			dY=sensMouseY-lastMouseY;
			lastDistance+=dY*(25.4/MOUSE_CPI);
			x_mou+=(float)dY*cos(heading_mou*DEG2RAD)*(25.4/MOUSE_CPI);
			y_mou+=(float)dY*sin(heading_mou*DEG2RAD)*(25.4/MOUSE_CPI);

			lastMouseX=sensMouseX;
			lastMouseY=sensMouseY;
		#endif	// MEASURE_MOUSE_AVAILABLE
		#ifdef MEASURE_COUPLED_AVAILABLE
			/* Werte der Encoder und der Maus mit dem Faktor G_POS verrechnen */
			x_pos=G_POS*x_mou+(1-G_POS)*x_enc;
			y_pos=G_POS*y_mou+(1-G_POS)*y_enc;
			/* Korrektur, falls mou und enc zu unterschiedlichen Seiten zeigen */
			if (fabs(heading_mou-heading_enc) > 180) {
				/* wir nutzen zum Rechnen zwei Drehrichtungen */
				heading = heading_mou<=180 ? heading_mou*G_POS : (heading_mou-360)*G_POS;
				heading += heading_enc<=180 ? heading_enc*(1-G_POS) : (heading_enc-360)*(1-G_POS);
				/* wieder auf eine Drehrichtung zurueck */
				if (heading < 0) heading += 360;					
			} else 
				heading = G_POS*heading_mou + (1-G_POS)*heading_enc;
			if (heading >= 360) heading -= 360;
		#else
			#ifdef MEASURE_MOUSE_AVAILABLE
				/* Mauswerte als Standardwerte benutzen */
				heading=heading_mou;
				x_pos=x_mou;
				y_pos=y_mou;
			#else
				/* Encoderwerte als Standardwerte benutzen */
				#ifndef CMPS03_AVAILABLE
					heading=heading_enc;
				#endif
				x_pos=x_enc;
				y_pos=y_enc;
			#endif	// MEASURE_MOUSE_AVAILABLE
		#endif	// MEASURE_COUPLED_AVAILABLE
	}
	if (timer_ms_passed(&old_speed, 250)) {
		#ifdef MCU
			uint8 sreg = SREG;
			cli();
		#endif
		/* <CS> */
		register int16 sensEncL_tmp = sensEncL;
		register int16 sensEncR_tmp = sensEncR;
		/* </CS> */
		#ifdef MCU
			SREG = sreg;
		#endif
		v_enc_left=  (((sensEncL_tmp - lastEncL1) * (float)WHEEL_PERIMETER) / ENCODER_MARKS)*4;
		v_enc_right= (((sensEncR_tmp - lastEncR1) * (float)WHEEL_PERIMETER) / ENCODER_MARKS)*4;
		v_enc_center=(v_enc_left+v_enc_right)/2;
		lastEncL1= sensEncL_tmp;
		lastEncR1= sensEncR_tmp;
		#ifdef MEASURE_MOUSE_AVAILABLE
			/* Speed aufgrund Maussensormessungen */
			v_mou_center=lastDistance*4;
			/* Aufteilung auf die Raeder zum Vergleich mit den Radencodern */
			/* Sonderfaelle pruefen */
			if (oldHead==90 || oldHead==270 || heading_mou==90 || heading_mou==270) {
				float temp;
				/* winkel um 90 Grad vergroessern */
				oldHead+=90;
				heading_mou+=90;
				// Koordinaten anpassen
				temp=old_x;
				old_x=old_y;
				old_y=-temp;
				temp=x_mou;
				x_mou=y_mou;
				y_mou=-temp;
				modifiedAngles=True;
			} 
			
			/* Steigungen berechnen */
			s1=-tan(oldHead*DEG2RAD);
			s2=-tan(heading_mou*DEG2RAD);
			
			/* Geradeausfahrt? (s1==s2) */
			if (s1==s2) {
				/* Bei Geradeausfahrt ist v_left==v_right==v_center */
				v_mou_left=v_mou_right=v_mou_center;
			} else {
				/* y-Achsenabschnitte berechnen */
				a1=old_x-s1*old_y;
				a2=x_mou-s2*y_mou;
				/* Schnittpunkt berechnen */
				yd=(a2-a1)/(s1-s2);	
				xd=s2*yd+a2;
				/* Radius ermitteln */
				radius=sqrt((x_mou-xd)*(x_mou-xd)+(y_mou-yd)*(y_mou-yd));
				/* Vorzeichen des Radius feststellen */
				if (lastHead<0) {
					/* Drehung rechts, Drehpunkt liegt rechts vom Mittelpunkt
					 * daher negativer Radius */
					 radius=-radius;
				}
				if (v_mou_center<0) {
					/* rueckwaerts => links und rechts vertauscht, daher VZ vom Radius umdrehen */
					radius=-radius;
				}
				/* Geschwindigkeiten berechnen */
				right_radius=radius+WHEEL_DISTANCE;
				left_radius=radius-WHEEL_DISTANCE;
				v_mou_right=lastHead*right_radius*(4*2*M_PI/360);
				v_mou_left=lastHead*left_radius*(4*2*M_PI/360);
			}
			/* Falls Koordinaten/Winkel angepasst wurden, nun wieder korrigieren */
			if (modifiedAngles){
				float temp;
				/* Winkel wieder um 90 Grad reduzieren */
				oldHead-=90;
				heading_mou-=90;
				/* Koordinaten wieder korrigieren */
				temp=old_x;
				old_x=-old_y;
				old_y=temp;
				temp=x_mou;
				x_mou=-y_mou;
				y_mou=temp;	
			}
			lastDistance=0;
			lastHead=0;
			old_x=x_mou;
			old_y=y_mou;
			oldHead=heading_mou;
		#endif	// MEASURE_MOUSE_AVAILABLE
		#ifdef MEASURE_COUPLED_AVAILABLE
			v_left=G_SPEED*v_mou_left+(1-G_SPEED)*v_enc_left;
			v_right=G_SPEED*v_mou_right+(1-G_SPEED)*v_enc_left;
			v_center=G_SPEED*v_mou_center+(1-G_SPEED)*v_enc_center;
		#else
			#ifdef MEASURE_MOUSE_AVAILABLE
				/* Mauswerte als Standardwerte benutzen */
				v_left=v_mou_left;
				v_right=v_mou_right;
				v_center=v_mou_center;
			#else
				/* Encoderwerte als Standardwerte benutzen */
				v_left=v_enc_left;
				v_right=v_enc_right;
				v_center=v_enc_center;
			#endif	// MEASURE_MOUSE_AVAILABLE
		#endif	// MEASURE_COUPLED_AVAILABLE
		#ifdef SRF10_AVAILABLE
			sensSRF10 = srf10_get_measure();	/*!< Messung Ultraschallsensor */
		#endif	// SRF10_AVAILABLE
	}
}

/*!
 * Die Funktion gibt aus, ob sich innerhalb einer gewissen Entfernung ein Objekt-Hindernis befindet.
 * @param distance	Entfernung in mm, bis zu welcher ein Objekt gesichtet wird. 
 * @return 			Gibt False (0) zurueck, wenn kein Objekt innerhalb von distance gesichtet wird. Ansonsten die Differenz 
 * zwischen dem linken und rechten Sensor. Negative Werte besagen, dass das Objekt naeher am linken, positive, dass 
 * es naeher am rechten Sensor ist. Sollten beide Sensoren den gleichen Wert haben, gibt die Funktion 1 zurueck, um
 * von False unterscheiden zu koennen. 
 */
int16 is_obstacle_ahead(int16 distance) {
	if (sensDistL > distance && sensDistR > distance) 
		return False;
	if (sensDistL - sensDistR == 0) 
		return 1;
	return sensDistL - sensDistR;
}

#ifdef SENSOR_DISPLAY_AVAILABLE
	/*!
	 * @brief	Displayhandler fuer Sensoranzeige
	 */
	void sensor_display(void){
		display_cursor(1,1);
		display_printf("P=%03X %03X D=%03d %03d ",sensLDRL,sensLDRR,sensDistL,sensDistR);
		
		display_cursor(2,1);
		display_printf("B=%03X %03X L=%03X %03X ",sensBorderL,sensBorderR,sensLineL,sensLineR);
		
		display_cursor(3,1);
		display_printf("R=%2d %2d F=%d K=%d T=%d ",sensEncL%10,sensEncR%10,sensError,sensDoor,sensTrans);
		
		display_cursor(4,1);
		#ifdef RC5_AVAILABLE
			static uint16 RC5_old;
			if (RC5_Code != 0) RC5_old = RC5_Code;
			#ifdef MAUS_AVAILABLE
				display_printf("I=%04X M=%05d %05d",RC5_old,sensMouseX,sensMouseY);
			#else
				display_printf("I=%04X",RC5_old);
			#endif				
		#else
			#ifdef MAUS_AVAILABLE
				display_printf("M=%05d %05d",sensMouseX,sensMouseY);
			#endif
		#endif	
	}
#endif	// SENSOR_DISPLAY_AVAILABLE

#ifdef DISPLAY_ODOMETRIC_INFO
	/*!
	 * @brief	Displayhandler fuer Odometrieanzeige
	 */
	void odometric_display(void){
		/* Zeige Positions- und Geschwindigkeitsdaten */
		display_cursor(1,1);
		display_printf("heading: %3d.%u  ",(int16)heading, (int16)((heading-(int16)heading)*10));
		display_cursor(2,1);
		display_printf("x: %3d  y: %3d  ",(int16)x_pos,(int16)y_pos);
		display_cursor(3,1);
		display_printf("v_l: %3d v_r: %3d  ",(int16)v_left,(int16)v_right);						
		#ifdef MEASURE_MOUSE_AVAILABLE
			display_cursor(4,1);
			display_printf("squal: %3d v_c: %3d",maus_get_squal(),(int16)v_mou_center);
		#endif
	}
#endif	// DISPLAY_ODOMETRIC_INFO

#ifdef TEST_AVAILABLE
	/*! 
	 * Zeigt den internen Status der Sensoren mit den LEDs an 
	 */
	void show_sensors_on_led(void) {
		static volatile uint8_t led_status = 0x00;
		led_t * status = (led_t *)&led_status;
		#ifdef TEST_AVAILABLE_ANALOG
			(*status).rechts	= (sensDistR >> 8) & 0x01;
			(*status).links		= (sensDistL >> 8) & 0x01;
			(*status).rot		= (sensLineL >> 9) & 0x01;
			(*status).orange	= (sensLineR >> 9) & 0x01;
			(*status).gelb		= (sensLDRL >> 8)  & 0x01;
			(*status).gruen		= (sensLDRR >> 8)  & 0x01;
			(*status).tuerkis	= (sensBorderL >> 9) & 0x01;
			(*status).weiss		= (sensBorderR >> 9) & 0x01;
		#endif	// TEST_AVAILABLE_ANALOG
		#ifdef TEST_AVAILABLE_DIGITAL
			(*status).rechts	= sensEncR  & 0x01;
			(*status).links		= sensEncL  & 0x01;
			(*status).rot		= sensTrans & 0x01;
			(*status).orange	= sensError & 0x01;
			(*status).gelb		= sensDoor  & 0x01;
			#ifdef MAUS_AVAILABLE
				(*status).gruen		= (sensMouseDX >> 1) & 0x01;
				(*status).tuerkis	= (sensMouseDY >> 1) & 0x01;
			#endif
			#ifdef RC5_AVAILABLE
				(*status).weiss		= RC5_Code & 0x01;
			#endif
		#endif	// TEST_AVAILABLE_DIGITAL
				
		LED_set(led_status);
	}
#endif	// TEST_AVAILABLE
