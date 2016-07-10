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
 * \file 	uart-test_pc.c
 * \brief 	Testprogramm fuer UART unter Linux
 * \author 	Timo Sandmann (mail@timosandmann.de)
 * \date 	04.05.2013
 */

#ifdef PC

#include "ct-Bot.h"
#include "uart.h"
#include "bot-2-atmega.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#ifdef ARM_LINUX_BOARD

extern command_t cmd_to_send;

void uart_test(uint32_t runs) {
	puts("Initializing...");
	if (bot_2_atmega_init() != 0) {
		exit(1);
	}
	set_bot_2_atmega();
	memset(&cmd_to_send, 0, sizeof(command_t));
	memset(&received_command, 1, sizeof(command_t));

	printf("Sending %u CMD_DONE packages...\n", runs);

	struct timeval start, end;
	gettimeofday(&start, NULL);

	uint32_t i;
	for (i = 0; i < runs; ++i) {
//		uint8_t stop = 0;
		command_write(CMD_DONE, SUB_CMD_NORM, i, 0, 0);
		int8_t r = receive_until_frame(CMD_DONE);
		if (r != 0) {
			LOG_ERROR("uart_test(): %u: received invalid command: %d", i, r);
			unsigned j;
			uint8_t * rx = (uint8_t *) &received_command;
			for (j = 0; j < sizeof(command_t); ++j) {
				printf("0x%02x ", *(rx++));
			}
			puts("");
//			stop = 1;
		}
//		puts("tx:");
//		command_display(&cmd_to_send);
//		puts("rx:");
//		command_display(&received_command);
//		unsigned j;
//		uint8_t * tx = (uint8_t *) &cmd_to_send;
//		uint8_t * rx = (uint8_t *) &received_command;
//		for (j = 0; j < sizeof(command_t); ++j) {
//			if (*rx != *tx && j != 3) {
//				printf("uart_test(): ERROR on cmd %u: byte 0x%x differs: 0x%x/0x%x\n", i, j, *tx, *rx);
//				stop = 1;
//			}
//			++rx;
//			++tx;
//		}
//		if (stop) {
//			exit(1);
//		}
		memset(&cmd_to_send, 0, sizeof(command_t));
		memset(&received_command, 1, sizeof(command_t));
	}
	gettimeofday(&end, NULL);
	uint64_t t = (end.tv_sec - start.tv_sec) * 1000000UL + end.tv_usec - start.tv_usec;
	printf("done (%u commands in %llu us).\n", runs, t);
	printf("%llu us / cmd.\n", t / runs);

	exit(0);
}

#endif // ARM_LINUX_BOARD
#endif // PC
