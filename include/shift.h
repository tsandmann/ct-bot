/*! @file 	shift.c
 * @brief 	Routinen zur Ansteuerung der Shift-Register
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/

#ifndef SHIFT_H_
#define SHIFT_H_


#define SHIFT_LATCH	(1<<1)			///< Clock to store Data into Shiftregister

#define SHIFT_REGISTER_DISPLAY	0x04	///< Port-Pin for shiftregister latch (display)
#define SHIFT_REGISTER_LED	0x10	///< Port-Pin for shiftregister latch (leds)
#define SHIFT_REGISTER_ENA	0x08	///< Port-Pin for shiftregister latch (enable)

/*!
 * Initialisert die Shift-Register
 */
void shift_init();

/*!
 * Schiebt Daten durch eines der drei 74HC595 Schieberegister
 * Achtung den Port sollte man danach noch per shift_clear() zurücksetzen
 * @param data	Das Datenbyte
 * @param latch_data der Pin an dem der Daten-latch-Pin des Registers (PIN 11) hängt
 * @param latchtore der Pin an dem der latch-Pin zum transfer des Registers (PIN 12) hängt
 */
void shift_data_out(char data, char latch_data, char latch_store);

/*!
 * Schiebt Daten durch eines der drei 74HC595 Schieberegister
 * vereinfachte Version, braucht kein shift_clear()
 * geht NICHT für das Shift-register, an dem das Display-hängt!!!
 * @param data	Das Datenbyte
 * @param latch_data der Pin an dem der Daten-latch-Pin des Registers (PIN 11) hängt
 */
void shift_data(char data, char latch_data);

/*!
 * Setzt die Shift-Register wieder zurück
 */ 
void shift_clear();
#endif
