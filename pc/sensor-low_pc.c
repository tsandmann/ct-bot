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
 * \file 	sensor-low_pc.c
 * \brief 	Low-Level Routinen fuer die Sensor Steuerung des c't-Bots
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	01.12.2005
 */

#ifdef PC
#include "ct-Bot.h"
#include "sensor-low.h"
#include "bot-2-sim.h"
#include "command.h"
#include "sensor.h"
#include <string.h>

#include "tcp.h"
#include <sys/socket.h>
#include <sys/ioctl.h>

#define IMAGE_WIDTH 640

static int sock;

/**
 * Initialisiere alle Sensoren.
 */
void bot_sens_init(void) {
#ifdef ARM_LINUX_BOARD
	sensor_update_distance = sensor_dist_straight; // Distanzsensordaten 1:1 weiterreichen
#endif

	sock = tcp_openConnection("localhost");
}

/**
 * Alle Sensoren aktualisieren.
 */
void bot_sens(void) {
	static int last_pos = CAM_CENTER;

	size_t bytes_avail = 0;
	ioctl(sock, FIONREAD, &bytes_avail);
	if (bytes_avail >= sizeof(int[2])) {
		int pos[2] = {0, 0};
		size_t i;
		int n;
		for (i = 0; i < bytes_avail / sizeof(pos); ++i) {
			n = recv(sock, pos, sizeof(pos), 0);
		}
		if (n < 0) {
			printf("recv() failed or connection closed prematurely\n");
		} else if (pos[0]) {
			const int x = pos[0];
			const int diff = x - IMAGE_WIDTH / 2;
			if (diff < -IMAGE_WIDTH / 32 || diff > IMAGE_WIDTH / 32) {
				last_pos += diff / (IMAGE_WIDTH / 64);
				if (last_pos < CAM_LEFT) {
					last_pos = CAM_LEFT;
				} else if (last_pos > CAM_RIGHT) {
					last_pos = CAM_RIGHT;
				}
			}
			LOG_DEBUG("last_pos=%d\tpos[0]=%d\tpos[1]=%d", last_pos, pos[0], pos[1]);
			servo_set(SERVO2, last_pos);
		}
	}

	sensor_update(); // Weiterverarbeitung der rohen Sensordaten
	led_update(); // LEDs updaten
}
#endif // PC
