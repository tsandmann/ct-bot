/*! @file 	bot-logik.h
 * @brief 	High-Level Routinen für die Steuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/

#ifndef bot_logik_H_
#define bot_logik_H_

extern volatile int16 target_speed_l;	///< Sollgeschwindigkeit linker Motor
extern volatile int16 target_speed_r;	///< Sollgeschwindigkeit rechter Motor

/*!
 * Drehe die Räder um die gegebene Zahl an Encoder-Schritten weiter
 * @param left Schritte links
 * @param right Schritte rechts
 */
void bot_goto(int left, int right);

/*!
 * Kümmert sich intern um dsie ausführung der goto-Kommandos
 * @see bot_goto()
 */
void bot_behave(void);

#endif
