/*! @file 	timer.h
 * @brief 	Timer und counter
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

// Bei einem Prescaler von 1024 z�hlt die CPU 14400 Schritte pro Sekunde!!
// Die Werte f�r TIMER_X_CLOCK sind Angaben in Hz


/*!
 * Timer 0 interrupt freq in Hz
 * Achtung hieran h�ngt die RTC, 
 * 1000/TIMER_0_CLOCK muss eine ganze Zahl
 * ergeben!!!
 */
#define TIMER_0_CLOCK	200	

/*! 
 * Timer 1 frequency in Hz 
 */
#define TIMER_1_CLOCK	1024	// ADC-Takt (ADC triggert auf TCNT1==OCR1B)

/*!
 * Timer 2 frequency in Hz 
 */
#define TIMER_2_CLOCK	5619	// Derzeit genutzt f�r RC5-Dekodierung

/*!
 * initilaisiert Timer 0 und startet ihn 
 */
void timer_0_init(void);

/*!
 * initilaisiert Timer 1 und startet ihn 
 */
void timer_1_init(void);

/*!
 * initilaisiert Timer 2 und startet ihn 
 */
void timer_2_init(void);
