/*! @file 	bot-mot.h 
 * @brief 	Low-Level Routinen für die Motorsteuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/

#define BOT_SPEED_STOP		0		///< Motor aus
#define BOT_SPEED_SLOW		10		///< langsame Fahrt
#define BOT_SPEED_NORMAL	50		///< normale Fahrt
#define BOT_SPEED_FAST		150		///< schnelle Fahrt
#define BOT_SPEED_MAX		255		///< maximale Fahrt

#define SERVO_LEFT 	8
#define SERVO_RIGHT	16
#define SERVO_MIDDLE   ((SERVO_RIGHT- SERVO-LEFT)/2)

#define SERVO1 1
#define SERVO2 2

extern int volatile speed_l;			///< Geschwindigkeit des linken Motors
extern int volatile speed_r;			///< Geschwindigkeit des rechten Motors

/*!
 *  Initialisiert alles für die Motosteuerung 
 */
void bot_mot_init(void);

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
void motor_set(int left, int right);

/*!
 * Zugriff auf die Motoren mit Angabe über Encoderstände
 * @param left Anzahl Encoderschritte links
 * @param right Anzahl Encoderschritte rechts
 */
void motor_goto(int left, int right);

/*!
 * unmittelbarere Zugriff auf die beiden Motoren
 * normalerweise NICHT verwenden!!!!!
 * @param left speed links
 * @param right speed rechts
*/
void bot_motor(int left, int right);

/*!
 * Stellt die Servos
 * Sinnvolle Werte liegen zwischen 8 und 16
 */
void servo_set(char servo, char pos);
