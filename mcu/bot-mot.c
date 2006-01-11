/*! @file 	bot-mot.c 
 * @brief 	Low-Level Routinen für die Motorsteuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/
#ifdef MCU

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>

#include <stdlib.h>
#include "bot-mot.h"
#include "bot-sens.h"


//PWM-Steuerung der Motoren
#define BOT_PWM_L_PIN 0x80	//IO17
#define BOT_PWM_L_PORT PORTD
#define BOT_PWM_L_DDR DDRD

#define BOT_PWM_R_PIN 0x04	//IO16
#define BOT_PWM_R_PORT PORTD
#define BOT_PWM_R_DDR DDRD

//Drehrichtung der Motoren
#define BOT_DIR_L_PIN 0x80	//IO6
#define BOT_DIR_L_PORT PORTC
#define BOT_DIR_L_DDR DDRC

#define BOT_DIR_R_PIN 0x40	//IO7 
#define BOT_DIR_R_PORT PORTC
#define BOT_DIR_R_DDR DDRC


// Steuerung der Motor-Pins

//PWM Linker/Rechter Motor
#define PWM_L(P) BOT_PWM_L_PORT = (BOT_PWM_L_PORT & ~BOT_PWM_L_PIN) | (BOT_PWM_L_PIN * P)
#define PWM_R(P) BOT_PWM_R_PORT = (BOT_PWM_R_PORT & ~BOT_PWM_R_PIN) | (BOT_PWM_R_PIN * P)

//Richtung Linker/Rechter Motor
#define DIR_L(P) BOT_DIR_L_PORT = (BOT_DIR_L_PORT & ~BOT_DIR_L_PIN) | (BOT_DIR_L_PIN * P)
#define DIR_R(P) BOT_DIR_R_PORT = (BOT_DIR_R_PORT & ~BOT_DIR_R_PIN) | (BOT_DIR_R_PIN * P)

volatile char mot_l_high =0;	///< High-Anteil linker Motor (255=on,0=off)
volatile char mot_r_high =0;	///< High-Anteil rechter Motor (255=on,0=off)

volatile char mot_pwm_cnt =0;	///< PWM-Counter 

volatile char mot_l_dir =0;	///< Drehrichtung linker Motor
volatile char mot_r_dir =0;	///< Drehrichtung rechter Motor

volatile int speed_l=0;		///< Geschwindigkeit linker Motor
volatile int speed_r=0;		///< Geschwindigkeit linker Motor

/*!
 *  Initilisiert alles für die Motosteuerung 
 */
void bot_mot_init(){
//Sieht umst�ndlich aus, erzeugt aber rel. kurzen Code
	BOT_PWM_L_DDR|=BOT_PWM_L_PIN;
	PWM_L(0);

	BOT_PWM_R_DDR|=BOT_PWM_R_PIN;
	PWM_R(0);
	
	BOT_DIR_L_DDR|=BOT_DIR_L_PIN;
	DIR_L(0);
	
	BOT_DIR_R_DDR|=BOT_DIR_R_PIN;
	DIR_R(0);
}

/*!
 * unmittelbarere Zugriff auf die beiden Motoren
 * normalerweise NICHT verwenden!!!!!
 * @param left PWM links
 * @param right PWM rechts
*/
void bot_motor(int left, int right){
	PWM_R(abs(right));
	
	if (left < 0 ){	 DIR_L(1); 
		}else {	 DIR_L(0);}
	
	PWM_L(abs(left));
	if (right < 0 ){ DIR_R(1);
		} else { DIR_R(0);}
}

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
void motor_set(int left, int right){
	if (abs(left) > BOT_SPEED_MAX)	// Nicht schneller fahren als m�glich
		mot_l_high = BOT_SPEED_MAX;
	else if (left == 0)	// Stop wird nicht ver�ndert
		mot_l_high = BOT_SPEED_STOP;
	else if (abs(left) < BOT_SPEED_SLOW)	// Nicht langsamer als die 
		mot_l_high = BOT_SPEED_SLOW;	// Motoren k�nnen
	else				// Sonst den Wunsch �bernehmen
		mot_l_high = (char) abs(left);
	
	if (abs(right) > BOT_SPEED_MAX)// Nicht schneller fahren als m�glich
		mot_r_high = BOT_SPEED_MAX;
	else if (abs(right) == 0)	// Stop wird nicht ver�ndert
		mot_r_high = BOT_SPEED_STOP;
	else if (abs(right) < BOT_SPEED_SLOW)	// Nicht langsamer als die 
		mot_r_high = BOT_SPEED_SLOW;	// Motoren k�nnen
	else				// Sonst den Wunsch �bernehmen
		mot_r_high = (char) abs(right);
	
	if (left < 0 )	mot_l_dir=0;
	else if (left > 0 ) mot_l_dir=1;
	DIR_L(mot_l_dir);
	
	if (right < 0 )	mot_r_dir=0;	
	else if (right > 0 ) mot_r_dir=1;
	DIR_R(mot_r_dir);
}


/*!
 * PWM-Steuerung und Co 
 */
void motor_isr(){	
	mot_pwm_cnt++; 
	if (mot_pwm_cnt >= BOT_SPEED_MAX) 
		mot_pwm_cnt=0;
	
	if (mot_l_high > mot_pwm_cnt){
		PWM_L(1);
	} else {
		PWM_L(0);
	}

	if (mot_r_high > mot_pwm_cnt){
		PWM_R(1);
	} else {
		PWM_R(0);
	}
}
#endif
