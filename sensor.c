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

/*!
 * @file 	sensor.c
 * @brief 	Architekturunabhaengiger Teil der Sensorsteuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	15.01.2005
 */

#include "ct-Bot.h"
#include "timer.h"
#include "bot-local.h"
#include "math.h"
#include "sensor_correction.h"
#include "ui/available_screens.h"
#include "display.h"
#include "sensor.h"
#include "mouse.h"
#include "log.h"
#include "led.h"
#include "eeprom.h"
#include "motor.h"
#include "math_utils.h"
#include "ir-rc5.h"
#include "uart.h"
#include <stdio.h>
#include <float.h>

#define HEADING_START		0	/*!< Blickrichtung, mit der sich der Bot initialisiert */
#define HEADING_SIN_START	0	/*!< sin(HEADING_START) */
#define HEADING_COS_START	1	/*!< cos(HEADING_START) */

#define SPEED_UPDATE_TIME	500U	/*!< Zeitspanne zwischen zwei Geschwindigkeits-Berechnungen [ms] */

int16_t sensLDRL = 0;		/*!< Lichtsensor links */
int16_t sensLDRR = 0;		/*!< Lichtsensor rechts */

int16_t sensDistL = 1023;	/*!< Distanz linker IR-Sensor in [mm], wenn korrekt umgerechnet wird */
int16_t sensDistR = 1023;	/*!< Distanz rechter IR-Sensor in [mm], wenn korrekt umgerechnet wird */
uint8_t sensDistLToggle = 0;	/*!< Toggle-Bit des linken IR-Sensors */
uint8_t sensDistRToggle = 0;	/*!< Toggle-Bit des rechten IR-Sensors */
/*! Zeiger auf die Auswertungsfunktion fuer die Distanzsensordaten, const. solange sie nicht kalibriert werden */
void (* sensor_update_distance)(int16_t * const p_sens, uint8_t * const p_toggle, const distSens_t * ptr, int16_t volt) = sensor_dist_lookup;

distSens_t EEPROM sensDistDataL[] = SENSDIST_DATA_LEFT;		/*!< kalibrierte Referenzdaten fuer linken IR-Sensor */
distSens_t EEPROM sensDistDataR[] = SENSDIST_DATA_RIGHT;	/*!< kalibrierte Referenzdaten fuer rechten IR-Sensor */
uint8_t EEPROM sensDistOffset = SENSDIST_OFFSET;			/*!< Spannungs-Offset IR-Sensoren */

int16_t sensBorderL = 0;	/*!< Abgrundsensor links */
int16_t sensBorderR = 0;	/*!< Abgrundsensor rechts */

int16_t sensLineL = 0;	/*!< Lininensensor links */
int16_t sensLineR = 0;	/*!< Lininensensor rechts */

uint8_t sensTrans = 0;	/*!< Sensor Ueberwachung Transportfach */

uint8_t sensDoor = 0;	/*!< Sensor Ueberwachung Klappe */

uint8_t sensError = 0;	/*!< Ueberwachung Servo oder Batteriefehler [0/1]  1= alles ok */

#ifdef BPS_AVAILABLE
uint16_t sensBPS = BPS_NO_DATA; /*!< Bot Positioning System */
/*! RC5-Konfiguration fuer BPS-Sensor */
ir_data_t bps_ir_data = {
	0, 0, 0, 0, 0, BPS_NO_DATA
};
#endif // BPS_AVAILABLE

#ifdef MOUSE_AVAILABLE
int8_t sensMouseDX;		/*!< Maussensor Delta X, positive Werte zeigen querab der Fahrtrichtung nach rechts */
int8_t sensMouseDY;		/*!< Maussensor Delta Y, positive Werte zeigen in Fahrtrichtung */
int16_t sensMouseX;		/*!< Mausposition X, positive Werte zeigen querab der Fahrtrichtung nach rechts */
int16_t sensMouseY;		/*!< Mausposition Y, positive Werte zeigen in Fahrtrichtung  */
#endif // MOUSE_AVAILABLE

int16_t sensEncL = 0;	/*!< Encoder linkes Rad */
int16_t sensEncR = 0;	/*!< Encoder rechtes Rad */

float heading_enc = HEADING_START;	/*!< Blickrichtung aus Encodern */
float x_enc = 0;		/*!< X-Koordinate aus Encodern [mm] */
float y_enc = 0;		/*!< Y-Koordinate aus Encodern [mm] */
int16_t v_enc_left = 0;		/*!< Abrollgeschwindigkeit des linken Rades in [mm/s] [-128 bis 127] relaisitisch [-50 bis 50] */
int16_t v_enc_right = 0;	/*!< Abrollgeschwindigkeit des linken Rades in [mm/s] [-128 bis 127] relaisitisch [-50 bis 50] */
int16_t v_enc_center = 0;	/*!< Schnittgeschwindigkeit ueber beide Raeder */

#ifdef PC
int16_t simultime = 0;	/*!< Simulierte Zeit */
#endif

#ifdef MEASURE_MOUSE_AVAILABLE
float heading_mou = HEADING_START;	/*!< Aktuelle Blickrichtung relativ zur Startposition aus Mausmessungen */
float x_mou = 0;			/*!< Aktuelle X-Koordinate in mm relativ zur Startposition aus Mausmessungen */
float y_mou = 0;			/*!< Aktuelle Y-Koordinate in mm relativ zur Startposition aus Mausmessungen */
int16_t v_mou_center = 0;	/*!< Geschwindigkeit in mm/s ausschliesslich aus den Maussensorwerten berechnet */
int16_t v_mou_left = 0;		/*!< ...aufgeteilt auf linkes Rad */
int16_t v_mou_right = 0;	/*!< ...aufgeteilt auf rechtes Rad */
#endif // MEASURE_MOUSE_AVAILABLE

float heading = HEADING_START;			/*!< Aktuelle Blickrichtung aus Encoder-, Maus- oder gekoppelten Werten */
int16_t heading_int = HEADING_START;	/*!< (int16_t) heading */
int16_t heading_10_int = HEADING_START * 10;	/*!< = (int16_t) (heading * 10.0f) */
float heading_sin = HEADING_SIN_START;			/*!< = sin(rad(heading)) */
float heading_cos = HEADING_COS_START;			/*!< = cos(rad(heading)) */
int16_t x_pos = 0;			/*!< Aktuelle X-Position aus Encoder-, Maus- oder gekoppelten Werten */
int16_t y_pos = 0;			/*!< Aktuelle Y-Position aus Encoder-, Maus- oder gekoppelten Werten */
int16_t v_left = 0;			/*!< Geschwindigkeit linkes Rad aus Encoder-, Maus- oder gekoppelten Werten */
int16_t v_right = 0;		/*!< Geschwindigkeit rechtes Rad aus Encoder-, Maus- oder gekoppelten Werten */
int16_t v_center = 0;		/*!< Geschwindigkeit im Zentrum des Bots aus Encoder-, Maus- oder gekoppelten Werten */

#ifdef MEASURE_POSITION_ERRORS_AVAILABLE
int16_t pos_error_radius = 0;	/*!< Aktueller Fehlerradius der Position */
#endif

#ifdef SRF10_AVAILABLE
uint16_t sensSRF10;		/*!< Messergebniss Ultraschallsensor */
#endif

#ifdef CMPS03_AVAILABLE
cmps03_t sensCmps03 = {0};	/*!< Lage laut CMPS03-Kompass */
#endif

/*!
 * Interpoliert linear zwischen zwei gegebenen Wertepaaren
 * @param x1	groesere Abszisse
 * @param y1	Ordinate zu x1, f(x1)
 * @param x2	kleinere Abszisse
 * @param y2	Ordinate zu x2, f(x2)
 * @param xs	Abzisse des zu interpolierenden Punktes
 * @return		f(xs)
 * Gibt den Funktionswert einer Stelle auf der errechneten Geraden durch die zwei Punkte zurueck.
 * Achtung, die Funktion rechnet so weit wie moeglich in 8 Bit, das Ergebnis ist nur korrekt,
 * wenn x1 >= xs >= x2, y2 >= y1, x1 != x2 erfuellt ist!
 */
static inline uint8_t lin_interpolate(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t xs) {
	if (x1 == x2) {
		return 255;
	}
	uint16_t m = ((uint16_t) (y2 - y1) << 8) / (uint8_t) (x1 - x2);	// m >= 0
	uint8_t x_diff = (uint8_t) (x1 - xs);
	return (uint8_t) ((uint8_t) ((x_diff * m) >> 8) + y1);	// m war kuenstlich um 8 Bit hochskaliert
}

/*!
 * Errechnet aus den rohren Distanzsensordaten die zugehoerige Entfernung
 * @param p_sens	Zeiger auf den (Ziel-)Sensorwert
 * @param p_toggle	Zeiger auf die Toggle-Variable des Zielsensors
 * @param ptr		Zeiger auf auf Sensorrohdaten im EEPROM fuer p_sens
 * @param volt_16	Spannungs-Ist-Wert, zu dem die Distanz gesucht wird (in 16 Bit)
 */
void sensor_dist_lookup(int16_t * const p_sens, uint8_t * const p_toggle, const distSens_t * ptr, int16_t volt_16) {
//	if (sizeof(sensDistDataL) != sizeof(sensDistDataR)) {
//		/* sensDistDataL und sensDistDataR muessen gleich gross sein! */
//		LOG_ERROR("sensDistData unzulaessig");
//		return;
//	}
	uint8_t i;
	uint8_t n = sizeof(sensDistDataL) / sizeof(distSens_t) / 2;
	uint8_t volt;
	/* Offset einlesen und Messwerte pruefen */
	uint8_t offset = ctbot_eeprom_read_byte(&sensDistOffset);
	if (volt_16 > 255 * 2 + offset) volt = 255;
	else volt = (uint8_t) ((volt_16 >> 1) - offset);

	/* Spannung in LT-Table suchen */
	uint8_t pivot = ctbot_eeprom_read_byte(&ptr[n-1].voltage);	// in welcher Region muessen wir suchen?
	if (volt > pivot) {
		/* in unterer Haelfte suchen */
		i = 0;
	} else {
		/* in oberer Haelfte suchen */
		i = n;
		ptr += n;
		n = sizeof(sensDistDataL) / sizeof(distSens_t);
	}
	uint8_t tmp=0;
	for (; i<n; i++) {
		tmp = ctbot_eeprom_read_byte(&ptr->voltage);
		if (volt > tmp)	// aufsteigend suchen, damit der kritische Fall (kleine Entfernung) schneller gefunden wird
			break;	// ptr zeigt jetzt auf die naechst kleinere Spannung
		ptr++;
	}
	if (i == 0) {
		/* kleinste Entfernung annehmen, falls reale Entfernung < kleinste bekannte Entfernung */
		*p_sens = SENS_IR_MIN_DIST;	// SENS_IR_INFINITE waere eigentlich besser, das mag aber maze nicht
		/* Sensorupdate-Info toggeln und beenden */
		*p_toggle = (uint8_t) (~*p_toggle);
		return;
	}

	/* Entfernung berechnen und speichern */
	uint8_t distance = lin_interpolate(ctbot_eeprom_read_byte(&(ptr-1)->voltage), ctbot_eeprom_read_byte(&(ptr-1)->dist), tmp, ctbot_eeprom_read_byte(&ptr->dist), volt);
	*p_sens = distance >= SENS_IR_MAX_DIST/5 ? SENS_IR_INFINITE : distance * 5;	// Distanz ist gefuenftelt in den Ref.-Daten;

	/* Sensorupdate-Info toggeln */
	*p_toggle = (uint8_t) (~*p_toggle);
}

/*!
 * Gibt die Eingabedaten des Distanzsensors 1:1 zur Ausgabe
 * @param p_sens	Zeiger auf Ausgabewert
 * @param p_toggle	Zeiger auf die Toggle-Variable des Zielsensors
 * @param ptr		wird nicht ausgewertet
 * @param input		Eingabewert
 */
void sensor_dist_straight(int16_t * const p_sens, uint8_t * const p_toggle, const distSens_t * ptr, int16_t input) {
	(void) ptr;

	/* Ausgabe = Eingabe */
	*p_sens = input;

	/* Sensorupdate-Info toggeln */
	*p_toggle = (uint8_t) (~*p_toggle);
}

/*!
 * Kuemmert sich um die Weiterverarbeitung der rohen Sensordaten
 */
void sensor_update(void) {
#ifndef OS_AVAILABLE
	static uint8_t old_pos = 0;		/*!< Ticks fuer Positionsberechnungsschleife */
#endif
	static uint16_t old_speed = 0;	/* Ticks fuer Geschwindigkeitsberechnungsschleife */
#ifdef MEASURE_MOUSE_AVAILABLE
	static int16_t lastMouseX = 0;	/*!< letzter Mauswert X fuer Positionsberechnung */
	static int16_t lastMouseY = 0;	/*!< letzter Mauswert Y fuer Positionsberechnung */
	static float lastDistance = 0;	/*!< letzte gefahrene Strecke */
	static float lastHead = HEADING_START;		/*!< letzter gedrehter Winkel */
	static float oldHead = HEADING_START;		/*!< Winkel aus dem letzten Durchgang */
	static float old_x = 0;			/*!< Position X aus dem letzten Durchgang */
	static float old_y = 0;			/*!< Position Y aus dem letzten Durchgang */
	float radius = 0;				/*!< errechneter Radius des Drehkreises */
	float s1 = 0;					/*!< Steigung der Achsengerade aus dem letzten Durchgang */
	float s2 = 0;					/*!< Steigung der aktuellen Achsengerade */
	float a1 = 0;					/*!< Y-Achsenabschnitt der Achsengerade aus dem letzten Durchgang */
	float a2 = 0;					/*!< Y-Achsenabschnitt der aktuellen Achsengerade */
	float xd = 0;					/*!< X-Koordinate Drehpunkt */
	float yd = 0;					/*!< Y-Koordinate Drehpunkt */
	float right_radius = 0;			/*!< Radius des Drehkreises des rechten Rads */
	float left_radius = 0;			/*!< Radius des Drehkreises des linken Rads */
#endif // MEASURE_MOUSE_AVAILABLE
	static int16_t lastEncL = 0;	/* letzter Encoderwert links fuer Positionsberechnung */
	static int16_t lastEncR = 0;	/* letzter Encoderwert rechts fuer Positionsberechnung */
	static int16_t lastEncL1 = 0;	/* letzter Encoderwert links fuer Geschwindigkeitsberechnung */
	static int16_t lastEncR1 = 0;	/* letzter Encoderwert rechts fuer Geschwindigkeitsberechnung */
	float dHead = 0.0f;				/* Winkeldifferenz aus Encodern */
	float deltaY = 0.0f;			/* errechneter Betrag Richtungsvektor aus Encodern */
	int16_t diffEncL;				/* Differenzbildung linker Encoder */
	int16_t diffEncR;				/* Differenzbildung rechter Encoder */
	float sl;						/* gefahrene Strecke linkes Rad */
	float sr;						/* gefahrene Strecke rechtes Rad */

#ifdef MEASURE_POSITION_ERRORS_AVAILABLE
	static direction_t last_dir = {{0, 0}};		/* letzte Drehrichtungen der Raeder */
	static position_t last_dir_change = {0, 0}; /* Position des letzten Richtungswechsels */
	static float error_angle = 0.0f;			/* Betrag des Fehlerwinkels [Bogenmass] */
	static int16_t last_error_radius = 0;		/* letzte Fehlerdistanz (Radius) [mm] */
#endif // MEASURE_POSITION_ERRORS_AVAILABLE

#ifdef MEASURE_MOUSE_AVAILABLE
	int16_t dX;						/*!< Differenz der X-Mauswerte */
	int16_t dY;						/*!< Differenz der Y-Mauswerte */
	int8_t modifiedAngles = False;	/*!< Wird True, wenn aufgrund 90 Grad oder 270 Grad die Winkel veraendert werden mussten */

	sensMouseY += sensMouseDY;		/*!< Mausdelta Y aufaddieren */
	sensMouseX += sensMouseDX;		/*!< Mausdelta X aufaddieren */
#endif // MEASURE_MOUSE_AVAILABLE

#ifndef OS_AVAILABLE
	if (timer_ms_passed_8(&old_pos, 10))
#endif
	{
		/* Gefahrene Boegen aus Encodern berechnen */
#ifdef MCU
		uint8_t sreg = SREG;
		__builtin_avr_cli();
#endif
		/* <CS> */
		int16_t sensEncL_tmp = sensEncL;
		int16_t sensEncR_tmp = sensEncR;
		/* </CS> */
#ifdef MCU
		SREG = sreg;
#endif
		diffEncL = sensEncL_tmp - lastEncL;
		diffEncR = sensEncR_tmp - lastEncR;
		if (diffEncL != 0 || diffEncR != 0) {
			lastEncL = sensEncL_tmp;
			lastEncR = sensEncR_tmp;
			sl = diffEncL * ((float) WHEEL_PERIMETER / ENCODER_MARKS);
			sr = diffEncR * ((float) WHEEL_PERIMETER / ENCODER_MARKS);

			/* Winkel berechnen */
			dHead = (sr - sl) / WHEEL_TO_WHEEL_DIAMETER;
			/* Winkel ist hier noch im Bogenmass */
			/* Position berechnen */
			/* dazu Betrag des Vektors berechnen */
			if (dHead == 0.0f) {
				/* Geradeausfahrt, deltaY=diffEncL=diffEncR */
				deltaY = sl;
			} else {
				/* Laenge berechnen aus alpha/2 */
				deltaY = (sl + sr) * sinf(dHead / 2.0f) / dHead;

				/* Winkel in Grad umrechnen */
				dHead = deg(dHead);

				heading_enc += dHead;
				if (heading_enc >= 360) {
					heading_enc -= 360;
				} else if (heading_enc < 0) {
					heading_enc += 360;
				}

#ifndef CMPS03_AVAILABLE
#ifndef MEASURE_MOUSE_AVAILABLE
				heading = heading_enc;
				heading_int = (int16_t) heading_enc;
				heading_10_int = (int16_t) (heading_enc * 10.0f);
#endif // !MEASURE_MOUSE_AVAILABLE
				const float h_enc = rad(heading_enc);
				heading_sin = sinf(h_enc);
				heading_cos = cosf(h_enc);
#endif // CMPS03_AVAILABLE
			}

			if (deltaY != 0.0f) {
				/* neue Positionen berechnen */
				x_enc += deltaY * heading_cos;
				y_enc += deltaY * heading_sin;
#ifndef MEASURE_MOUSE_AVAILABLE
				/* Encoderwerte als Standardwerte benutzen */
				x_pos = (int16_t) x_enc;
				y_pos = (int16_t) y_enc;
#endif // !MEASURE_MOUSE_AVAILABLE
			}

#ifdef MEASURE_POSITION_ERRORS_AVAILABLE
			direction_t dir_change = {{0, 0}};
			dir_change.raw = direction.raw ^ last_dir.raw;

			float head_diff = 0.0f;
			const int16_t x_enc_int = (int16_t) x_enc;
			const int16_t y_enc_int = (int16_t) y_enc;

			if (dir_change.raw != 0) {
				/* Richtungswechsel bei min. einem Rad */
				last_dir = direction;
				if (dir_change.left && dir_change.right) {
					/* Drehrichtung beider Raeder hat sich geaendert */
					head_diff = (float) WHEEL_PERIMETER / ENCODER_MARKS / WHEEL_TO_WHEEL_DIAMETER * 2.0f;
				} else {
					/* Drehrichtung eines Rads hat sich geaendert */
					head_diff = (float) WHEEL_PERIMETER / ENCODER_MARKS / WHEEL_TO_WHEEL_DIAMETER;
				}

//				LOG_DEBUG("head_diff=%f Grad", deg(head_diff));
				error_angle = head_diff;
//				LOG_DEBUG("error_angle=%f Grad", deg(error_angle));
				last_error_radius = pos_error_radius;
//				LOG_DEBUG("last_error_radius=%d mm", last_error_radius);
				last_dir_change.x = x_enc_int;
				last_dir_change.y = y_enc_int;
			}
			const int16_t dist = (int16_t) sqrtf((float) get_dist(x_enc_int, y_enc_int, last_dir_change.x, last_dir_change.y));
			pos_error_radius = (int16_t) (error_angle * (float) dist) + last_error_radius; // Kreisbogenlaenge + bisheriger Fehler
#endif // MEASURE_POSITION_ERRORS_AVAILABLE
		}

#ifdef MEASURE_MOUSE_AVAILABLE
		dX = sensMouseX - lastMouseX;
		/* heading berechnen */
		dHead = (float) dX * (360.0f / (float) MOUSE_FULL_TURN);
		if (dHead != 0.0f) {
			heading_mou += dHead;
			lastHead += dHead;
			if (heading_mou >= 360) {
				heading_mou -= 360;
			} else if (heading_mou < 0) {
				heading_mou += 360;
			}
#ifndef MEASURE_COUPLED_AVAILABLE
			heading = heading_mou;
			heading_int = (int16_t) heading_mou;
			heading_10_int = (int16_t) (heading_mou * 10.0f);
#endif // MEASURE_COUPLED_AVAILABLE
			const float h_mou = rad(heading_mou);
			heading_sin = sinf(h_mou);
			heading_cos = cosf(h_mou);
		}

		/* x/y pos berechnen */
		dY = sensMouseY - lastMouseY;
		if (dY != 0) {
			const float delta = (float) dY * (25.4f / MOUSE_CPI);
			lastDistance += delta;
			x_mou += delta * heading_cos;
			y_mou += delta * heading_sin;

			lastMouseY = sensMouseY;
		}
		lastMouseX = sensMouseX;
#endif // MEASURE_MOUSE_AVAILABLE
#ifdef MEASURE_COUPLED_AVAILABLE
		/* Werte der Encoder und der Maus mit dem Faktor G_POS verrechnen */
		x_pos = (int16_t) (G_POS * x_mou + (1 - G_POS) * x_enc);
		y_pos = (int16_t) (G_POS * y_mou + (1 - G_POS) * y_enc);
		/* Korrektur, falls mou und enc zu unterschiedlichen Seiten zeigen */
		if (fabs(heading_mou - heading_enc) > 180) {
			/* wir nutzen zum Rechnen zwei Drehrichtungen */
			heading = heading_mou <= 180 ? heading_mou * G_POS : (heading_mou - 360) * G_POS;
			heading += heading_enc <= 180 ? heading_enc * (1 - G_POS) : (heading_enc - 360) * (1 - G_POS);
			/* wieder auf eine Drehrichtung zurueck */
			if (heading < 0) {
				heading += 360;
			}
		} else {
			heading = G_POS * heading_mou + (1 - G_POS) * heading_enc;
		}
		if (heading >= 360) {
			heading -= 360;
		}
		heading_int = (int16_t) heading;
		heading_10_int = (int16_t) (heading * 10.0f);
		const float h = rad(heading);
		heading_sin = sinf(h);
		heading_cos = cosf(h);
#else
#ifdef MEASURE_MOUSE_AVAILABLE
		/* Mauswerte als Standardwerte benutzen */
		x_pos = (int16_t) x_mou;
		y_pos = (int16_t) y_mou;
#endif // MEASURE_MOUSE_AVAILABLE
#endif // MEASURE_COUPLED_AVAILABLE

		if (timer_ms_passed_16(&old_speed, SPEED_UPDATE_TIME)) {
			const int16_t diffEncL1 = sensEncL_tmp - lastEncL1;
			const int16_t diffEncR1 = sensEncR_tmp - lastEncR1;
			const float time_correction = (((float) WHEEL_PERIMETER / ENCODER_MARKS) * (1000000.0f / (float) TIMER_STEPS))
				/ MS_TO_TICKS((float) SPEED_UPDATE_TIME);
			v_enc_left = (int16_t) (diffEncL1 * time_correction);
			v_enc_right = (int16_t) (diffEncR1 * time_correction);
			v_enc_center = (v_enc_left + v_enc_right) / 2;
			lastEncL1 = sensEncL_tmp;
			lastEncR1 = sensEncR_tmp;

#ifndef MEASURE_MOUSE_AVAILABLE
			/* Encoderwerte als Standardwerte benutzen */
			v_left = v_enc_left;
			v_right = v_enc_right;
			v_center = v_enc_center;
#endif // MEASURE_MOUSE_AVAILABLE
#ifdef MEASURE_MOUSE_AVAILABLE
			/* Speed aufgrund Maussensormessungen */
			v_mou_center = (int16_t) (lastDistance * (1000 / SPEED_UPDATE_TIME));
			/* Aufteilung auf die Raeder zum Vergleich mit den Radencodern */
			/* Sonderfaelle pruefen */
			if (oldHead == 90 || oldHead == 270 || heading_mou == 90 || heading_mou == 270) {
				float temp;
				/* Winkel um 90 Grad vergroessern */
				oldHead += 90;
				heading_mou += 90;
				// Koordinaten anpassen
				temp = old_x;
				old_x = old_y;
				old_y = -temp;
				temp = x_mou;
				x_mou = y_mou;
				y_mou = -temp;
				modifiedAngles = True;
			}

			/* Steigungen berechnen */
			s1 = -tan(rad(oldHead));
			s2 = -tan(rad(heading_mou));

			/* Geradeausfahrt? (s1==s2) */
			if (s1 == s2) {
				/* Bei Geradeausfahrt ist v_left==v_right==v_center */
				v_mou_left = v_mou_right = v_mou_center;
			} else {
				/* y-Achsenabschnitte berechnen */
				a1 = old_x - s1 * old_y;
				a2 = x_mou - s2 * y_mou;
				/* Schnittpunkt berechnen */
				yd = (a2 - a1) / (s1 - s2);
				xd = s2 * yd + a2;
				/* Radius ermitteln */
				radius = sqrtf((x_mou - xd) * (x_mou - xd) + (y_mou - yd) * (y_mou - yd));
				/* Vorzeichen des Radius feststellen */
				if (lastHead < 0) {
					/* Drehung rechts, Drehpunkt liegt rechts vom Mittelpunkt
					 * daher negativer Radius */
					 radius = -radius;
				}
				if (v_mou_center < 0) {
					/* rueckwaerts => links und rechts vertauscht, daher VZ vom Radius umdrehen */
					radius = -radius;
				}
				/* Geschwindigkeiten berechnen */
				right_radius = radius + WHEEL_DISTANCE;
				left_radius = radius - WHEEL_DISTANCE;
				v_mou_right = (int16_t) (lastHead * right_radius * (4 * 2 * M_PI / 360));
				v_mou_left = (int16_t) (lastHead * left_radius * (4 * 2 * M_PI / 360));
			}
			/* Falls Koordinaten/Winkel angepasst wurden, nun wieder korrigieren */
			if (modifiedAngles) {
				float temp;
				/* Winkel wieder um 90 Grad reduzieren */
				oldHead -= 90;
				heading_mou -= 90;
				/* Koordinaten wieder korrigieren */
				temp = old_x;
				old_x = -old_y;
				old_y = temp;
				temp = x_mou;
				x_mou = -y_mou;
				y_mou = temp;
			}
			lastDistance = 0;
			lastHead = 0;
			old_x = x_mou;
			old_y = y_mou;
			oldHead = heading_mou;
#endif // MEASURE_MOUSE_AVAILABLE
#ifdef MEASURE_COUPLED_AVAILABLE
			v_left = (int16_t) (G_SPEED * v_mou_left + (1 - G_SPEED) * v_enc_left);
			v_right = (int16_t) (G_SPEED * v_mou_right + (1 - G_SPEED) * v_enc_left);
			v_center = (int16_t) (G_SPEED * v_mou_center + (1 - G_SPEED) * v_enc_center);
#else
#ifdef MEASURE_MOUSE_AVAILABLE
			/* Mauswerte als Standardwerte benutzen */
			v_left = v_mou_left;
			v_right = v_mou_right;
			v_center = v_mou_center;
#endif // MEASURE_MOUSE_AVAILABLE
#endif // MEASURE_COUPLED_AVAILABLE
		}
	}
}

/*!
 * Setzt die Auswertungen der Sensordaten zurueck
 */
void sensor_reset(void) {
	/* Radencoder */
	heading_enc = HEADING_START;
	x_enc = 0.0f;
	y_enc = 0.0f;

#ifdef MEASURE_MOUSE_AVAILABLE
	/* Maussensor */
	heading_mou = HEADING_START;
	x_mou = 0.0f;
	y_mou = 0.0f;
#endif // MEASURE_MOUSE_AVAILABLE

	heading = 0.0f;
	heading_int = 0;
	heading_10_int = 0;
	x_pos = 0;
	y_pos = 0;

#ifdef MEASURE_POSITION_ERRORS_AVAILABLE
	direction.raw = (uint8_t) ((~direction.raw) & 0x3);
	pos_error_radius = 0;
#endif
}

/*!
 * Die Funktion gibt aus, ob sich innerhalb einer gewissen Entfernung ein Objekt-Hindernis befindet.
 * @param distance	Entfernung in mm, bis zu welcher ein Objekt gesichtet wird.
 * @return 			Gibt False (0) zurueck, wenn kein Objekt innerhalb von distance gesichtet wird. Ansonsten die Differenz
 * zwischen dem linken und rechten Sensor. Negative Werte besagen, dass das Objekt naeher am linken, positive, dass
 * es naeher am rechten Sensor ist. Sollten beide Sensoren den gleichen Wert haben, gibt die Funktion 1 zurueck, um
 * von False unterscheiden zu koennen.
 */
int16_t is_obstacle_ahead(int16_t distance) {
	if (sensDistL > distance && sensDistR > distance) {
		return False;
	}
	if (sensDistL - sensDistR == 0) {
		return 1;
	}
	return sensDistL - sensDistR;
}

/*!
 * Updatet die LEDs je nach Sensorwert
 */
void led_update(void) {
#ifdef LED_AVAILABLE
#ifndef TEST_AVAILABLE
	if (sensTrans != 0) {
		LED_on(LED_GELB);
	} else {
		LED_off(LED_GELB);
	}
	if (sensError != 0) {
		LED_on(LED_ORANGE);
	} else {
		LED_off(LED_ORANGE);
	}

	if (sensDistL < 500) {
		LED_on(LED_LINKS);
	} else {
		LED_off(LED_LINKS);
	}
	if (sensDistR < 500) {
		LED_on(LED_RECHTS);
	} else {
		LED_off(LED_RECHTS);
	}
#if defined MCU && defined UART_AVAILABLE
	if (uart_infifo.overflow == 1) {
		LED_on(LED_TUERKIS);
	}
#endif // MCU && UART_AVAILABLE
#else // TEST_AVAILABLE
	static volatile uint8_t led_status = 0x00;
	led_t * status = (led_t *) &led_status;
	bit_t tmp;
#ifdef TEST_AVAILABLE_ANALOG
	tmp.byte = (uint8_t) (sensDistR >> 8);
	(*status).rechts = tmp.bit;
	tmp.byte = (uint8_t) (sensDistL >> 8);
	(*status).links = tmp.bit;
	tmp.byte = (uint8_t) (sensLineL >> 9);
	(*status).rot = tmp.bit;
	tmp.byte = (uint8_t) (sensLineR >> 9);
	(*status).orange =  tmp.bit;
	tmp.byte = (uint8_t) (sensLDRL >> 8);
	(*status).gelb = tmp.bit;
	tmp.byte = (uint8_t) (sensLDRR >> 8);
	(*status).gruen = tmp.bit;
	tmp.byte = (uint8_t) (sensBorderL >> 9);
	(*status).tuerkis = tmp.bit;
	tmp.byte = (uint8_t) (sensBorderR >> 9);
	(*status).weiss = tmp.bit;
#endif // TEST_AVAILABLE_ANALOG
#ifdef TEST_AVAILABLE_DIGITAL
	tmp.byte = (uint8_t) sensEncR;
	(*status).rechts = tmp.bit;
	tmp.byte = (uint8_t) sensEncL;
	(*status).links = tmp.bit;
	tmp.byte = sensTrans;
	(*status).rot = tmp.bit;
	tmp.byte = sensError;
	(*status).orange = tmp.bit;
	tmp.byte = sensDoor;
	(*status).gelb = tmp.bit;
#ifdef MOUSE_AVAILABLE
	tmp.byte = (uint8_t) (sensMouseDX >> 1);
	(*status).gruen = tmp.bit;
	tmp.byte = (uint8_t) (sensMouseDY >> 1);
	(*status).tuerkis = tmp.bit;
#endif
#ifdef RC5_AVAILABLE
	tmp.byte = (uint8_t) rc5_ir_data.ir_data;
	(*status).weiss = tmp.bit;
#endif
#endif // TEST_AVAILABLE_DIGITAL

	LED_set(led_status);
#endif // !TEST_AVAILABLE
#endif // LED_AVAILABLE
}

#ifdef DISPLAY_SENSOR_AVAILABLE
/*!
 * Displayhandler fuer Sensoranzeige
 */
void sensor_display(void) {
	display_cursor(1, 1);
	display_printf("P=%03X %03X D=%03d %03d ", sensLDRL, sensLDRR, sensDistL, sensDistR);

	display_cursor(2, 1);
	display_printf("B=%03X %03X L=%03X %03X ", sensBorderL, sensBorderR, sensLineL, sensLineR);

	display_cursor(3, 1);
	display_printf("R=%2d %2d F=%d K=%d T=%d ", sensEncL % 10, sensEncR % 10, sensError, sensDoor, sensTrans);

	display_cursor(4, 1);
#ifdef RC5_AVAILABLE
	static uint16_t RC5_old;
	if (RC5_Code != 0) RC5_old = RC5_Code;
#ifdef MOUSE_AVAILABLE
	display_printf("I=%04X M=%05d %05d", RC5_old, sensMouseX, sensMouseY);
#else
	display_printf("I=%04X", RC5_old);
#endif // MOUSE_AVAILABLE
#else
#ifdef MOUSE_AVAILABLE
	display_printf("M=%05d %05d", sensMouseX, sensMouseY);
#endif // MOUSE_AVAILABLE
#endif // RC5_AVAILABLE
}
#endif // DISPLAY_SENSOR_AVAILABLE

#ifdef DISPLAY_ODOMETRIC_INFO
/*!
 * Displayhandler fuer Odometrieanzeige
 */
void odometric_display(void) {
	/* Zeige Positions- und Geschwindigkeitsdaten */
	display_cursor(1, 1);
	display_printf("heading: %3d.%u  ", heading_int, heading_10_int % 10);
	display_cursor(2, 1);
	display_printf("x: %3d  y: %3d  ", x_pos, y_pos);
	display_cursor(3, 1);
	display_printf("v_l: %3d v_r: %3d  ", v_left, v_right);
#ifdef BPS_AVAILABLE
	display_cursor(4, 1);
	static uint16_t BPS_old = BPS_NO_DATA;
	static uint8_t count = 0;
	if (sensBPS != BPS_NO_DATA || count > 10) {
		BPS_old = sensBPS;
		count = 0;
	}
	count++;

	display_printf("BPS: 0x%04x", BPS_old);
#elif defined MEASURE_MOUSE_AVAILABLE
	display_cursor(4, 1);
	display_printf("squal: %3d v_c: %3d", mouse_get_squal(), (int16_t) v_mou_center);
#else
	display_cursor(4, 1);
#ifdef MEASURE_POSITION_ERRORS_AVAILABLE
	display_printf("v_c: %3d err: %5d", v_center, pos_error_radius);
#else
	display_printf("v_c: %3d", v_center);
#endif // MEASURE_POSITION_ERRORS_AVAILABLE
#endif // BPS_AVAILABLE
}
#endif // DISPLAY_ODOMETRIC_INFO
