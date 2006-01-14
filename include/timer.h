/*! @file 	timer.h
 * @brief 	Timer und counter
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

// Die Werte fuer TIMER_X_CLOCK sind Angaben in Hz

/*!
 * Timer 2 frequency in Hz 
 */
#define TIMER_2_CLOCK	5619	// Derzeit genutzt f�r RC5-Dekodierung

/*!
 * initilaisiert Timer 2 und startet ihn 
 */
void timer_2_init(void);
