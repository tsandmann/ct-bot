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

/*! @file 	motor.c
 * @brief 	High-Level-Routinen fuer die Motorsteuerung des c't-Bot
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	15.01.05
*/

#include <stdlib.h>
#include "global.h"
#include "ct-Bot.h"
#include "motor.h"
#include "bot-local.h"
#include "motor-low.h"
#include "timer.h"
#include "sensor.h"
#include "display.h"

volatile int16 speed_l=0;	/*!< Geschwindigkeit linker Motor */
volatile int16 speed_r=0;	/*!< Geschwindigkeit rechter Motor */

direction_t direction;		/*!< Drehrichtung der Motoren */

#define PWMMAX 255   /*!< Maximaler PWM-Wert */
#define PWMMIN 0     /*!< Minimaler PWM-Wert */

#ifdef SPEED_CONTROL_AVAILABLE
	volatile int8 encoderTargetRateL=0;
	volatile int8 encoderTargetRateR=0;

	/*!
	 * @brief 	Drehzahlregelung fuer die Motoren des c't-Bots
	 * @author 	Benjamin Benz (bbe@heise.de
	 * @date 	01.05.06
	 * Getrennte Drehzahlregelung fuer linken und rechten Motor sorgt fuer konstante Drehzahl und somit annaehernd
	 * fuer Geradeauslauf
	 * Feintuning von Kp, Ki, Kd verbessert die Genauigkeit und Schnelligkeit der Regelung
	 * Querkopplung der Motoren verbessert Gleichlauf, beispielsweise x*(sensEncL - sensEncR) 
	 * in jeden Regler einbauen
	*/
	void speed_control(void){
		int8 Kp=0;
		int8 Ki=0;
	
		int16 StellwertL=motor_left;  /*!< Stellwert links*/
		int16 StellwertR=motor_right;  /*!< Stellwert rechts*/
		
		volatile static int16 lastEncoderL=0;      /*!< vorhergehender Wert von sensEncL */	
		volatile static int8 lastErrL=0;   /*!< letzter Drehzahlfehler links */
		volatile static int8 last2ErrL=0;  /*!< vorletzter Drehzahlfehler links */
	
		volatile static int16 lastEncoderR=0;      /*!< vorhergehender Wert von sensEncR */	
		volatile static int8 lastErrR=0;   /*!< letzter Drehzahlfehler rechts */
		volatile static int8 last2ErrR=0;  /*!< vorletzter Drehzahlfehler rechts */
	
		int16 err=0;    	        // aktuelle Abweichung vom Soll-Wert
		int16 encoderRate=0;		// IST-Wert [Encoder-ticks/Aufruf]
	
	
		// Wir arbeiten mit verschiedenen PID-Parametern fuer verschiedene Geschwindigkeitsabschnitte
		if (encoderTargetRateL <= PID_LOW_RATE){
			Kp=PID_LOW_Kp;
			Ki=PID_LOW_Ki;
		} else {
			if (encoderTargetRateL >= PID_HIGH_RATE) {
				Kp=PID_HIGH_Kp;
				Ki=PID_HIGH_Ki;
			} else {
				Kp=(PID_HIGH_Kp+PID_LOW_Kp)/2;
				Ki=(PID_HIGH_Ki+PID_LOW_Ki)/2;
			}
		}
	
	
	
		//Regler links  
		if (encoderTargetRateL == 0){
			StellwertL=0;
			err=0; lastErrL = 0;
		} else {
			encoderRate = sensEncL-lastEncoderL;  	 // aktuelle Ist-Wert berechnen [Encoder-ticks/aufruf]
			lastEncoderL = sensEncL;   				 // Anzahl der Encoderpulse merken fuer naechsten Aufruf merken
			err = encoderTargetRateL - encoderRate;  // Regelabweichung links
	
			// Stellwert Berechnen
			StellwertL +=  (Kp * (err - lastErrL)) / 10;				// P-Beitrag
			StellwertL +=  (Ki * (err + lastErrL)/2 ) /10;			// I-Beitrag
		//	StellwertL +=  Kd * (errL - 2 * lastErrL + last2ErrL);	// D-Beitrag
			
			//berechneten Stellwert auf zulaessige Groesse begrenzen  
			if (StellwertL >  PWMMAX)	StellwertL =  PWMMAX;         
			if (StellwertL < -PWMMAX)	StellwertL = -PWMMAX; 
		
			#ifdef DISPLAY_REGELUNG_AVAILABLE
				if (display_screen==DISPLAY_REGELUNG_AVAILABLE){
					display_cursor(1,1);
					display_printf("%03d/%03d ",encoderRate,encoderTargetRateL);
					display_cursor(2,1);
					display_printf("e =%03d ",err);
					display_cursor(3,1);	
					display_printf("L =%04d ", StellwertL);
				}
			#endif
		}
	
		last2ErrL = lastErrL;              // alten N-2 Fehler merken
		lastErrL = err;                   // alten N-1 Fehler merken
	
	
		//Regler rechts  
		if (encoderTargetRateR == 0){
			StellwertR=0;
			err=0; lastErrR=0;
		} else {
			encoderRate = sensEncR-lastEncoderR;  	// aktuelle Ist-Wert berechnen [Encoder-ticks/aufruf]
			lastEncoderR = sensEncR;   				// Anzahl der Encoderpulse merken fuer naechsten Aufruf merken
			err = encoderTargetRateR - encoderRate;  // Regelabweichung links
			  
			// Stellwert Berechnen
			StellwertR +=  (Kp * (err - lastErrR))/10;					// P-Beitrag
			StellwertR +=  (Ki * (err + lastErrR)/2)/10;					// I-Beitrag
		//	StellwertR +=  Kd * (err - 2 * lastErrR + last2ErrR);	// D-Beitrag
			
			//berechneten Stellwert auf zulaessige Groesse begrenzen  
			if (StellwertR >  PWMMAX) StellwertR =  PWMMAX;         
			if (StellwertR < -PWMMAX) StellwertR = -PWMMAX;
	
			#ifdef DISPLAY_REGELUNG_AVAILABLE
				if (display_screen==DISPLAY_REGELUNG_AVAILABLE){
					display_cursor(1,10);
					display_printf("%03d/%03d ",encoderRate,encoderTargetRateR);
					display_cursor(2,10);
					display_printf("e =%03d ",err);
					display_cursor(3,10);	
					display_printf("R =%04d ", StellwertR);	
				}
			#endif
		}
	
		last2ErrR = lastErrR;              // alten N-2 Fehler merken
		lastErrR = err;                   // alten N-1 Fehler merken
	
		#ifdef DISPLAY_REGELUNG_AVAILABLE
			if (display_screen==DISPLAY_REGELUNG_AVAILABLE){
				display_cursor(4,1);	
				display_printf("Kp=%03d Ki=%03d", Kp, Ki);	
			}
		#endif
	
		// Und nun den Wert setzen
		bot_motor(StellwertL,StellwertR);
	}
#endif

/*!
 * Direkter Zugriff auf den Motor
 * @param left	Geschwindigkeit fuer den linken Motor
 * @param right Geschwindigkeit fuer den linken Motor
 * Geschwindigkeit liegt zwischen -255 und +255.
 * 0 bedeutet Stillstand, 255 volle Kraft voraus, -255 volle Kraft zurueck.
 * Sinnvoll ist die Verwendung der Konstanten: BOT_SPEED_XXX, 
 * also z.B. motor_set(BOT_SPEED_LOW,-BOT_SPEED_LOW);
 * fuer eine langsame Drehung
*/
void motor_set(int16 left, int16 right){
	volatile static int16 old_mot_time_s=0;
	volatile static int16 old_mot_time_ms=0;

	if (left == BOT_SPEED_IGNORE)	
		left=BOT_SPEED_STOP;

	if (right == BOT_SPEED_IGNORE)	
		right=BOT_SPEED_STOP;

	
	// Haben wir ueberhaupt etwas zu tun?
	if ((speed_l == left) && (speed_r == right)){
		// Hier sitzt die eigentliche Regelung
		#ifdef SPEED_CONTROL_AVAILABLE
			if (timer_get_ms_since(old_mot_time_s,old_mot_time_ms) > SPEED_CONTROL_INTERVAL) {
				speed_control();
				old_mot_time_s= timer_get_s();
				old_mot_time_ms= timer_get_ms();
			}
		#endif
		return;		// Keine Aenderung? Dann zuerueck
	}
		
	if (abs(left) > BOT_SPEED_MAX)	// Nicht schneller fahren als moeglich
		speed_l = BOT_SPEED_MAX;
	else if (left == 0)				// Stop wird nicht veraendert
		speed_l = BOT_SPEED_STOP;
	else if (abs(left) < BOT_SPEED_SLOW)	// Nicht langsamer als die 
		speed_l = BOT_SPEED_SLOW;	// Motoren koennen
	else				// Sonst den Wunsch uebernehmen
		speed_l = abs(left);

	if (abs(right) > BOT_SPEED_MAX)// Nicht schneller fahren als moeglich
		speed_r = BOT_SPEED_MAX;
	else if (abs(right) == 0)	// Stop wird nicht veraendert
		speed_r = BOT_SPEED_STOP;
	else if (abs(right) < BOT_SPEED_SLOW)	// Nicht langsamer als die 
		speed_r = BOT_SPEED_SLOW;	// Motoren koennen
	else				// Sonst den Wunsch uebernehmen
		speed_r = abs(right);
	
	if (left < 0 )
		speed_l=-speed_l;
	
	if (right < 0 )	
		speed_r=-speed_r;
		
	#ifdef SPEED_CONTROL_AVAILABLE
		// TODO Hier koennten wir die Motorkennlinie heranziehen um gute Einstiegswerte fuer die Regelung zu haben
		// Evtl. sogar eine im EEPROM kalibrierbare Tabelle??
		encoderTargetRateL = left / SPEED_TO_ENCODER_RATE;	// Geschwindigkeit [mm/s] umrechnen in [EncoderTicks/Aufruf]
		encoderTargetRateR = right / SPEED_TO_ENCODER_RATE;	// Geschwindigkeit [mm/s] umrechnen in [EncoderTicks/Aufruf]
	#endif
	
	// Zuerst einmal eine lineare Kennlinie annehmen
	bot_motor(speed_l/2,speed_r/2);
	
}

/*!
 * Stellt die Servos
 * Sinnvolle Werte liegen zwischen 8 und 16
 * @param servo Nummer des Servos
 * @param servo Zielwert
 */
void servo_set(uint8 servo, uint8 pos){
	if (pos< SERVO_LEFT)
		pos=SERVO_LEFT;
	if (pos> SERVO_RIGHT)
		pos=SERVO_RIGHT;
		
	bot_servo(servo,pos);
}

/*!
 * Initialisiere den Motorkrams
 */
void motor_init(void){
	speed_l=0;
	speed_r=0;
	motor_low_init();
}


