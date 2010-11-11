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
 * \file 	behaviour_drive_chess.c
 * \brief	Der Bot spielt Schach
 *
 * Das Schachverhalten ermoeglicht die manuelle Eingabe
 * eines Schachzuges des Spielers 1 (weiss) ueber das Screeninterface. Nach Betaetigen
 * der GO-Taste zieht der Bot-Spieler und der Bot faehrt auf dem Schachbrett diesen
 * Zug nach. Nun ist der menschliche Spieler wieder am Zug via Screeneingabe bzw. drueckt sofort GO,
 * um fuer sich den Zug machen zu lassen. Am besten ist es (weil ja die Schachfiguren nicht visualisiert werden),
 * sich parallel dazu ein echtes Schachbrett aufzubauen und manuell die Zuege nachzuvollziehen. In meinem ersten Test
 * musste ich eine Niederlage gegen den Bot einstecken:-)
 * Fuer das Verhalten ist das Chess-Layout zu laden, damit wenigstens die Schachfelder visualisiert werden und damit die
 * Botfahrten der gegnerischen Schachzuege. Die gruenen Felder stehen hierbei stellvertretend fuer die weissen Felder.
 * Inspiriert zu diesem Verhalten wurde ich durch folgende Seiten:
 * http://bralug.de/wiki/BLIT2008-Board_spielt_Schach
 * Detailliertes zu dem Micro-Max Schach ist hier zu finden:
 * http://home.hccnet.nl/h.g.muller/max-src2.html
 * 
 * Brainstorming fuer moegliche Erweiterungen:
 * Der Bot koennte gegen den anderen spielen ohne Mensch, d.h. Bot 1 ist Spieler 1 und der andere der Gegenspieler.
 * Jeder bot macht jeweils einen Zug und faehrt diesen ab. Danach ist der andere dran. Dies muesste/koennte dann ueber die
 * bot_2_bot-Kommunikation laufen. Noch schoener waere es natuerlich, echte Spielfiguren auf dem Java3D-Schachbrett zu haben
 * (oder zuerst auch einfache Objekte), wobei der Bot sich jeweils die Figur greift (bot_catch_pillar) und an der Zielposition
 * ablaedt (bot_unload_pillar)...
 *
 * \author 	Frank Menzel (Menzelfr@gmx.de), H.G. Muller (Micro-Max Schach)
 * \date 	15.09.2009
 */

#include "bot-logic/bot-logic.h"

#ifdef BEHAVIOUR_DRIVE_CHESS_AVAILABLE
#include "ui/available_screens.h"
#include "log.h"
#include "rc5-codes.h"
#include "display.h"
#include <stdlib.h>
#ifdef MCU
#include <avr/pgmspace.h>
#endif // MCU

//#define DEBUG_CHESS	// Schalter fuer Debugausgaben

#ifndef LOG_AVAILABLE
#undef DEBUG_CHESS
#endif
#ifndef DEBUG_CHESS
#undef LOG_DEBUG
#define LOG_DEBUG(...) {}
#endif

/***************************************************************************/
/* Los geht es mit den benoetigten Routinen fuer das Schachprogramm        */
/* sowie dem Schachprogramm dann selbst                                    */
/***************************************************************************/

/* Digitstellen des Schachzuges fuer Eingabe als auch fuer Anzeige des berechneten Zuges */
static uint8_t d_0 = '0';
static uint8_t d_1 = '0';
static uint8_t d_2 = '0';
static uint8_t d_3 = '0';


/*!
 * Ausgabe einer Stelle des Zuges auf dem Display
 * @param offset Position der anzuzeigenden Stelle
 * @param d      Ausgabezeichen
 */
static void print_move_digit(uint8_t offset, uint8_t d) {
	switch (offset) {
	case 0:
		d_0 = d;
		break;

	case 1:
		d_1 = d;
		break;

	case 2:
		d_2 = d;
		break;

	case 3:
		d_3 = d;
		break;
	}
}


/*!
 * Loeschen der Zug-Anzeigestellen auf dem Display
 */
static void clean_move_digits(void) {
	d_0 = ' ';
	d_1 = ' ';
	d_2 = ' ';
	d_3 = ' ';
}

/*!
 * Anzeige der 4 Zug-Zeichenstellen
 * @param d0 Zeichen an 1. Stelle
 * @param d1 Zeichen an 2. Stelle
 * @param d2 Zeichen an 3. Stelle
 * @param d3 Zeichen an 4. Stelle
 */
static void print_move(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) {
	// globale Vars nur setzen, werden vom Display selbst ausgegeben
	d_0 = d0;
	d_1 = d1;
	d_2 = d2;
	d_3 = d3;
} 


/***************************************************************************/
/*                               micro-Max,                                */
/* A chess program smaller than 2KB (of non-blank source), by H.G. Muller  */
/***************************************************************************/
/* version 4.8 (1953 characters) features:                                 */
/* - recursive negamax search                                              */
/* - all-capture MVV/LVA quiescence search                                 */
/* - (internal) iterative deepening                                        */
/* - best-move-first 'sorting'                                             */
/* - a hash table storing score and best move                              */
/* - futility pruning                                                      */
/* - king safety through magnetic, frozen king in middle-game              */
/* - R=2 null-move pruning                                                 */
/* - keep hash and repetition-draw detection                               */
/* - better defense against passers through gradual promotion              */
/* - extend check evasions in inner nodes                                  */
/* - reduction of all non-Pawn, non-capture moves except hash move (LMR)   */
/* - full FIDE rules (expt under-promotion) and move-legality checking     */

#define M 0x88                              /* Unused bits in valid square */  
#define S 128                               /* Sign bit of char            */
#define I 8000                              /* Infinity score              */
#define U 18                                /* D() Stack array size        */

static struct {
	short q, l, e;							/* Args: (q,l)=window, e=current eval. score */
	short m, v, 							/* m=value of best move so far, v=current evaluation */
	V, P;
	unsigned char E, z, n; 					/* Args: E=e.p. sqr.; z=level 1 flag; n=depth */
	signed char r; 							/* step vector current ray */
	unsigned char j, 						/* j=loop over directions (rays) counter */
	B, d, 									/* B=board scan start, d=iterative deepening counter */
	h, C, 									/* h=new ply depth?, C=ply depth? */
	u, p, 									/* u=moving piece, p=moving piece type */
	x, y,									/* x=origin square, y=target square of current move */
	F, 										/* F=e.p., castling skipped square */
	G, 										/* G=castling R origin (corner) square */
	H, t, 									/* H=capture square, t=piece on capture square */
	X, Y, 									/* X=origin, Y=target square of best move so far */
	a; 										/* D() return address state */
} _, A[U], *J = A + U; 						/* _=working set, A=stack array, J=stack pointer */

static short Q,                             /* pass updated eval. score    */
K,                                          /* input move, origin square   */
i,s,                                        /* temp. evaluation term       */
Dq,Dl,De,                                   /* D() arguments               */
DD;                                         /* D() return value            */

const signed char w[] PROGMEM = {
	0, 2, 2, 7, -1, 8, 12, 23
};                      								/*!< relative piece values */

const signed char o[] PROGMEM = {
	-16,-15,-17,0,1,16,0,1,16,15,17,0,14,18,31,33,0,	/* step-vector lists */
	7,-1,11,6,8,3,6,									/* 1st dir. in o[] per piece */
	6,3,5,7,4,5,3,6										/* initial piece setup */
};
  
static unsigned char L,                     /* input move, target square */
b[129],                                     /* board: half of 16x8+dummy */
c[5],                                       /* input move ASCII buffer */
k,                                          /* moving side 8=white 16=black */
O,                                          /* pass e.p. flag at game level */
R,                                          /* captured non pawn material */
DE,Dz,Dn,                                   /* D() arguments */
Da,                                         /* D() state */
W = 99;                                     /* @ temporary */
/*hv;*/                                     /* zeige Hauptvariante, d.h automat. Zugermittlung oder nicht */


//*********************************************************
static unsigned char st = 2; /*!< Spielstufe (Level) */
static long timer;        	 /*!< wird alle 4.096ms erhoeht bzw. durch Increment */

//static int16_t MaxMoves = 40, MaxTime = 30 * 40;
static int16_t moves;
//static long nodes, newNodes;

#define F_NEWGAME	'3'
#define F_LEVEL		'.'
#define F_FAVORIT	'?'
#define CL			'9'
#define GO			':'

#ifdef PC
#define o(ndx)	(signed char)o[ndx]
#define w(ndx)	(signed char)w[ndx]
#else
#define o(ndx)	(signed char)pgm_read_byte(o + (ndx))
#define w(ndx)	(signed char)pgm_read_byte(w + (ndx))
#endif // PC


/* better readability of working struct variables */
#define q _.q
#define l _.l
#define e _.e
#define E _.E
#define z _.z
#define n _.n
#define m _.m
#define v _.v
#define V _.V
#define P _.P
#define r _.r
#define j _.j
#define B _.B
#define d _.d
#define h _.h
#define C _.C
#define u _.u
#define p _.p
#define x _.x
#define y _.y
#define F _.F
#define G _.G
#define H _.H
#define t _.t
#define X _.X
#define Y _.Y
#define a _.a

/*!
 * fuehrt die Schach-Zuege aus
 */
static void D(void) { 														/* iterative Negamax search */
	D: if (--J < A) { 														/* stack pointer decrement and underrun check */
		++J;
		DD = -l;
		goto R;																/* simulated return */
	}
	q = Dq;
	l = Dl;
	e = De;
	E = DE;
	z = Dz;
	n = Dn; 																/* load arguments */
	a = Da; 																/* load return address state */

	--q; 																	/* adj. window: delay bonus */
	k ^= 24; 																/* change sides */
	d = X = Y = 0; 															/* start iter. from scratch */
	while (d++ < n || d < 3 || 												/* iterative deepening loop */
		((z & (K == I)) && (((timer < 0) & (d < 98) || 						/* root: deepen upto time */
		(K = X, L = (unsigned char) (Y & ~M), d = 3))))) {					/* time's up: go do best */

		x = B = X; 															/* start scan at prev. best */
		h = (unsigned char) (Y & S);										/* request try noncastl. 1st */
		if (d < 3) {
			P = I;
		}
		else {
			*J = _;
			Dq = -l;
			Dl = 1 - l;
			De = -e;
			DE = S;
			Dz = 0;
			Dn = (unsigned char) (d - 3);									/* save locals, arguments */
			Da = 0;
			goto D; 														/* Search null move */
			R0: _ = *J;
			P = DD; 														/* load locals, return value */
		}
		m = (-P < l) | (R > 35) ? d > 2 ? -I : e : -P; 						/* Prune or stand-pat */
		++timer; 															/* node count (for timing) */
		do {
			u = b[x]; 														/* scan board looking for */
			if (u & k) {													/* own piece (inefficient!) */
				p = (unsigned char) (u & 7); 								/* p = piece type (set r>0) */
				r = (signed char) p;
				j = (unsigned char) (o(p+16));								/* first step vector f.piece */
				while ((r = (signed char) ((p > 2) & (r < 0) ? -r :
					-o(++j)))) {											/* loop over directions o[] */
					A: 														/* resume normal after best */
					y = x;
					F = G = S; 												/* (x,y)=move, (F,G)=castl.R */
					do { 													/* y traverses ray, or: */
						H = y = (unsigned char) (h ? Y ^ h : y + r); 		/* sneak in prev. best move */
						if (y & M) {
							break; 											/* board edge hit */
						}
						m = (E - S) & b[E] && (y - E < 2) &
							((E - y) < 2) ? I : m;							/* bad castling */
						if ((p < 3) & (y == E)) {
							H ^= 16; 										/* shift capt.sqr. H if e.p.*/
						}
						t = b[H];
						if ((t & k) | ((p < 3) & (!((y - x) & 7) - !t))) {
							break;	 										/* capt. own, bad pawn mode */
						}
						i = 37 * w(t&7) + (t & 192); 						/* value of capt. piece t */
						m = i < 0 ? I : m; 									/* K capture */
						if ((m >= l) & (d > 1)) {
							goto J;
						}													/* abort on fail high */

						v = d - 1 ? e : i - p; 								/* MVV/LVA scoring */
						if (d - !t > 1) {									/* remaining depth */
							v = p < 6 ? b[x + 8] - b[y + 8] : 0; 			/* center positional pts. */
							b[G] = b[H] = b[x] = 0;
							b[y] = u | 32; 									/* do move, set non-virgin */
							if (!(G & M)) {
								b[F] = (unsigned char) (k + 6);				/* castling: put R & score */
								v += 50;
							}
							v -= (p - 4) | (R > 29) ? 0 : 20; 				/* penalize mid-game K move */
							if (p < 3) {									/* pawns: */
								v -= 9 * (((x - 2 )& M || (b[x - 2] - u)) + /* structure, undefended */
								((x + 2) & M || (b[x + 2] - u)) - 1 		/* squares plus bias */
								+ (b[x ^ 16] == k + 36)) 					/* kling to non-virgin King */
								- (R >> 2); 								/* end-game Pawn-push bonus */
								V = (y + r + 1) & S ? 647 - p : 2 * (u & (y + 16) & 32); /* promotion or 6/7th bonus */
								b[y] = (unsigned char) (b[y] + V);
								i += V; 									/* change piece, add score */
							}
							v += e + i;
							V = m > q ? m : q; 								/* new eval and alpha */
							C = (unsigned char) (d - 1 - ((d > 5) &
								(p > 2) & !t & !h));
							C = (unsigned char) ((R > 29) | (d < 3) |		/* extend 1 ply if in check */
								(P - I) ? C : d);
							do
								if ((C > 2) | (v > V)) {
									*J = _;
									Dq = -l;
									Dl = -V;
									De = -v;
									DE = F;
									Dz = 0;
									Dn = C; 								/* save locals, arguments  */
									Da = 1;
									goto D; 								/* iterative eval. of reply */
									R1: _ = *J;
									s = -DD; 								/* load locals, return value */
								} else {
									s = v; 									/* or fail low if futile */
								}
							while ((s > q) & (++C < d));
							v = s;
							if (z && K - I && v + I &&
								(x == K) & (y == L)) {						/* move pending & in root: */
								Q = -e - i;
								O = F;										/* exit if legal & found */
								R = (unsigned char) (R + (i >> 7));
								++J;
								DD = l;
								goto R;
																			/* captured non-P material */
							}
							b[G] = (unsigned char) (k + 6);
							b[F] = b[y] = 0;
							b[x] = u;
							b[H] = t; 										/* undo move,G can be dummy */
						}
						if (v > m) {										/* new best, update max,best */
							m = v;
							X = x;
							Y = (unsigned char) (y | (S & F));				/* mark double move with S */
						}
						if (h) {
							h = 0;
							goto A;
						} 													/* redo after doing old best */
						if ((x + r - y) | (u & 32) | 						/* not 1st step,moved before */
							((p > 2) & ((p - 4) | (j - 7) ||				/* no P & no lateral K move, */
							b[G = (unsigned char) ((x + 3) ^
							(r >> 1 & 7))] - k - 6							/* no virgin R in corner G, */
							|| b[G ^ 1] | b[G ^ 2]))) {						/* no 2 empty sq. next to R */
							t = (unsigned char) (t + (p < 5));				/* fake capt. for nonsliding */
						} else {
							F = y;											/* enable e.p. */
						}
					} while (!t);											/* if not capt. continue ray */
				}
			}
		} while ((x = (unsigned char) ((x + 9) & ~M)) - B);					/* next sqr. of board, wrap */
		J: if ((m > I - M) | (m < M - I)) {
			d = 98; 														/* mate holds to any depth */
		}
		m = (m + I) | (P == I) ? m : 0; 									/* best loses K: (stale)mate */
/*		if(z & hv & d > 2) {
			print_move('a'+(X&7), '8'-(X>>4), 'a'+(Y&7), '8'-(Y>>4&7));
		}
*/
	}																		/* encoded in X S,8 bits */
	k ^= 24;																/* change sides back */
	++J;
	DD = m += m < e;														/* delayed-loss bonus */
	R: if (J != A + U) {
		switch (a) {
		case 0:
			goto R0;
		case 1:
			goto R1;
		}
	} else {
		return;
	}
}



/* *****************************************************************************
 * Ende der Original-Schachroutinen
 * Beginn der Routinen des Verhaltens und natuerlich Verhalten selbst
 *******************************************************************************/
#ifdef PC
#define OFFSET 240; /*!< Offsetwert, um genau von einer Sim-Schachfeld-Position zur naechsten zu kommen */
#else // MCU
// fuer MCU anpassen je nach Groesse des echten Schachbrettes
#define OFFSET 240; /*!< Offsetwert, um genau von einer Sim-Schachfeld-Position zur naechsten zu kommen */
#endif // PC

static uint8_t state = 0;	/*!< Status des Verhaltens */
static uint8_t white;		/*!< Kennung welcher Spieler gerade dran ist, nur Schwarz wird vom Bot gefahren */

/*!
 * Umwandeln der Zug-Zeichenstellen in Bot-Worldkoordinaten zum Abfahren des Zuges auf dem Sim-Schachfeld
 * @param x_c 1. Stelle der Zugangabe (a bei Zug a2)
 * @param y_c 2. Stelle der Zugangabe (2 bei Zug a2)
 * @param *wx x-Koordinate fuer Bot-Zielposition
 * @param *wy y-Koordinate fuer Bot-Zielposition
 */
static void chess2worldkoords(char x_c, char y_c, int16_t * wx, int16_t * wy) {
	// aus Klein- Grossbuchstaben machen und Offsetwert zu-/abrechnen
	*wx = (x_c - 97) * -OFFSET; // nach Rechts x-Richtung, Werte werden negativ und um 1 Offsetfeld verschoben
	*wx = *wx - OFFSET
	*wy = (y_c - 49) * OFFSET; // nach Oben y-Richtung, Werte positiver aber um 1 Offset-Feld verschoben
	*wy = *wy + OFFSET;
}

/*!
 * Aufgerufene Routine nach Taste GO; der eingegebene Zug wird geparst bzw. ein neuer Schachzug ermittelt
 * Ist die manuelle Zugeingabe erforderlich oder der Zug war nicht gueltig, erscheint auf dem Display ZU G?
 * Routine wurde etwas von der Original-Schachroutine fuer diese Zwecke abgewandelt, enthaelt aber auch deren
 * Initialisierungen, die man wohl nicht unbedingt verstehen muss bzw. erklaeren kann:-) Naeheres wohl auf der
 * o.a. Website 
 * @result Rueckgabe der ausgefuehrten Aktion bzw. Status nach GO 
 *   1: manueller Zug geparst und gueltig;
 *   2: Zug vom Prog berechnet;
 *   0: sonst (Zug nicht gueltig)
 */
static uint8_t chess_go(void) {
	uint8_t result;

	result = 0;

	if (*c - GO) {
		/* parse entered move */
		K = *c - 16 * c[1] + 799;
		L = (unsigned char) (c[2] - 16 * c[3] + 799);
		LOG_DEBUG("Zug wurde eingegeben und parsen");
	} else {
		K = I;
		timer = -128 << st; // set time control
		clean_move_digits();
		LOG_DEBUG("Ich denke...");
	}

	Dq = -I;
	Dl = I;
	De = Q;
	DE = O;
	Dz = 1;
	Dn = 3; // store arguments of D()
	Da = 0; // state

	D();
	if (*c - GO) { // Zugeingabe vorhanden
		if (I == DD) { // Zug ist gueltig
			LOG_DEBUG("eing. Zug war gueltig");
			result = 1;
		} else { // .. ungueltig
			print_move('Z', 'U', 'G', '?');
			LOG_DEBUG("Zug? war nicht gueltig");
		}
		*c = GO;
	} else { // Computerzug
		result = 2;
		++moves; // Zuganzahl erhoehen
		print_move((uint8_t) ('a' + (K & 7)), (uint8_t) ('8' - (K >> 4)), (uint8_t) ('a' + (L & 7)), (uint8_t) ('8' - (L >> 4 & 7))); // Displayausgabe
		LOG_DEBUG("Computerzug Zug %1d", moves);
		LOG_DEBUG("Zug aus D() von %c %c nach %c %c", 'a' + (K & 7), '8' - ( K >> 4), 'a' + (L & 7), '8' - (L >> 4 & 7));
		if (!(DD > -I + 1)) {
			print_move('M', 'A', 'T', 'T'); // Displayausgabe
			LOG_DEBUG("-- SCHACH MATT! --");
		}
	}

	return result;
}

/*!
 * Initialisierungen fuer neues Spiel
 * aus Originalquelle uebernommen, naeheres dazu sicher auf o.a. Website
 */
static void new_game_init(void) {
	LOG_DEBUG("Neues Spiel");
	print_move('S', 'H', 'A', 'H');

	k = 16;
	Q = R = 0;
	O = (unsigned char) Q;
	for (W = 0; W < sizeof b; ++W) {
		b[W] = 0;
	}
	for (W = 0; W < sizeof c; ++W) {
		c[W] = 0; // damit beim naechsten Start auch wieder Zugeingabe ausgegeben wird
	}
	W = 8;
	while (W--) { // initial board setup
		b[W] = (unsigned char) ((b[W + 112] = (unsigned char) (o(W+24) + 8)) + 8);

		b[W + 16] = 18;
		b[W + 96] = 9;
		L = 8;
		while (L--) {
			b[16 * L + W + 8] = (unsigned char) ((W - 4) * (W - 4) + (L + L - 7) * (L + L - 7) / 4); // center-pts table
		}
	} // (in unused half b[])

	W = 4;
	moves = 0; // Anzahl Zuege ruecksetzen
	white = 0; // Spieler init.
}


// Status des Verhaltens
#define NEWGAME_STATE 0
#define GO_STATE      1
#define GO_STATE_NEXT 2
#define CO_END		  99


/*!
 * Das eigentliche Schach-Fahrverhalten
 * Fuer den Bot-Spieler Schwarz wird nach dessen Zugberechnung der Zug vom Bot nachgefahren, faehrt also von der Zug-Startposition
 * zur Zug-Endposition. Das Verhalten ist dann damit abgeschlossen
 * @param *data	Der Verhaltensdatensatz
 */
void bot_drive_chess_behaviour(Behaviour_t * data) {
	int16_t wx; // zur Umwandlung des Zuges in Botkoordinaten
	int16_t wy;
	uint8_t res_go; //Status nach GO

	switch (state) {
	case NEWGAME_STATE:
		/* Neues Spiel wird gestartet */
		LOG_DEBUG("-NEWGAME-");
		state = GO_STATE;
		new_game_init();
		break;

	case GO_STATE:
		/* Einsprung fuer GO nach Zugeingabe */
		LOG_DEBUG("GO STATE");
		state = CO_END;

		res_go = chess_go();

		if (res_go == 1) { // nach man. Zug gleich Computerzug hinterher
			white = (uint8_t) ! white;
			LOG_DEBUG("gleich Compi ziehen lassen, Zug war OK, weiss %1d", white);

			state = GO_STATE; // gleichen Verhaltensstatus aufrufen

		} else {
			if (res_go == 2) {
				white = (uint8_t) ! white;
				if (white) {
					LOG_DEBUG("Weiss-keine Botbewegung");
				} else { // nur wenn Schwarz an der Reihe ist Bot bewegen
					LOG_DEBUG("Bot fahren nach Autozug");
					LOG_DEBUG("Zug aus D() von %c %c nach %c %c", 'a' + (K & 7), '8' - (K >> 4), 'a' + ( L& 7), '8' - (L >> 4 & 7));
					chess2worldkoords((char) ('a' + (K & 7)), (char) ('8' - (K >> 4 & 7)), &wx, &wy);
					LOG_DEBUG("gehe zu Zug-Startpos %1d,%1d,weiss %1d", wy, wx, white);
					bot_goto_pos(data, wy, wx, 999);

					state = GO_STATE_NEXT;
					break;
				}
			}
		}
		break;

	case GO_STATE_NEXT:
		/* Bot wird zu den Zug-Zielkoordinaten gefahren */
		chess2worldkoords((char) ('a' + (L & 7)), (char) ('8' - (L >> 4 & 7)), &wx, &wy);
		LOG_DEBUG("gehe zu Zug-Zielpos %1d,%1d,weiss %1d", wy, wx, white);
		bot_goto_pos(data, wy, wx, 999);

		state = CO_END; // nach Abfahren Verhaltensende

		break;

	default:
		exit_behaviour(data, SUBFAIL);
		return;
	}
}

/*!
 * Neustart des eigentlichen Schach-Fahrverhalten
 * Fuer den Bot-Spieler schwarz wird nach dessen Zugberechnung der Zug vom Bot nachgefahren, faehrt also von der Zug-Startposition
 * zur Zug-Endposition. Das Verhalten ist dann damit abgeschlossen
 * @param *caller	Der Verhaltensdatensatz
 */
void bot_drive_chess(Behaviour_t * caller) {
	switch_to_behaviour(caller, bot_drive_chess_behaviour, OVERRIDE);
	state = NEWGAME_STATE;
}

/*!
 * Start des Schach-Fahrverhalten nach GO-Betaetigung, d.h. nach Eingabe des Zuges wurde GO betaetigt
 * @param *caller	Der Verhaltensdatensatz
 */
void bot_drive_chess_go(Behaviour_t * caller) {
	switch_to_behaviour(caller, bot_drive_chess_behaviour, OVERRIDE);
	state = GO_STATE;
}

/*!
 * Routine fuer manuelle Zugeingabe (Tastaturauswertung)
 */
static void after_input(void) {
	if (!(W & 1)) { // Umwandlung Zahl->Zeichen an je 1. Stelle, also an je ungerader Position
		c[W] = (unsigned char) (c[W] + 'a' - '1'); // W ist hierbei der Digit-Positionszaehler des Digits fuer den Zug
	}
	switch (W) {
	default:
		print_move_digit(W, c[W]);
		++W;
		break;
	case 4: // getkey() anzeigen
		clean_move_digits();
		c[0] = c[4];
		print_move_digit(0, c[0]);
		W = 1;
		break;
	case 5: // getkey() nicht anzeigen
		clean_move_digits();
		W = 0;
		break;
	}
}

/*!
 * Keyhandler fuer das Chess-Verhalten
 */
#ifdef DISPLAY_DRIVE_CHESS_AVAILABLE
static void drivechess_disp_key_handler(void) {
	switch (RC5_Code) {
	case RC5_CODE_STOP:
		/* Newgame */
		RC5_Code = 0;
		c[W] = F_NEWGAME;
		// Verhaltensstart Newgame
		bot_drive_chess(NULL);
		break;

	case RC5_CH_PLUS:
		/* Level veraendern */
		RC5_Code = 0;
		c[W] = F_LEVEL;
		st++;
		LOG_DEBUG("Level erhoeht auf %1d", st);
		break;

	case RC5_CH_MINUS:
		/* Level veraendern */
		RC5_Code = 0;
		c[W] = F_LEVEL;
		st--;
		LOG_DEBUG("Level verringert auf %1d", st);
		break;

	case RC5_CODE_BLUE:
		/* Zugzeile loeschen */
		RC5_Code = 0;
		c[W] = CL;
		LOG_DEBUG("CLEAR %c", c[W]);
		after_input();
		clean_move_digits();
		W = 0;
		break;

	case RC5_CODE_PLAY:
		/* GO */
		RC5_Code = 0;
		if (W == 99) {
			LOG_DEBUG("vor GO erstmaliges INIT");
			//new_game_init();
			bot_drive_chess(NULL);
		} else {
			// Verhaltensstart im Status GO
			bot_drive_chess_go(NULL);
		}
		c[W] = GO;
		break;

	default:
		/* wird immer durchlaufen fuer alle anderen Tasten als oben angegeben */
		/* hier nur noch die Tasten fuer man. Zugeingabe rauspicken */
		switch (RC5_Code) {
		case RC5_CODE_1:
			c[W] = '1';
			after_input();
			RC5_Code = 0;
			break;
		case RC5_CODE_2:
			c[W] = '2';
			after_input();
			RC5_Code = 0;
			break;
		case RC5_CODE_3:
			c[W] = '3';
			after_input();
			RC5_Code = 0;
			break;
		case RC5_CODE_4:
			c[W] = '4';
			after_input();
			RC5_Code = 0;
			break;
		case RC5_CODE_5:
			c[W] = '5';
			after_input();
			RC5_Code = 0;
			break;
		case RC5_CODE_6:
			c[W] = '6';
			after_input();
			RC5_Code = 0;
			break;
		case RC5_CODE_7:
			c[W] = '7';
			after_input();
			RC5_Code = 0;
			break;
		case RC5_CODE_8:
			c[W] = '8';
			after_input();
			RC5_Code = 0;
			break;
		case RC5_CODE_9:
			c[W] = '9';
			after_input();
			RC5_Code = 0;
			break;
		} //switch

		break;
	} // switch
} // Ende Keyhandler
 
/*!
 * Display zum Steuern des Schachverhaltens
 */
void drive_chess_display(void) {
	display_cursor(1, 1);
	display_printf("Chess Level %1d",st);
	display_cursor(2, 1);
	display_printf("Zug : %c%c   %c%c",d_0,d_1,d_2,d_3);
	display_cursor(3, 1);
	display_puts("CL BLUE/Lv CH+-");
	display_cursor(4, 1);
	display_puts("Go PLAY/New STOP");

	drivechess_disp_key_handler(); // aufrufen des Key-Handlers
}
#endif	// DISPLAY_DRIVE_CHESS_AVAILABLE

#endif // BEHAVIOUR_DRIVE_CHESS_AVAILABLE
