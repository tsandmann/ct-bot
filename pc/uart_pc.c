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
 * \file 	uart_pc.c
 * \brief 	Routinen zur seriellen Kommunikation unter Linux
 * \author	Timo Sandmann (mail@timosandmann.de)
 * \date 	03.05.2013
 */

#if defined PC && defined __linux__

#include "ct-Bot.h"
#include "uart.h"
#include "log.h"

#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>


//#define DEBUG_UART_PC // Schalter, um auf einmal alle Debugs an oder aus zu machen

#ifndef DEBUG_UART_PC
#undef LOG_DEBUG
#define LOG_DEBUG(...) {} /**< Log-Dummy */
#endif


static int fd = -1;
static struct termios old_settings;

/**
 * Initialisiert das UART
 * \param port Port des UARTs
 * \return Fehlercode oder 0, wenn alles OK
 */
uint8_t uart_init(const char * port) {
#if UART_BAUD == 57600
	int baudr = B57600;
#elif UART_BAUD == 115200
	int baudr = B115200;
#elif UART_BAUD == 230400
	int baudr = B230400;
#elif UART_BAUD == 460800
	int baudr = B460800;
#elif UART_BAUD == 500000
	int baudr = B500000;
#elif UART_BAUD == 1000000
	int baudr = B1000000;
#else
#error "Baudrate nicht unterstuetzt!"
	int baudr = 0;
#endif // UART_BAUD

	return uart_init_baud(port, baudr);
}

/**
 * Initialisiert das UART mit der angegebenen Baudrate
 * \param port Port des UARTs
 * \param baudr gewuenschte Baudrate des UARTs
 * \return Fehlercode oder 0, wenn alles OK
 */
uint8_t uart_init_baud(const char * port, int baudr) {
	LOG_DEBUG("uart_init_baud(): Opening port %s ...", port);

	fd = open(port, O_RDWR | O_NOCTTY /*| O_NDELAY*/);
	if (fd == -1) {
		LOG_ERROR("Unable to open port \"%s\"", port);
		return 1;
	}

	if (tcflush(fd, TCIOFLUSH)) {
		LOG_ERROR("Unable to flush serial port \"%s\"", port);
		return 2;
	}

	int r = tcgetattr(fd, &old_settings);
	if (r != 0) {
		LOG_ERROR("Unable to read settings of port \"%s\"", port);
		uart_close();
		return 3;
	}

	struct termios settings;
	memset(&settings, 0, sizeof(settings));


	settings.c_cflag = baudr | CS8 | CLOCAL | CREAD;
	settings.c_iflag = IGNPAR;
	settings.c_oflag = 0;
	settings.c_lflag = 0;
	settings.c_cc[VMIN] = 1; // block until n bytes are received
	settings.c_cc[VTIME] = 0; // block until timeout (n * 100 ms)
//	cfmakeraw(&settings);

	r = tcsetattr(fd, TCSANOW, &settings);
	if (r != 0) {
		LOG_ERROR("Unable to set settings to port \"%s\"", port);
		uart_close();
		return 4;
	}

	int status;
	if (ioctl(fd, TIOCMGET, &status) == -1) {
		LOG_ERROR("Unable to get status of port \"%s\"", port);
		uart_close();
		return 5;
	}

	int bytes_available;
	if (ioctl(fd, FIONREAD, &bytes_available) == -1) {
		LOG_ERROR("Unable to get available bytes of port \"%s\"", port);
		uart_close();
		return 6;
	}

	LOG_DEBUG("uart_init_baud(): Reading available %d bytes from port %s ...", bytes_available, port);
	int i, tmp;
	for (i = 0; i < bytes_available; ++i) {
		if (uart_read(&tmp, 1) != 1) {
			LOG_ERROR("Unable to read available bytes of port \"%s\"", port);
			uart_close();
			return 7;
		}
	}

	LOG_DEBUG("uart_init_baud(): Port %s initialized", port);

	return 0;
}

/**
 * Schliesst den UART-Port
 * \return Fehlercode oder 0, wenn alles OK
 */
uint8_t uart_close(void) {
	if (fd == -1) {
		return 1;
	}

	int r = tcsetattr(fd, TCSANOW, &old_settings);
	if (r != 0) {
		LOG_ERROR("Unable to set settings to serial port");
		return 2;
	}

	close(fd);

	fd = -1;

	return 0;
}

/**
 * Flusht das UART
 */
void uart_flush(void) {
	tcflush(fd, TCIOFLUSH);
}

/**
 * Liest Zeichen vom UART
 * \param data		Der Zeiger an den die gelesenen Zeichen kommen
 * \param length	Anzahl der zu lesenden Bytes
 * \return			Anzahl der tatsaechlich gelesenen Zeichen
 */
int16_t uart_read(void * data, int16_t length) {
	LOG_DEBUG("uart_read(%d)", length);
	int16_t n = read(fd, data, length);
	LOG_DEBUG("uart_read(): read %d bytes", n);
	return n;
}

/**
 * Sendet Daten per UART im Little Endian
 * \param *data		Zeiger auf Datenpuffer
 * \param length	Groesse des Datenpuffers in Bytes
 * \return			Anzahl der geschriebenen Bytes
 */
int16_t uart_write(const void * data, int16_t length) {
	LOG_DEBUG("uart_write(%d)", length);
	int16_t n = write(fd, data, length);
	LOG_DEBUG("uart_write(): %d bytes written", n);
	return n;
}

/**
 * Prueft, ob Daten vom UART verfuegbar
 * \return	Anzahl der verfuegbaren Bytes oder -1 bei Fehler
 */
int16_t uart_data_available(void) {
	int bytes_available;
	if (ioctl(fd, FIONREAD, &bytes_available) == -1) {
		LOG_ERROR("uart_data_available(): ioctl() failed");
		return -1;
	}
	return (int16_t) bytes_available;
}

#endif // PC
