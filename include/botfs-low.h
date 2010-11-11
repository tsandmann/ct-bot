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
 * \file 	botfs-low.h
 * \brief 	Low-Level-Funktionen des Dateisystems BotFS
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	18.02.2008
 */

#ifndef BOTFSLOW_H_
#define BOTFSLOW_H_

#include "ct-Bot.h"

#ifdef BOT_FS_AVAILABLE
#include "os_thread.h"

extern uint32_t first_block; /**< Adresse des ersten Blocks des Volumes */

#ifdef MCU
typedef uint8_t botfs_mutex_t;
#define BOTFS_MUTEX_INITIALIZER 0
#else
typedef pthread_mutex_t botfs_mutex_t;
#define BOTFS_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#endif // MCU

/**
 * Laedt das Volume
 * \param *image	Dateiname des Images
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte
 * \param create	Soll das Volume erzeugt werden, falls es nicht existiert?
 * \return			0, falls kein Fehler
 */
int8_t botfs_init_low(char * image, void * buffer, uint8_t create);

/**
 * Schliesst das BotFS-Volume (beendet BotFS sauber)
 */
void botfs_close_volume_low(void);

#ifdef MCU
/**
 * Wartet, bis ein Mutex verfuegbar ist und sperrt es dann
 * \param *p_mutex Zeiger auf das gewuenschte Mutex
 * \todo: Auslagern in allgemeine Fkt. os_mutex_lock()
 */
void botfs_acquire_lock_low(botfs_mutex_t * p_mutex);

/**
 * Gibt ein Mutex wieder frei
 * \param *p_mutex Zeiger auf das Mutex
 * \todo: Auslagern in allgemeine Fkt. os_mutex_unlock()
 */
static inline void botfs_release_lock_low(botfs_mutex_t * p_mutex) {
	*p_mutex = 0;
}

#else // PC

/**
 * Wartet, bis ein Mutex verfuegbar ist und sperrt es dann
 * \param *p_mutex Zeiger auf das gewuenschte Mutex
 * \todo: Auslagern in allgemeine Fkt. os_mutex_lock()
 */
static inline void botfs_acquire_lock_low(botfs_mutex_t * p_mutex) {
	pthread_mutex_lock(p_mutex);
}

/**
 * Gibt ein Mutex wieder frei
 * \param *p_mutex Zeiger auf das Mutex
 * \todo: Auslagern in allgemeine Fkt. os_mutex_unlock()
 */
static inline void botfs_release_lock_low(botfs_mutex_t * p_mutex) {
	pthread_mutex_unlock(p_mutex);
}
#endif // MCU

#if defined DEBUG_BOTFS || defined DEBUG_BOTFS_LOGFILE
/**
 * Schreibt eine Log-Message
 * \param *fd		File-Handle fuer Ausgabe
 * \param *format	Format-String (wie bei printf)
 */
void botfs_log_low(FILE * fd, const char * format, ...);
#endif // defined DEBUG_BOTFS || defined DEBUG_BOTFS_LOGFILE

/**
 * Liest einen BOTFS_BLOCK_SIZE Byte grossen Block
 * \param block		Blockadresse der Daten
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte, in den die Daten geschrieben werden
 * \return			0, falls kein Fehler
 */
int8_t botfs_read_low(uint16_t block, void * buffer) __attribute__((noinline));

/**
 * Schreibt einen BOTFS_BLOCK_SIZE Byte grossen Block
 * \param block		Blockadresse der Daten
 * \param *buffer	Puffer fuer mindestens BOTFS_BLOCK_SIZE Byte, dessen Daten geschrieben werden
 * \return			0, falls kein Fehler
 */
int8_t botfs_write_low(uint16_t block, void * buffer) __attribute__((noinline));

/**
 * Rechnet eine (lokale) Blockadresse in einen Sektor um
 * \param block	Blockadresse
 * \return		absolute Sektoradresse
 */
static inline uint32_t get_sector(uint16_t block) {
	return first_block + block;
}

#ifdef PC
/**
 * Erzeugt eine neue Image-Datei fuer ein Volume
 * \param *image	Name der Image-Datei
 * \param size		Groesse der Datei in Byte
 * \return			0, falls kein Fehler
 */
int8_t botfs_create_volume_low(const char * image, uint32_t size);

#if defined DEBUG_BOTFS || defined DEBUG_BOTFS_LOGFILE
/**
 * Schreibt den Log-Puffer raus
 * \param *fd	File-Handle fuer Ausgabe
 */
void botfs_flush_log(FILE * fd);
#endif // defined DEBUG_BOTFS || defined DEBUG_BOTFS_LOGFILE
#endif // PC

#endif // BOT_FS_AVAILABLE
#endif // BOTFSLOW_H_
