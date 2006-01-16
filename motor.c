/*! @file 	motor.c
 * @brief 	High-Level Routinen für die Motorsteuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	15.01.05
*/
#include <stdlib.h>
#include "global.h"
#include "ct-Bot.h"
#include "motor.h"
#include "motor-low.h"

volatile int16 speed_l=0;	///< Geschwindigkeit linker Motor
volatile int16 speed_r=0;	///< Geschwindigkeit rechter Motor


/*!
 * Direkter Zugriff auf den Motor
 * @param left	Geschwindigkeit für den linken Motor
 * @param right Geschwindigkeit für den linken Motor
 * zwischen -255 und +255
 * 0 bedeutet steht, 255 volle Kraft voraus -255 volle Kraft zur�ck
 * Sinnvoll ist die Verwendung der Konstanten: BOT_SPEED_XXX 
 * Also z.B. motor_set(BOT_SPEED_LOW,-BOT_SPEED_LOW);
 * für eine langsame Drehung
*/
void motor_set(int16 left, int16 right){
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
		
	bot_motor(speed_l,speed_r);
}

/*!
 * Initialisiere den Motorkrams
 */
void motor_init(){
	speed_l=0;
	speed_r=0;
	motor_low_init();
}
