/*
 *                   Uwe Berger; 2010
 *                  =================
 * Einige Defines, die hauptsaechlich den uBasic-Sprachumfang und die Zielplattform steuern.
 *
 * In der Regel bedeutet:
 * 0 --> nicht vorhanden/abgeschaltet
 * 1 --> vorhanden/angeschaltet
 *
 */

/**
 * \file 	ubasic_config.h
 * \brief 	uBasic-Konfiguration
 * \author 	Uwe Berger (bergeruw@gmx.net)
 * \date 	10.06.2010
 */

#ifndef __UBASIC_CONFIG_H__
#define __UBASIC_CONFIG_H__

#ifdef BEHAVIOUR_UBASIC_AVAILABLE
#include "log.h"

// regulaere Standardausgabe
#define PRINTF(...) LOG_INFO(__VA_ARGS__)

// max. Stringlaenge (Basic)
#define MAX_STRINGLEN 40

// max. Schachtelungstiefe fuer GOSUB (Basic)
#define MAX_GOSUB_STACK_DEPTH 2

// max. Schachtelungstiefe fuer FOR-NEXT (Basic)
#define MAX_FOR_STACK_DEPTH 4

// max. Anzahl Variablen (Basic)
#define MAX_VARNUM 26

// max. Laenge von Funktions- und Variablennamen in call(), vpeek() und vpoke()
#define MAX_NAME_LEN 15

// einige Basic-Erweiterungen, die man nicht immer unbedingt benoetigt
#define UBASIC_ABS		1
#define UBASIC_NOT		1
#define UBASIC_CALL 	1
#define UBASIC_CVARS	1
#define UBASIC_REM		1

// exit(1) in Fehlersituationen macht sich bei AVRs etwas schlecht...
#define BREAK_NOT_EXIT	1

// Verwendung des AVR-PROGMEM fuer einige Daten
#define USE_PROGMEM		1

// damit es fuer den Bot auch in Basic das WAIT gibt
#define BOT_WAIT        1

// AVR-spezifischen Befehle an-/abwaehlen
#define AVR_WAIT		0
#ifdef BOT_WAIT
#undef AVR_WAIT // wenn es BOT_WAIT gibt, dann fuer AVR ausschalten
#endif
#define AVR_EPEEK		0
#define AVR_EPOKE		0
#define AVR_DIR			0
#define AVR_IN			0
#define AVR_OUT			0
#define AVR_ADC			0
#define AVR_RND			0

// AVR-Ports fuer Basic-Befehle dir, in, out
#define HAVE_PORTA		0
#define HAVE_PORTB		0
#define HAVE_PORTC		0
#define HAVE_PORTD		0

// AVR: Anzahl der ADC-Eingaenge (0...ACD_COUNT_MAX)
#define ADC_COUNT_MAX	4

#endif // BEHAVIOUR_UBASIC_AVAILABLE
#endif // __UBASIC_CONFIG_H__
