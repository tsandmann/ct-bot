/*
 * c't-Bot
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

/**
 * \file 	behaviour_abl.h
 * \brief 	Abstract Bot Language Interpreter
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	28.11.2007
 */

#ifndef BEHAVIOUR_ABL_H_
#define BEHAVIOUR_ABL_H_

#ifdef BEHAVIOUR_ABL_AVAILABLE

#define ABL_INSTRUCTION_LENGTH		32		/**< maximale Laenge einer Instruktion in Byte */
#define ABL_STACK_SIZE				16		/**< Stackgroesse in Byte (Zweierpotenz!) */
#define ABL_STACK_MASK (ABL_STACK_SIZE - 1)	/**< Bitmaske fuer Stackpointer */
#define ABL_PATHNAME_LENGTH			16		/**< maximale Laenge des Pfads zur ABL-Datei */

#if (ABL_STACK_SIZE & ABL_STACK_MASK)
#error ABL stack size is not a power of 2!
#endif

extern remote_call_data_t abl_params[];		/**< Parameter-Daten so wie die RemoteCalls sie erwarten (32 Bit aligned) */
extern char abl_eeprom_data[]; 				/**< EEPROM-Bereich fuer ABL-Daten */

/**
 * Der ABL-Interpreter als Verhalten
 * \param *data Der Verhaltensdatensatz
 */
void bot_abl_behaviour(Behaviour_t * data);

/**
 * Botenfunktion des ABL-Interpreters.
 * Laedt das erste Programm-Segment, initialisiert Pointer und startet das Verhalten.
 * Nicht per Remote-Call aufrufbar, da das Verhalten selbst Remote-Calls absetzt.
 * \param *caller Zeiger auf den Verhaltensdatensatz des Aufrufers
 * \param *filename	Programmdatei oder NULL, falls EEPROM / vorherige Programmdatei
 */
void bot_abl(Behaviour_t * caller, const char * filename);

/**
 * Laedt ein ABL-Programm aus einer Datei
 * \param *filename Dateiname
 * \return Fehlercode: 0, falls alles ok
 */
int8_t abl_load(const char * filename);

/**
 * Syntax-Check des aktuellen ABL-Programms.
 * Liefert SUBSUCCESS, falls Syntax OK, sonst SUBFAIL.
 * \param *caller Zeiger auf Verhaltensdatensatz des Aufrufers
 * \param line Zeilennummer, bis zu der geprueft werden soll, 0 fuer alle
 */
void bot_abl_check(Behaviour_t * caller, uint16_t line);

/**
 * Bricht ein laufendes ABL-Programm ab
 */
void abl_cancel(void);

/**
 * Pusht den Parameter Nr. 0 auf den ABL-Stack
 * \see abl_push_value(uint16_t value)
 */
void abl_push(void);

/**
 * Pusht einen Wert auf den ABL-Stack
 * \param value Der Wert, der auf den Stack soll
 */

static inline
#ifndef DOXYGEN
__attribute__((always_inline))
#endif
void abl_push_value(uint16_t value) {
	abl_params[0].u16 = value;
	abl_push();
}

/**
 * Displayhandler fuer ABL-Stack Ausgabe
 */
void abl_stack_trace(void);

#endif // BEHAVIOUR_ABL_AVAILABLE

#endif // BEHAVIOUR_ABL_H_
