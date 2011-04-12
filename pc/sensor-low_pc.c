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
#include "botfs.h"
#include <string.h>

/**
 * Initialisiere alle Sensoren.
 * Dummy fuer PC-Code
 */
void bot_sens_init(void) {
	// NOP
}

/**
 * Alle Sensoren aktualisieren.
 */
void bot_sens(void) {
	sensor_update(); // Weiterverarbeitung der rohen Sensordaten
	led_update(); // LEDs updaten
}

#ifdef BOT_FS_AVAILABLE
#define SLOG_WITH_SPEED_CONTROL	1
#define SLOG_WITHOUT_SPEED_CONTROL 0

/**
 * Konvertiert eine (binaere) Speedlog-Datei ("AVR-Endian") in eine Textdatei.
 * \param input_file Der Dateiname der BotFS-Image-Datei
 */
void convert_slog_file(char * input_file) {
	/** Datentyp der Logbloecke auf der MMC, falls Motorregelung an */
	typedef struct {
		uint8_t encRate;
		uint8_t targetRate;
		int16_t err;
		int16_t pwm;
		uint32_t time;
	} PACKED slog_speedcontrol_t;

	/** Datentyp der Logbloecke auf der MMC, falls Motorregelung aus */
	typedef struct {
		int16_t pwm;
		uint32_t time;
	} PACKED slog_data_t;

	printf("Konvertiere die SpeedLog-Datei aus Image \"%s\" ins txt-Format\n", input_file);
	char buffer[BOTFS_BLOCK_SIZE];
	if (botfs_init(input_file, buffer, False) != 0) {
		puts("Konnte BotFS nicht initialisieren, Abbruch");
		return;
	}
	FILE *fp_output = fopen("slog.txt", "w");
	if (fp_output == NULL) {
		puts("Fehler");
		return;
	}

	botfs_file_descr_t input;
	if (botfs_open(SPEEDLOG_FILE_NAME, &input, BOTFS_MODE_r, buffer) != 0) {
		puts("Fehler beim Oeffnen der Speedlog-Datei");
		return;
	}
	const uint32_t filesize = botfs_get_filesize(&input) * BOTFS_BLOCK_SIZE;

	uint8_t * header;
	botfs_read_header_data(&input, &header, buffer);
	const uint8_t type = *header;
	uint32_t j;
	uint8_t i, k;
	switch (type) {
	case SLOG_WITHOUT_SPEED_CONTROL:
		puts(" Datei ist vom Typ ohne Motorregelung");
		union {
			slog_data_t data[2][25]; /**< Speed-Log Daten */
			uint8_t raw[512]; /**< Platzfueller auf MMC-Block-Groesse */
		} slog; /**< Speed-Log Datentyp */
		const int n = sizeof(slog.data[0]) / sizeof(slog.data[0][0]);

		for (k = 0; k < 2; ++k) { // einmal fuer links und einmal fuer rechts
			botfs_seek(&input, 0, SEEK_SET); // zum Dateianfang
			const char * headL = "PWM_L\tTimestamp\n";
			const char * headR = "PWM_R\tTimestamp\n";
			if (k == 0) {
				fwrite(headL, strlen(headL), 1, fp_output); // links
			} else {
				fwrite(headR, strlen(headR), 1, fp_output); // rechts
			}
			for (j = 0; j <= filesize / 512; ++j) {
				botfs_read(&input, &slog);
				for (i = 0; i < n; ++i) { // 25 Daten pro Block
					if (slog.data[k][i].time == 0) {
						/* 0-Zeile */
						break;
					}
					snprintf(buffer, sizeof(buffer), "%d\t%u\n", slog.data[k][i].pwm, slog.data[k][i].time);
					fwrite(buffer, strlen(buffer), 1, fp_output);
				}
			}
			fwrite("\n", 1, 1, fp_output); // Leerzeile trennt links und rechts
			fwrite("\n", 1, 1, fp_output); // 2. Leerzeile wichtig fuer gnuplot
		}
		break;

	case SLOG_WITH_SPEED_CONTROL:
		puts(" Datei ist vom Typ mit Motorregelung");
		uint8_t data[512];
		for (k = 0; k < 2; k++) { // einmal fuer links und einmal fuer rechts
			fwrite("Ist-Geschw.\tSoll-Geschw.\tFehler\tPWM\tTimestamp\n", 46, 1, fp_output);
			botfs_seek(&input, 0, SEEK_SET); // zum Dateianfang
			/* blockweise Daten einlesen */
			for (j = 0; j <= filesize / 512; ++j) {
				botfs_read(&input, data);
				for (i = 0; i < 25; ++i) { // 25 Daten pro Block
					/* AVR-Speicherformat einlesen, Prinzip hackhack, ist so aber unabhaengig von der Zielplattform */
					slog_speedcontrol_t tmp;
					/* Monsters here */
					tmp.encRate = data[k * 250 + i * 10];
					if (tmp.encRate == 0) {
						/* 0-Zeile */
						break;
					}
					tmp.targetRate = data[k * 250 + i * 10 + 1];
					tmp.err = data[k * 250 + i * 10 + 2] | data[k * 250 + i * 10 + 3] << 8;
					tmp.pwm = data[k * 250 + i * 10 + 4] | data[k * 250 + i * 10 + 5] << 8;
					tmp.time = data[k * 250 + i * 10 + 6] | data[k * 250 + i * 10 + 7] << 8
						| data[k * 250 + i * 10 + 8] << 16 | data[k * 250 + i * 10 + 9] << 24;
					/* Ausgabezeile bauen und schreiben */
					sprintf(buffer, "%u\t%u", tmp.encRate*2, tmp.targetRate*2);
					fwrite(buffer, strlen(buffer), 1, fp_output);
					sprintf(buffer, "\t%d\t%u", tmp.err*2, tmp.pwm);
					fwrite(buffer, strlen(buffer), 1, fp_output);
					sprintf(buffer, "\t%u\n", tmp.time);
					fwrite(buffer, strlen(buffer), 1, fp_output);
				}
			}
			fwrite("\n", 1, 1, fp_output); // Leerzeile trennt links und rechts
			fwrite("\n", 1, 1, fp_output); // 2. Leerzeile wichtig fuer gnuplot
		}
		break;
	}

	botfs_close_volume();
	fclose(fp_output);
	puts("done."); // fertig :)
}
#endif // BOT_FS_AVAILABLE
#endif // PC
