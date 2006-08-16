/*
 * c't-Sim - Robotersimulator fuer den c't-Bot
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

/*! @file 	sensor.c  
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
#ifdef SRF10_AVAILABLE
	#include "srf10.h"
#endif

// Defines einiger, haeufiger benoetigter Konstanten
#define DEG2RAD (2*M_PI/360)


volatile int16 sensLDRL=0;		/*!< Lichtsensor links */
volatile int16 sensLDRR=0;		/*!< Lichtsensor rechts */

volatile int16 sensDistL=1023;		/*!< Distanz linker IR-Sensor in [mm], wenn korrekt umgerechnet wird */
volatile int16 sensDistR=1023;		/*!< Distanz rechter IR-Sensor in [mm], wenn korrekt umgerechnet wird */

volatile int16 sensBorderL=0;	/*!< Abgrundsensor links */
volatile int16 sensBorderR=0;	/*!< Abgrundsensor rechts */

volatile int16 sensLineL=0;	/*!< Lininensensor links */
volatile int16 sensLineR=0;	/*!< Lininensensor rechts */

volatile uint8 sensTrans=0;		/*!< Sensor Ueberwachung Transportfach */

volatile uint8 sensDoor=0;		/*!< Sensor Ueberwachung Klappe */

volatile uint8 sensError=0;		/*!< Ueberwachung Motor oder Batteriefehler */

volatile int8 sensMouseDX;		/*!< Maussensor Delta X, positive Werte zeigen querab der Fahrtrichtung nach rechts */
volatile int8 sensMouseDY;		/*!< Maussensor Delta Y, positive Werte zeigen in Fahrtrichtung */

volatile int16 sensMouseX;		/*!< Mausposition X, positive Werte zeigen querab der Fahrtrichtung nach rechts */
volatile int16 sensMouseY;		/*!< Mausposition Y, positive Werte zeigen in Fahrtrichtung  */

volatile int16 sensEncL=0;		/*!< Encoder linkes Rad */
volatile int16 sensEncR=0;		/*!< Encoder rechtes Rad */
volatile float heading_enc=0;	/*!< Blickrichtung aus Encodern */
volatile float x_enc=0;		/*!< X-Koordinate aus Encodern [mm] */
volatile float y_enc=0;		/*!< Y-Koordinate aus Encodern [mm] */
volatile float v_enc_left=0;	/*!< Abrollgeschwindigkeit des linken Rades in [mm/s] [-128 bis 127] relaisitisch [-50 bis 50] */
volatile float v_enc_right=0;	/*!< Abrollgeschwindigkeit des linken Rades in [mm/s] [-128 bis 127] relaisitisch [-50 bis 50] */
volatile float v_enc_center=0;	/*!< Schnittgeschwindigkeit ueber beide Raeder */

#ifdef PC
	volatile int16 simultime=0;	/*! Simulierte Zeit */
#endif

#ifdef MEASURE_MOUSE_AVAILABLE
	volatile float heading_mou=0;		/*!< Aktuelle Blickrichtung relativ zur Startposition aus Mausmessungen */
	volatile float x_mou=0;			/*!< Aktuelle X-Koordinate in mm relativ zur Startposition aus Mausmessungen */
	volatile float y_mou=0;			/*!< Aktuelle Y-Koordinate in mm relativ zur Startposition aus Mausmessungen */
	volatile float v_mou_center=0;		/*!< Geschwindigkeit in mm/s ausschliesslich aus den Maussensorwerten berechnet */
	volatile float v_mou_left=0;		/*!< ...aufgeteilt auf linkes Rad */
	volatile float v_mou_right=0;		/*!< ...aufgeteilt auf rechtes Rad */
#endif


#ifdef MEASURE_COUPLED_AVAILABLE
	volatile float heading=0;			/*!< Aktuelle Blickrichtung aus gekoppelten Werten */
	volatile float x_pos=0;			/*!< Aktuelle X-Position aus gekoppelten Werten */
	volatile float y_pos=0;			/*!< Aktuelle Y-Position aus gekoppelten Werten */
	volatile float v_left=0;			/*!< Geschwindigkeit linkes Rad aus gekoppelten Werten */
	volatile float v_right=0;			/*!< Geschwindigkeit rechtes Rad aus gekoppelten Werten */
	volatile float v_center=0;			/*!< Geschwindigkeit im Zentrum des Bots aus gekoppelten Werten */
#endif


volatile int8 sensors_initialized = 0;	/*!< Wird 1 sobald die Sensorwerte zur Verfügung stehen */

#ifdef SRF10_AVAILABLE
	volatile uint16 sensSRF10;	/*!< Messergebniss Ultraschallsensor */
#endif


/*! Linearisiert die Sensorwerte
 * @param left Linker Rohwert [0-1023]
 * @param right Rechter Rohwert [0-1023]
 */
void sensor_abstand(uint16 left, uint16 right){
	sensDistL = SENSDISTSLOPELEFT / (left - SENSDISTOFFSETLEFT);
	sensDistR = SENSDISTSLOPERIGHT / (right - SENSDISTOFFSETRIGHT);
	
			// Korrigieren, wenn ungueltiger Wert
	if (sensDistL > SENS_IR_MAX_DIST || sensDistL<0)
		sensDistL=SENS_IR_INFINITE;

	if (sensDistR > SENS_IR_MAX_DIST || sensDistR<0)
		sensDistR=SENS_IR_INFINITE;
	
}

/*! Sensor_update
* Kuemmert sich um die Weiterverarbeitung der rohen Sensordaten 
*/
void sensor_update(void){
	#ifdef TIME_AVAILABLE
		static uint16 oldms_pos=0;	/*!< ms-Wert fuer Positionsberechnungsschleife */
		static uint16 olds_pos=0;	/*!< s-Wert fuer Positionsberechnungsschleife */
		static uint16 oldms_speed=0;/*!< ms-Wert fuer Geschwindigkeitsberechnungsschleife */
		static uint16 olds_speed=0; /*!< s-Wert fuer Geschwindigkeitsberechnungsschleife */	
		#ifdef MEASURE_MOUSE_AVAILABLE
				static int16 lastMouseX=0;		/*!< letzter Mauswert X fuer Positionsberechnung */
				static int16 lastMouseY=0;		/*!< letzter Mauswert Y fuer Positionsberechnung */
				static float lastDistance=0;	/*!< letzte gefahrene Strecke */
				static float lastHead=0;		/*!< letzter gedrehter Winkel */
				static float oldHead=0;		/*!< Winkel aus dem letzten Durchgang */
				static float old_x=0;			/*!< Position X aus dem letzten Durchgang */
				static float old_y=0;			/*!< Position Y aus dem letzten Durchgang */
				float radius=0;				/*!< errechneter Radius des Drehkreises */
				float s1=0;					/*!< Steigung der Achsengerade aus dem letzten Durchgang */
				float s2=0;					/*!< Steigung der aktuellen Achsengerade */
				float a1=0;					/*!< Y-Achsenabschnitt der Achsengerade aus dem letzten Durchgang */
				float a2=0;					/*!< Y-Achsenabschnitt der aktuellen Achsengerade */
				float xd=0;					/*!< X-Koordinate Drehpunkt */
				float yd=0;					/*!< Y-Koordinate Drehpunkt */
//				float dPos=0;					/*!< aktuelle Positionsaenderung  */
				float right_radius=0;			/*!< Radius des Drehkreises des rechten Rads */
				float left_radius=0;			/*!< Radius des Drehkreises des linken Rads */
		#endif
	#endif
	static int16 lastEncL =0;		/*!< letzter Encoderwert links fuer Positionsberechnung */
	static int16 lastEncR =0;		/*!< letzter Encoderwert rechts fuer Positionsberechnung */
	static int16 lastEncL1=0;		/*!< letzter Encoderwert links fuer Geschwindigkeitsberechnung */
	static int16 lastEncR1=0;		/*!< letzter Encoderwert rechts fuer Geschwindigkeitsberechnung */
	static int8 mouseOverflowX=False;		/*!< Es gab einen Ueberlauf von sensMouseX */
	static int8 mouseOverflowY=False;		/*!< Es gab einen Ueberlauf von sensMouseY */
	float dHead=0;					/*!< Winkeldifferenz aus Encodern */
	float deltaY=0;				/*!< errechneter Betrag Richtungsvektor aus Encodern */
	int16 diffEncL;					/*!< Differenzbildung linker Encoder */
	int16 diffEncR;					/*!< Differenzbildung rechter Encoder */
	float sl;						/*!< gefahrene Strecke linkes Rad */
	float sr;						/*!< gefahrene Strecke rechtes Rad */
	float dX;						/*!< Differenz der X-Mauswerte */
	float dY;						/*!< Differenz der Y-Mauswerte */
	int8 modifiedAngles=False;		/*!< Wird True, wenn aufgrund 90� oder 270� die Winkel veraendert werden mussten */
	
	if (sensMouseY>0 && sensMouseDY>0 && (int16)(sensMouseY+sensMouseDY)<0) {
		/* Beide Positiv. Bei negativem Ergebnis gab es einen Ueberlauf */
		mouseOverflowY=True;
	} else if (sensMouseY<0 && sensMouseDY<0 && (int16)(sensMouseY+sensMouseDY)>0) {
		/* Beide Negativ. Bei einem positiven Ergebnis gab es einen Ueberlauf */
		mouseOverflowY=True;
	}
	if (sensMouseX>0 && sensMouseDX>0 && (int16)(sensMouseX+sensMouseDX)<0) {
		/* Beide Positiv. Bei negativem Ergebnis gab es einen Ueberlauf */
		mouseOverflowX=True;
	} else if (sensMouseX<0 && sensMouseDX<0 && (int16)(sensMouseX+sensMouseDX)>0) {
		/* Beide Negativ. Bei einem positiven Ergebnis gab es einen Ueberlauf */
		mouseOverflowX=True;
	}
	sensMouseY += sensMouseDY;		/*!< Mausdelta Y aufaddieren */
	sensMouseX += sensMouseDX;		/*!< Mausdelta X aufaddieren */
	
	#ifdef TIME_AVAILABLE
	if (timer_get_ms_since(olds_pos,oldms_pos)>50) {
		olds_pos=timer_get_s();
		oldms_pos=timer_get_ms();
		/* Gefahrene Boegen aus Encodern berechnen */
		diffEncL=sensEncL-lastEncL;
		diffEncR=sensEncR-lastEncR;
		lastEncL=sensEncL;
		lastEncR=sensEncR;
		sl=(float)diffEncL*WHEEL_PERIMETER/ENCODER_MARKS;
		sr=(float)diffEncR*WHEEL_PERIMETER/ENCODER_MARKS;
		/* Winkel berechnen */
		dHead=(float)(sr-sl)/(2*WHEEL_DISTANCE);
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
		while (heading_enc>359) heading_enc=heading_enc-360;
		while (heading_enc<0) heading_enc=heading_enc+360;
		
		x_enc+=(float)deltaY*cos(heading_enc*DEG2RAD);
		y_enc+=(float)deltaY*sin(heading_enc*DEG2RAD);	
		#ifdef MEASURE_MOUSE_AVAILABLE
			if (mouseOverflowX) {
				/* es gab einen Overflow, Ergebnis korrigieren */
				if (sensMouseX>0) {
					dX=-32768-lastMouseX-sensMouseX;
				} else {
					dX=32767-lastMouseX-(-32768-sensMouseX);	
				}
				mouseOverflowX=False; /* Flag zuruecksetzen */
			} else {
				dX=sensMouseX-lastMouseX;
			}
			/* heading berechnen */
			dHead=(float)dX*360/MOUSE_FULL_TURN;
			heading_mou+=dHead;
			lastHead+=dHead;
			if (heading_mou>359) heading_mou=heading_mou-360;
			if (heading_mou<0) heading_mou=heading_mou+360;
			/* x/y pos berechnen */
			if (mouseOverflowY) {
				/* es gab einen Overflow, Ergebnis korrigieren */
				if (sensMouseY>0) {
					dY=-32768-lastMouseY-sensMouseY;
				} else {
					dY=32767-lastMouseY-(-32768-sensMouseY);	
				}
				mouseOverflowY=False; /* Flag zuruecksetzen */
			} else {
				dY=sensMouseY-lastMouseY;
			}
			x_mou+=(float)dY*cos(heading_mou*DEG2RAD)*25.4/MOUSE_CPI;
			lastDistance+=dY*25.4/MOUSE_CPI;
			y_mou+=(float)dY*sin(heading_mou*DEG2RAD)*25.4/MOUSE_CPI;
			lastMouseX=sensMouseX;
			lastMouseY=sensMouseY;
		#endif
		#ifdef MEASURE_COUPLED_AVAILABLE
			/* Werte der Encoder und der Maus mit dem Faktor G_POS verrechnen */
			x_pos=G_POS*x_mou+(1-G_POS)*x_enc;
			y_pos=G_POS*y_mou+(1-G_POS)*y_enc;
			heading=G_POS*heading_mou+(1-G_POS)*heading_enc;
		#endif
	}
	if (timer_get_ms_since(olds_speed,oldms_speed)>250) {	// sollte genau 1x pro Sekunde zutreffen
		olds_speed=timer_get_s();
		oldms_speed=timer_get_ms();
		v_enc_left=  (((sensEncL - lastEncL1) * WHEEL_PERIMETER) / ENCODER_MARKS)*4;
		v_enc_right= (((sensEncR - lastEncR1) * WHEEL_PERIMETER) / ENCODER_MARKS)*4;
		v_enc_center=(v_enc_left+v_enc_right)/2;
		lastEncL1= sensEncL;
		lastEncR1= sensEncR;
		#ifdef MEASURE_MOUSE_AVAILABLE
			/* Speed aufgrund Maussensormessungen */
			v_mou_center=lastDistance;
			/* Aufteilung auf die Raeder zum Vergleich mit den Radencodern */
			/* Sonderfaelle pruefen */
			if (oldHead==90 || oldHead==270 || heading_mou==90 || heading_mou==270) {
				float temp;
				/* winkel um 90� vergroessern */
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
			s1=tan(oldHead*DEG2RAD);
			s2=tan(heading_mou*DEG2RAD);
			
			/* Geradeausfahrt? (s1==s2) */
			if (s1==s2) {
				/* Bei Geradeausfahrt ist v_left==v_right==v_center */
				v_mou_left=v_mou_right=v_mou_center;
			} else {
				/* y-Achsenabschnitte berechnen */
				a1=old_y-s1*old_x;
				a2=y_mou-s2*x_mou;
				/* Schnittpunkt berechnen */
				xd=(a2-a1)/(s1-s2);
				yd=(s1*a2-s2*a1)/(s1-s2);
				/* Radius ermitteln */
				radius=sqrt((x_mou-xd)*(x_mou-xd)+(y_mou-yd)*(y_mou-yd));
				/* Vorzeichen des Radius feststelen */
				if (lastHead>0) {
					/* Drehung links, Drehpunkt liegt auf oder links vom Mittelpunkt
					 * daher negativer Radius */
					 radius=-radius;
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
				/* Geschwindigkeiten berechnen */
				right_radius=radius-WHEEL_DISTANCE;
				left_radius=radius+WHEEL_DISTANCE;
				v_mou_right=lastHead/360*2*M_PI*right_radius*4;
				v_mou_left=lastHead/360*2*M_PI*left_radius*4;
			}
			lastDistance=0;
			lastHead=0;
			old_x=x_mou;
			old_y=y_mou;
			oldHead=heading_mou;
		#endif
		#ifdef MEASURE_COUPLED_AVAILABLE
			v_left=G_SPEED*v_mou_left+(1-G_SPEED)*v_enc_left;
			v_right=G_SPEED*v_mou_right+(1-G_SPEED)*v_enc_left;
			v_center=G_SPEED*v_mou_center+(1-G_SPEED)*v_enc_center;
		#endif
		#ifdef SRF10_AVAILABLE
			sensSRF10 = srf10_get_measure();	/*!< Messung Ultraschallsensor */
		#endif	
	}
	#endif
	
	sensors_initialized=1;
}

