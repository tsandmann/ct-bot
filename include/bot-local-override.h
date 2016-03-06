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
 * \file 	bot-local-override.h
 * \brief 	Konstanten, die den Bot an reale Umgebungen anpassen, koennen hier ueberschrieben werden fuer den eigenen Bot.
 * \author 	Timo Sandmann
 * \date 	07.11.2015
 *
 * Um die Default-Konfiguration lokal zur ueberschreiben, kopiert man diese Datei in das Root-Verzeichnis des Projekts
 * (also /bot-local-override.h) und fuegt dort die ueberschrienen Paramter ein gemaess des Beispiels unten. Die Datei
 * bot-local-override.h im Root-Verzeichnis wird von der Versionsverwaltung ignoriert, so dass die lokale Konfiguration
 * durch Repository-Updates nicht ueberschrieben wird.
 */

#ifndef INCLUDE_BOT_LOCAL_OVERRIDE_DUMMY_H_
#define INCLUDE_BOT_LOCAL_OVERRIDE_DUMMY_H_

/* Beispiel, um die default-Konfiguration der Parameter PID_* zu ueberschreiben fuer den eigenen Bot: */
//#undef PID_Kp
//#define PID_Kp	70 /**< PID-Parameter proportional */
//#undef PID_Ki
//#define PID_Ki	10 /**< PID-Parameter intergral */
//#undef PID_Kd
//#define PID_Kd	20 /**< PID-Parameter differential */

#endif /* INCLUDE_BOT_LOCAL_OVERRIDE_DUMMY_H_ */
