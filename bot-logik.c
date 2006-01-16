/*! @file 	bot-logik.c
 * @brief 	High-Level Routinen für die Steuerung des c't-Bots
 * Diese Datei sollte der Einstiegspunkt für eigene Experimente sein, 
 * um den Roboter zu steuern.
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/

#include "ct-Bot.h"
#include "motor.h"
#include "sensor.h"
#include "bot-logik.h"

#include "rc5.h"
#include <stdlib.h>

#define	BORDER_DANGEROUS	500		///< Wert, ab dem wir sicher sind, dass es eine Kante ist


#define	COL_CLOSEST			100		///< Abstand in mm den wir als zu nah betrachten
#define	COL_NEAR			200		///< Nahbereich
#define	COL_FAR				400		///< Fernbereich

#define ZONE_CLOSEST	0			///< Zone für extremen Nahbereich
#define ZONE_NEAR		1			///< Zone für Nahbereich
#define ZONE_FAR		2			///< Zone für Fernbereich
#define ZONE_CLEAR		3			///< Zone für Freien Bereich

#define BRAKE_CLOSEST 	2			///< Bremsfaktor für extrmen Nahbereich ( <1 ==> bremsen >1 ==> rückwärts)
#define BRAKE_NEAR		0.6			///< Bremsfaktor für Nahbereich ( <1 ==> bremsen >1 ==> rückwärts)
#define BRAKE_FAR		0.2			///< Bremsfaktor für Fernbereich ( <1 ==> bremsen >1 ==> rückwärts)

char col_zone_l=ZONE_CLEAR;			///< Kollisionszone in der sich der linke Sensor befindet
char col_zone_r=ZONE_CLEAR;			///< Kollisionszone in der sich der rechte Sensor befindet


#define MOT_GOTO_MAX  3 		///< Richtungsänderungen, bis goto erreicht sein muss

volatile int16 mot_l_goto=0;	///< Speichert wieweit der linke Motor drehen soll
volatile int16 mot_r_goto=0;	///< Speichert wieweit der rechte Motor drehen soll

volatile int16 mot_goto_l=0;	///< Muss der linke motor noch drehen? 
volatile int16 mot_goto_r=0;	///< Muss der rechte motor noch drehen? 

volatile int16 speed_l_col=0;	///< Kollisionsschutz links
volatile int16 speed_r_col=0;	///< Kollisionsschutz links

volatile int16 target_speed_l=0;	///< Sollgeschwindigkeit linker Motor
volatile int16 target_speed_r=0;	///< Sollgeschwindigkeit rechter Motor


/*!
 * Drehe die Räder um die gegebene Zahl an Encoder-Schritten weiter
 * @param left Schritte links
 * @param right Schritte rechts
 */
void bot_goto(int left, int right){
	// Zielwerte Speichern
	mot_l_goto=left; 
	mot_r_goto=right;

	// Encoder Zur�cksetzen
	sensEncL=0;
	sensEncR=0;
	
	//Goto-System aktivieren
	if (left !=0) mot_goto_l= MOT_GOTO_MAX; 
	else mot_goto_l=0;

	if (right!=0) mot_goto_r=MOT_GOTO_MAX;
	else mot_goto_r=0;
}

/*!
 * Kümmert sich intern um die Ausführung der goto-Kommandos
 * veraendert target_speed_l und target_speed_r
 * @see bot_goto()
 */
void bot_goto_system(void){

  	int diff_l = sensEncL - mot_l_goto;	// Restdistanz links
	int diff_r = sensEncR - mot_r_goto;	// Restdistanz rechts	
	
	// Motor L hat noch keine MOT_GOTO_MAX Nulldurchg�nge gehabt
	if (mot_goto_l >0){
		if (abs(diff_l) <= 2){			// 2 Encoderst�nde genauigkeit reicht
			target_speed_l = BOT_SPEED_STOP;	//Stop
			mot_goto_l--;			// wie Nulldurchgang behandeln
		}else if (abs(diff_l) < 4)
			target_speed_l= BOT_SPEED_SLOW;
		else if (abs(diff_l) < 10)
			target_speed_l= BOT_SPEED_NORMAL;
		else if (abs(diff_l) < 40)
			target_speed_l= BOT_SPEED_FAST;
		else target_speed_l= BOT_SPEED_MAX;

		// Richtung	
		if (diff_l>0) {		// Wenn �bersteurt
			target_speed_l= -target_speed_l;	//Richtung umkehren
		}
		
		// Wenn neue Richtung ungleich alter Richtung
		if (((target_speed_l<0)&& (speed_l>0))|| ( (target_speed_l>0) && (speed_l<0) ) ) 
			mot_goto_l--;		// Nulldurchgang merken
	}

	// Motor R hat noch keine MOT_GOTO_MAX Nulldurchg�nge gehabt
	if (mot_goto_r >0){
		if (abs(diff_r) <= 2){			// 2 Encoderst�nde genauigkeit reicht
			target_speed_r = BOT_SPEED_STOP;	//Stop
			mot_goto_r--;			// wie Nulldurchgang behandeln
		}else if (abs(diff_r) < 4)
			target_speed_r= BOT_SPEED_SLOW;
		else if (abs(diff_r) < 10)
			target_speed_r= BOT_SPEED_NORMAL;
		else if (abs(diff_r) < 40)
			target_speed_r= BOT_SPEED_FAST;
		else target_speed_r= BOT_SPEED_MAX;

		// Richtung	
		if (diff_r>0) {		// Wenn �bersteurt
			target_speed_r= -target_speed_r;	//Richtung umkehren
		}

		// Wenn neue Richtung ungleich alter Richtung
		if (((target_speed_r<0)&& (speed_r>0))|| ( (target_speed_r>0) && (speed_r<0) ) ) 
			mot_goto_r--;		// Nulldurchgang merken
	}
}

/*!
 * TODO: Diese Funktion ist nur ein Dummy Beispiel, wie soetwas aussehen
 * könnte. Hier ist ein guter Einstiegspunkt für eigene Experimente und Algorithmen!
 * Passt auf, dass keine Kollision mit Hindernissen an der Front 
 * geschieht.
 * verändert speed_l_col und speed_r_col
 */ 
void bot_avoid_col(void){	
	if (sensDistR < COL_CLOSEST)	// sehr nah
		col_zone_r=ZONE_CLOSEST;	// dann auf jedenfall CLOSEST Zone
	else 
	// sind wir näher als Near und nicht in der inneren Zone gewesen
	if ((sensDistR < COL_NEAR) && (col_zone_r > ZONE_CLOSEST))
		col_zone_r=ZONE_NEAR;	// dann auf  in die NEAR Zone
	else
	// sind wir näher als FAR und nicht in der NEAR Zone gewesen
	if ((sensDistR < COL_FAR) && (col_zone_r > ZONE_NEAR))
		col_zone_r=ZONE_FAR;	// dann auf  in die FAR Zone
	else
	// wir waren in einer engeren Zone und verlassen sie in Richtung Near
	if (sensDistR < (COL_NEAR * 0.50))
		col_zone_r=ZONE_NEAR;	// dann auf  in die NEAR Zone
	else
	if (sensDistR < (COL_FAR * 0.50))
		col_zone_r=ZONE_FAR;	// dann auf  in die NEAR Zone
	else
		col_zone_r=ZONE_CLEAR;	// dann auf  in die NEAR Zone
	
	if (sensDistL < COL_CLOSEST)	// sehr nah
		col_zone_l=ZONE_CLOSEST;	// dann auf jedenfall CLOSEST Zone
	else 
	// sind wir näher als Near und nicht in der inneren Zone gewesen
	if ((sensDistL < COL_NEAR) && (col_zone_l > ZONE_CLOSEST))
		col_zone_l=ZONE_NEAR;	// dann auf  in die NEAR Zone
	else
	// sind wir näher als FAR und nicht in der NEAR Zone gewesen
	if ((sensDistL < COL_FAR) && (col_zone_l > ZONE_NEAR))
		col_zone_l=ZONE_FAR;	// dann auf  in die FAR Zone
	else
	// wir waren in einer engeren Zone und verlassen sie in Richtung Near
	if (sensDistL < (COL_NEAR * 0.50))
		col_zone_l=ZONE_NEAR;	// dann auf  in die NEAR Zone
	else
	if (sensDistL < (COL_FAR * 0.50))
		col_zone_l=ZONE_FAR;	// dann auf  in die NEAR Zone
	else
		col_zone_l=ZONE_CLEAR;	// dann auf  in die NEAR Zone
	
	
	switch (col_zone_l){
		case ZONE_CLOSEST:
			speed_r_col=-target_speed_r * BRAKE_CLOSEST;
			break;
		case ZONE_NEAR:
			speed_r_col=-target_speed_r  * BRAKE_NEAR;
			break;
		case ZONE_FAR:
			speed_r_col=-target_speed_r  * BRAKE_FAR;
			break;
		case ZONE_CLEAR:
			speed_r_col=0;
			break;
		default: col_zone_l=ZONE_CLEAR;
			break;
	}
		
	switch (col_zone_r){
		case ZONE_CLOSEST:
			speed_l_col=-target_speed_l * BRAKE_CLOSEST;
			break;
		case ZONE_NEAR:
			speed_l_col=-target_speed_l  * BRAKE_NEAR;
			break;
		case ZONE_FAR:
			speed_l_col=-target_speed_l  * BRAKE_FAR;
			break;
		case ZONE_CLEAR:
			speed_l_col=0;
			break;
		default: col_zone_r=ZONE_CLEAR;
			break;
	}	
	
/*	if (sensDistR < COL_CLOSEST)	// sehr nah
		speed_l_col=-target_speed_l-BOT_SPEED_NORMAL;	// rückwärts fahren
	else if (sensDistR < COL_NEAR)	//  nah
		speed_l_col=-target_speed_l * 0.9;		// langsamer werden
	else if (sensDistR < COL_FAR)	//  fern
		speed_l_col=-target_speed_r * 0.65;		// langsamer werden
    else speed_l_col=0;			// nichts tun

	if (sensDistL < COL_CLOSEST)	// sehr nah
		speed_r_col=-target_speed_r-BOT_SPEED_NORMAL;	// rückwärts fahren
	else if (sensDistL < COL_NEAR)	//  nah
		speed_r_col=-target_speed_r  * 0.9;
	else if (sensDistL < COL_FAR)	//  fern
		speed_r_col=-target_speed_r  * 0.65;
	     else speed_r_col=0;
*/	     
	if ((col_zone_r == ZONE_CLOSEST)&&(col_zone_l == ZONE_CLOSEST)){
		speed_l_col=-target_speed_l + BOT_SPEED_MAX;
		speed_r_col=-target_speed_r - BOT_SPEED_MAX;
	}
		
	
}


/*!
 * Verhindert, dass der Bot in Gräben fällt
 */
void bot_avoid_border(){
	int16 gotoL=0;		
	int16 gotoR=0;
	if (sensBorderL > BORDER_DANGEROUS)
		gotoL=-20;
	if (sensBorderR > BORDER_DANGEROUS)
		gotoR=-20;		
	
	if ((gotoL>0)||(gotoR>0)){
		bot_goto(gotoL,gotoR);
		// Kollisionsschutz überschreiben
		speed_l_col=0;
		speed_r_col=0;
	}
}

/*! 
 * Zentrale Verhaltens Routine 
 * Wird regelmässig aufgerufen. 
 * Dies ist der richtige Platz für eigene Routinen, 
 * um den Bot zu steuern
 */
void bot_behave(void){	
	#ifdef RC5_AVAILABLE
		rc5_control();
	#endif

	bot_avoid_col();		// changes speed_l_col, speed_r_col

//	bot_avoid_border();		// changes goto-system and speed_l_col, speed_r_col
	
	bot_goto_system();		//changes speed_r, speed_l
	
	motor_set(target_speed_l+speed_l_col,target_speed_r+speed_r_col);	
}
