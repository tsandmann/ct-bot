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
 * \file 	behaviour_ubasic_access.h
 * \brief 	Zugriffsroutingen fuer Basic-Interpreter
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	09.09.2011
 */

#ifndef BEHAVIOUR_UBASIC_ACCESS_H_
#define BEHAVIOUR_UBASIC_ACCESS_H_

#include "bot-logic.h"
#ifdef BEHAVIOUR_UBASIC_AVAILABLE
#include "sdfat_fs.h"
#include "init.h"
#include "log.h"
#include <stdio.h>

typedef uint16_t PTR_TYPE;
#define PROG_PTR 					ubasic_get_ptr()
#define GET_CONTENT_PROG_PTR		ubasic_get_content()
#define SET_PROG_PTR_ABSOLUT(param)	ubasic_set_ptr(param)
#define INCR_PROG_PTR				ubasic_incr_ptr()
#define END_OF_PROG_TEXT			ubasic_is_eof()
extern PTR_TYPE program_ptr;

/**
 * \return Offset des aktuellen Program-Pointers in der Datei
 */
static inline PTR_TYPE ubasic_get_ptr(void) {
	return ubasic_ptr;
}

/**
 * \return Das Zeichen, auf das get_ptr() zeigt
 */
static inline char ubasic_get_content(void) {
	return ubasic_content;
}

/**
 * Hilfsfunktion fuer set_ptr() und incr_ptr()
 * \param offset neuer Wert fuer ubasic_ptr
 */
static inline void _ubasic_update_ptr(uint16_t offset) __attribute__((always_inline));
static inline void _ubasic_update_ptr(uint16_t offset) {
	const uint16_t last_block = ubasic_ptr / SD_BLOCK_SIZE;
	ubasic_ptr = offset;
	const uint16_t block = ubasic_ptr / SD_BLOCK_SIZE;
	const uint16_t index = ubasic_ptr % SD_BLOCK_SIZE;

	if (block != last_block) {
		sdfat_seek(ubasic_prog_file, block * SD_BLOCK_SIZE, SEEK_SET);
		if (sdfat_read(ubasic_prog_file, GET_MMC_BUFFER(ubasic_buffer), SD_BLOCK_SIZE) != SD_BLOCK_SIZE) {
			LOG_ERROR("_ubasic_update_ptr(): sdfat_read() failed.");
		}
	}

	ubasic_content = (char) GET_MMC_BUFFER(ubasic_buffer)[index];
}

/**
 * Setzt den Program-Pointer auf einen neuen Wert
 * \param offset Neuer Wert (Offset innerhalb der Datei)
 */
static inline void ubasic_set_ptr(uint16_t offset) {
	_ubasic_update_ptr(offset);
}

/**
 * Erhoeht den Wert des Program-Pointers um eins
 */
static inline void ubasic_incr_ptr(void) {
	_ubasic_update_ptr(ubasic_ptr + 1);
}

/**
 * \return True, falls Programmende erreicht
 */
static inline char ubasic_is_eof(void) {
	return ubasic_get_content() == 0;
}

#endif // BEHAVIOUR_UBASIC_AVAILABLE
#endif // BEHAVIOUR_UBASIC_ACCESS_H_
