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
 * \file 	rc5-codes.h
 * \brief 	RC5-Fernbedienungscodes
 * \author 	Andreas Merkle (mail@blue-andi.de)
 * \date 	15.02.2006
 */

#ifndef RC5CODE_H_
#define RC5CODE_H_

#define RC5_TOGGLE	0x0800 /**< Das RC5-Toggle-Bit */
#define RC5_ADDRESS	0x07C0 /**< Der Adressbereich */
#define RC5_COMMAND	0x103F /**< Der Kommandobereich */

/* Im Normalbetrieb hilft die Beschraenkung der RC5_MASK auf RC5_COMMAND dem Bot, moeglichst viele FBs zu erkennen.
 * Zum erfassen neuer Codes sollte man unbedingt RC5_MASK auf (RC5_COMMAND | RC5_ADDRESS) setzen */

// Normalbetrieb
#define RC5_MASK (RC5_COMMAND)					/**< Welcher Teil des Kommandos wird ausgewertet? */
// Erfassen neuer FB-Codes
//#define RC5_MASK (RC5_COMMAND | RC5_ADDRESS)	/**< Welcher Teil des Kommandos wird ausgewertet? */


/**
 * Definition RC5-Codes verschiedener Fernbedienungen. Wer eine neue FB
 * einfuegen will, sollte
 *
 * 	- 	eine Definition per cut&paste duplizieren,
 * 	- 	eine geeignete Konstante RC_HAVE_{Herstellername}_{Fernbedienungsname}
 * 		waehlen,
 * 	-	diese Konstante in die obere Liste der FB-Selektion eintragen,
 * 	-	die alte FB-Konstante im kopierten Bereich ersetzen,
 * 	-	die eigentlichen, herausgefunden Codes eintragen.
 *
 * Ist das erledigt, und funktioniert die neue FB-Definition, kann diese
 * in der c't-Bot-Mailingliste vorgestellt werden. (mb/18.03.2006)
 */

#ifdef MCU
/** Dies ist die Standard-Fernbedienung fuer den Bot */
#define RC_HAVE_HQ_RC_UNIVERS29_334

//#define RC_HAVE_HAUPPAUGE_WINTV
//#define RC_HAVE_HAUPPAUGE_MediaMPV
//#define RC_HAVE_CONRAD_PROMO8
//#define RC_HAVE_VIVANCO_UR89
//#define RC_HAVE_VIVANCO_UR89_TV_CODE_089
//#define RC_HAVE_Technisat_TTS35AI
//#define RC_HAVE_LIFETEC_LT3607
//#define RC_HAVE_TOTAL_CONTROL
#else // PC
/** Dies ist die Standard-Fernbedienung fuer den Sim */
#define RC_HAVE_HQ_RC_UNIVERS29_334
//#define RC_HAVE_HAUPPAUGE_WINTV
#endif // MCU


/**
 * Default-Fernbedienung
 * HQ RC Univers 29, Geraetecode 334
 */
#ifdef RC_HAVE_HQ_RC_UNIVERS29_334
#define RC5_CODE_PWR		(0x118C & RC5_MASK)		/**< Taste An/Aus */

#define RC5_CODE_0			(0x1180 & RC5_MASK)		/**< Taste 0 / 10 */
#define RC5_CODE_1			(0x1181 & RC5_MASK)		/**< Taste 1 */
#define RC5_CODE_2			(0x1182 & RC5_MASK)		/**< Taste 2 */
#define RC5_CODE_3			(0x1183 & RC5_MASK)		/**< Taste 3 */
#define RC5_CODE_4			(0x1184 & RC5_MASK)		/**< Taste 4 */
#define RC5_CODE_5			(0x1185 & RC5_MASK)		/**< Taste 5 */
#define RC5_CODE_6			(0x1186 & RC5_MASK)		/**< Taste 6 */
#define RC5_CODE_7			(0x1187 & RC5_MASK)		/**< Taste 7 */
#define RC5_CODE_8			(0x1188 & RC5_MASK)		/**< Taste 8 */
#define RC5_CODE_9			(0x1189 & RC5_MASK)		/**< Taste 9 */
#define RC5_CODE_11			(0x118A & RC5_MASK)		/**< Taste 11 / 1- */
#define RC5_CODE_12			(0x11A3 & RC5_MASK)		/**< Taste 12 / 2- */

#define RC5_CODE_GREEN		(0x01BA & RC5_MASK)		/**< Gruene Taste */
#define RC5_CODE_RED		(0x01BD & RC5_MASK)		/**< Rote Taste */
#define RC5_CODE_YELLOW		(0x01B1 & RC5_MASK)		/**< Gelbe Taste */
#define RC5_CODE_BLUE		(0x01B0 & RC5_MASK)		/**< Blaue Taste */

#define RC5_CODE_I_II		(0x11AB & RC5_MASK)		/**< I/II-Taste */
#define RC5_CODE_TV_VCR		(0x11B8 & RC5_MASK)		/**< TV/VCR-Taste */

#define RC5_CODE_DOT		(0x11B7 & RC5_MASK)		/**< Taste mit rundem Punkt */

#define RC5_CODE_PLAY		(0x11B5 & RC5_MASK)		/**< PLAY-Taste */
#define RC5_CODE_STILL		(0x11A9 & RC5_MASK)		/**< Pause Taste */
#define RC5_CODE_STOP		(0x11B6 & RC5_MASK)		/**< Stopp Taste */
#define RC5_CODE_BWD		(0x11B2 & RC5_MASK)		/**< Backward Taste */
#define RC5_CODE_FWD		(0x11B4 & RC5_MASK)		/**< Forward Taste */

#define RC5_CODE_CH_PC		(0x11BF & RC5_MASK)		/**< CH*P/C Taste */
#define RC5_CODE_MUTE		(0x01BF & RC5_MASK)		/**< Mute-Taste */

#define RC5_VOL_PLUS		(0x1190 & RC5_MASK)		/**< Vol + Taste */
#define RC5_VOL_MINUS		(0x1191 & RC5_MASK)		/**< Vol - Taste */

#define RC5_CH_PLUS			(0x11A0 & RC5_MASK)		/**< Ch + Taste */
#define RC5_CH_MINUS		(0x11A1 & RC5_MASK)		/**< Ch - Taste */


#define RC5_CODE_UP			RC5_CODE_STILL			/**< Taste Hoch */
#define RC5_CODE_DOWN		RC5_CODE_STOP			/**< Taste Runter */
#define RC5_CODE_LEFT		RC5_CODE_BWD			/**< Taste Links */
#define RC5_CODE_RIGHT		RC5_CODE_FWD			/**< Taste Rechts */
#endif // RC_HAVE_HQ_RC_UNIVERS29_334

/**
 * Fernbedienung Hauppauge (simple WinTV-Karten Fernbedienung)
 */
#ifdef RC_HAVE_HAUPPAUGE_WINTV
#define RC5_CODE_0			(0x1000 & RC5_MASK)		/**< Taste 0 */
#define RC5_CODE_1			(0x1001 & RC5_MASK)		/**< Taste 1 */
#define RC5_CODE_2			(0x1002 & RC5_MASK)		/**< Taste 2 */
#define RC5_CODE_3			(0x1003 & RC5_MASK)		/**< Taste 3 */
#define RC5_CODE_4			(0x1004 & RC5_MASK)		/**< Taste 4 */
#define RC5_CODE_5			(0x1005 & RC5_MASK)		/**< Taste 5 */
#define RC5_CODE_6			(0x1006 & RC5_MASK)		/**< Taste 6 */
#define RC5_CODE_7			(0x1007 & RC5_MASK)		/**< Taste 7 */
#define RC5_CODE_8			(0x1008 & RC5_MASK)		/**< Taste 8 */
#define RC5_CODE_9			(0x1009 & RC5_MASK)		/**< Taste 9 */

#define RC5_CODE_PWR		(0x1026 & RC5_MASK)		/**< Taste Minimize */
#define RC5_CODE_FULL		(0x102E & RC5_MASK)		/**< Taste Full Screen */
#define RC5_CODE_SOURCE		(0x1022 & RC5_MASK)		/**< Taste Source */

#define RC5_CODE_UP			(0x1020 & RC5_MASK)		/**< Taste CH + */
#define RC5_CODE_DOWN		(0x1021 & RC5_MASK)		/**< Taste CH - */
#define RC5_CODE_LEFT		(0x1011 & RC5_MASK)		/**< Taste VOL- */
#define RC5_CODE_RIGHT		(0x1010 & RC5_MASK)		/**< Taste VOL+ */

#define RC5_CODE_I_II		RC5_CODE_SOURCE
#define RC5_CODE_TV_VCR		RC5_CODE_FULL
#define RC5_CODE_MUTE		RC5_CODE_FULL
#endif // RC_HAVE_HAUPPAUGE_WINTV

/**
 * Fernbedienung Hauppauge erweitert
 */
#ifdef RC_HAVE_HAUPPAUGE_MediaMPV
#define RC5_CODE_0			(0x17C0 & RC5_MASK)		/**< Taste 0 */
#define RC5_CODE_1			(0x17C1 & RC5_MASK)		/**< Taste 1 */
#define RC5_CODE_2			(0x17C2 & RC5_MASK)		/**< Taste 2 */
#define RC5_CODE_3			(0x17C3 & RC5_MASK)		/**< Taste 3 */
#define RC5_CODE_4			(0x17C4 & RC5_MASK)		/**< Taste 4 */
#define RC5_CODE_5			(0x17C5 & RC5_MASK)		/**< Taste 5 */
#define RC5_CODE_6			(0x17C6 & RC5_MASK)		/**< Taste 6 */
#define RC5_CODE_7			(0x17C7 & RC5_MASK)		/**< Taste 7 */
#define RC5_CODE_8			(0x17C8 & RC5_MASK)		/**< Taste 8 */
#define RC5_CODE_9			(0x17C9 & RC5_MASK)		/**< Taste 9 */

#define RC5_CODE_UP			(0x17E0 & RC5_MASK)		/**< Taste CH + */
#define RC5_CODE_DOWN		(0x17E1 & RC5_MASK)		/**< Taste CH - */
#define RC5_CODE_LEFT		(0x17D1 & RC5_MASK)		/**< Taste VOL- */
#define RC5_CODE_RIGHT		(0x17D0 & RC5_MASK)		/**< Taste VOL+ */

#define RC5_CODE_OK			(0x17E5 & RC5_MASK)		/**< Taste OK */

#define RC5_CODE_PWR   		(0x17FD & RC5_MASK)		/**< Taste An/Aus */

#define RC5_CODE_RED   		(0x17CB & RC5_MASK)		/**< Taste Rot */
#define RC5_CODE_GREEN		(0x17EE & RC5_MASK)		/**< Taste Gruen */
#define RC5_CODE_YELLOW		(0x17F8 & RC5_MASK)		/**< Taste Gelb */
#define RC5_CODE_BLUE		(0x17E9 & RC5_MASK)		/**< Taste Blau */

#define RC5_CODE_FWD		(0x17F4 & RC5_MASK)		/**< Taste >> */
#define RC5_CODE_BWD		(0x17F2 & RC5_MASK)		/**< Taste << */
#define RC5_CODE_PLAY		(0x17F5 & RC5_MASK)		/**< Taste > */
#define RC5_CODE_RECORD		(0x17F7 & RC5_MASK)		/**< Taste Aufnahme */
#define RC5_CODE_STOP		(0x17F6 & RC5_MASK)		/**< Taste Stop */
#define RC5_CODE_WAIT		(0x17F0 & RC5_MASK)		/**< Taste Pause */
#define RC5_CODE_REPLAY		(0x17E4 & RC5_MASK)		/**< Taste Anfang |< */
#define RC5_CODE_SKIP		(0x17DE & RC5_MASK)		/**< Taste Ende >| */

#define RC5_CODE_MUTE		(0x17CF & RC5_MASK)		/**< Taste Mute */
#define RC5_CODE_VIEW		(0x17CC & RC5_MASK)		/**< Taste View zwischen Mute und Full */
#define RC5_CODE_FULL		(0x17FC & RC5_MASK)		/**< Taste Full */

#define RC5_CODE_BACK		(0x17DF & RC5_MASK)		/**< Taste Back/Exit */
#define RC5_CODE_MENU		(0x17CD & RC5_MASK)		/**< Taste Menue */
#define RC5_CODE_GO			(0x17FB & RC5_MASK)		/**< Taste GO */

#define RC5_CODE_TV_VCR		RC5_CODE_VIEW
#define RC5_CH_PLUS			RC5_CODE_BWD		/**< Taste fuer Transprtfach schliessen */
#define RC5_CH_MINUS 		RC5_CODE_FWD		/**< Taste fuer Transportfach oeffnen */
#endif // RC_HAVE_HAUPPAUGE_MediaMPV

/**
 * Fernbedienung Conrad Promo 8
 */
#ifdef RC_HAVE_CONRAD_PROMO8
#define RC5_CODE_0			(0x1000 & RC5_MASK)		/**< Taste 0 */
#define RC5_CODE_1			(0x1001 & RC5_MASK)		/**< Taste 1 */
#define RC5_CODE_2			(0x1002 & RC5_MASK)		/**< Taste 2 */
#define RC5_CODE_3			(0x1003 & RC5_MASK)		/**< Taste 3 */
#define RC5_CODE_4			(0x1004 & RC5_MASK)		/**< Taste 4 */
#define RC5_CODE_5			(0x1005 & RC5_MASK)		/**< Taste 5 */
#define RC5_CODE_6			(0x1006 & RC5_MASK)		/**< Taste 6 */
#define RC5_CODE_7			(0x1007 & RC5_MASK)		/**< Taste 7 */
#define RC5_CODE_8			(0x1008 & RC5_MASK)		/**< Taste 8 */
#define RC5_CODE_9			(0x1009 & RC5_MASK)		/**< Taste 9 */

#define RC5_CODE_UP			(0x0010 & RC5_MASK)		/**< Taste Hoch */
#define RC5_CODE_DOWN		(0x0011 & RC5_MASK)		/**< Taste Runter */
#define RC5_CODE_LEFT		(0x0015 & RC5_MASK)		/**< Taste Links */
#define RC5_CODE_RIGHT		(0x0016 & RC5_MASK)		/**< Taste Rechts */

#define RC5_CODE_ENTER 		(0x0017 & RC5_MASK)  	/**< Enter-Taste*/
#define RC5_CODE_PWR   		(0x100C & RC5_MASK)     /**< Enter Taste als Ersatz fuer Taste An/Aus */

#define RC5_CODE_RED		(0x002B & RC5_MASK)		/**< Rote Taste */
#define RC5_CODE_GREEN		(0x002C & RC5_MASK)		/**< Gruene Taste */
#define RC5_CODE_YELLOW		(0x002D & RC5_MASK)		/**< Gelbe Taste */
#define RC5_CODE_BLUE		(0x002E & RC5_MASK)		/**< Blaue Taste */
#define RC5_CODE_VIEW		(0x0012 & RC5_MASK)		/**< Instant View Taste */


#define RC5_CODE_SELECT		(0x100D & RC5_MASK)		/**< Select Taste */
#define RC5_CODE_BWD		(0x1011 & RC5_MASK)		/**< Backward Taste */
#define RC5_CODE_FWD		(0x1010 & RC5_MASK)		/**< Forward Taste */

#define RC5_CODE_CH_PC		(0x1010 & RC5_MASK)		/**< Taste neben dem Minus */
#define RC5_CH_PLUS			(0x1020 & RC5_MASK)		/**< Ch + Taste */
#define RC5_CH_MINUS		(0x1021 & RC5_MASK)		/**< Ch - Taste */

#define RC5_CODE_AV			(0x1038 & RC5_MASK)		/**< Taste AV */
#define RC5_CODE_DOT		RC5_CODE_AV

#define RC5_CODE_I_II		RC5_CODE_SELECT
#define RC5_CODE_TV_VCR		RC5_CODE_VIEW
#define RC5_CODE_MUTE		RC5_CODE_ENTER
#endif // RC_HAVE_CONRAD_PROMO8

/**
 * Fernbedienung VIVANCO UR89, vor Verwendung auf VCR druecken
 * \author Andreas Staudenmayer
 */
#ifdef RC_HAVE_VIVANCO_UR89
#define RC5_CODE_0			(0x1140 & RC5_MASK)		/**< Taste 0 			   */
#define RC5_CODE_1			(0x1141 & RC5_MASK)		/**< Taste 1 			   */
#define RC5_CODE_2			(0x1142 & RC5_MASK)		/**< Taste 2 			   */
#define RC5_CODE_3			(0x1143 & RC5_MASK)		/**< Taste 3 			   */
#define RC5_CODE_4			(0x1144 & RC5_MASK)		/**< Taste 4 			   */
#define RC5_CODE_5			(0x1145 & RC5_MASK)		/**< Taste 5 			   */
#define RC5_CODE_6			(0x1146 & RC5_MASK)		/**< Taste 6 			   */
#define RC5_CODE_7			(0x1147 & RC5_MASK)		/**< Taste 7 			   */
#define RC5_CODE_8			(0x1148 & RC5_MASK)		/**< Taste 8 			   */
#define RC5_CODE_9			(0x1149 & RC5_MASK)		/**< Taste 9 			   */

#define RC5_CODE_PWR		(0x114C & RC5_MASK)		/**< Taste An, Aus         */

#define RC5_CODE_UP			(0x1160 & RC5_MASK)		/**< Taste Hoch            */
#define RC5_CODE_DOWN		(0x1161 & RC5_MASK)		/**< Taste Runter          */
#define RC5_CODE_LEFT		(0x0171 & RC5_MASK)		/**< Taste Links           */
#define RC5_CODE_RIGHT		(0x0170 & RC5_MASK)		/**< Taste Rechts          */

#define RC5_CODE_RED		(0x1172 & RC5_MASK)		/**< rote Taste            */
#define RC5_CODE_GREEN		(0x1176 & RC5_MASK)		/**< gruene Taste          */
#define RC5_CODE_YELLOW		(0x1175 & RC5_MASK)		/**< gelbe Taste           */
#define RC5_CODE_BLUE		(0x1174 & RC5_MASK)		/**< blaue Taste           */
#endif // RC_HAVE_VIVANCO_UR89

/**
 * Fernbedienung VIVANCO UR89, TV Modus (Alle Tasten funktionieren)
 * vor Verwendung fuer TV den Code 089 auf der Fernbedienung programmieren
 * \author Ulrich Scheffler
 */
#ifdef RC_HAVE_VIVANCO_UR89_TV_CODE_089
/* Jede Taste bekommt erstmal die Bezeichnung, die aufgedruckt ist */
#define RC5_CODE_PWR						(0x100C & RC5_MASK)		/**< Taste An/Aus-Symbol (rot)*/
#define RC5_CODE_MUTE						(0x100D & RC5_MASK)		/**< Taste Mute-Symbol (gruen) */
#define RC5_CODE_CH_PLUS					(0x1020 & RC5_MASK)		/**< Taste CH +  (blau) */
#define RC5_CODE_CH_MINUS					(0x1021 & RC5_MASK)		/**< Taste CH -  (blau) */
#define RC5_CODE_VOL_MINUS					(0x1011 & RC5_MASK)		/**< Taste VOL - (blau) */
#define RC5_CODE_VOL_PLUS					(0x1010 & RC5_MASK)		/**< Taste VOL + (blau) */
#define RC5_CODE_BOX_WITH_DOT				(0x0017 & RC5_MASK)		/**< Taste Quadrat mit Punkt */
#define RC5_CODE_TV_VCR						(0x1038 & RC5_MASK)		/**< Taste TV/VCR (gleicher Code wie Taste A.B) */
#define RC5_CODE_0							(0x1000 & RC5_MASK)		/**< Taste 0 */
#define RC5_CODE_1							(0x1001 & RC5_MASK)		/**< Taste 1 */
#define RC5_CODE_2							(0x1002 & RC5_MASK)		/**< Taste 2 */
#define RC5_CODE_3							(0x1003 & RC5_MASK)		/**< Taste 3 */
#define RC5_CODE_4							(0x1004 & RC5_MASK)		/**< Taste 4 */
#define RC5_CODE_5							(0x1005 & RC5_MASK)		/**< Taste 5 */
#define RC5_CODE_6							(0x1006 & RC5_MASK)		/**< Taste 6 */
#define RC5_CODE_7							(0x1007 & RC5_MASK)		/**< Taste 7 */
#define RC5_CODE_8							(0x1008 & RC5_MASK)		/**< Taste 8 */
#define RC5_CODE_9							(0x1009 & RC5_MASK)		/**< Taste 9 */
#define RC5_CODE_RETURN						(0x1022 & RC5_MASK)  	/**< Taste Return-Symbol (gleicher Code wie Taste EXIT) */
#define RC5_CODE_A_DOT_B					(0x1038 & RC5_MASK)		/**< Taste A.B (gleicher Code wie Taste TV/VCR) */
#define RC5_CODE_WAIT						(0x002F & RC5_MASK)		/**< Taste Pause-Symbol */
#define RC5_CODE_GREEN						(0x002C & RC5_MASK)		/**< Taste Gruen & Stop-Symbol */
#define RC5_CODE_RECORD						(0x102E & RC5_MASK)		/**< Taste Aufnahme-Symbol */
#define RC5_CODE_BOX						(0x103F & RC5_MASK)		/**< Taste Quadrat */
#define RC5_CODE_RED						(0x002B & RC5_MASK)		/**< Taste Rot  & << */
#define RC5_CODE_YELLOW						(0x002D & RC5_MASK)		/**< Taste Gelb & >  */
#define RC5_CODE_BLUE						(0x002E & RC5_MASK)		/**< Taste Blau & >> */
#define RC5_CODE_BOX_WITH_3_EQUAL_LINES		(0x103C & RC5_MASK)		/**< Taste Quadrat mit 3 gleichlangen Linien */
#define RC5_CODE_GREEN_UP					(0x0010 & RC5_MASK)		/**< Taste hoch   (gruen) */
#define RC5_CODE_GREEN_LEFT					(0x0015 & RC5_MASK)		/**< Taste links  (gruen) */
#define RC5_CODE_GREEN_RIGHT				(0x0016 & RC5_MASK)		/**< Taste rechts (gruen) */
#define RC5_CODE_GREEN_DOWN					(0x0011 & RC5_MASK)		/**< Taste runter (gruen) */
#define RC5_CODE_BOX_WITH_BOX				(0x1029 & RC5_MASK)		/**< Taste Quadrat mit innerem Rechteck und Pfeilen */
#define RC5_CODE_BOX_WITH_3_UNEQUAL_LINES	(0x102E & RC5_MASK)		/**< Taste Quadrat mit 3 ungleichlangen Linien */
#define RC5_CODE_OK							(0x1023 & RC5_MASK)		/**< Taste OK (gruen)  */
#define RC5_CODE_MENU						(0x0012 & RC5_MASK)		/**< Taste MENU */
#define RC5_CODE_EXIT						(0x1022 & RC5_MASK)		/**< Taste EXIT (gleicher Code wie Taste Return) */

/* Vorhandene Tasten werden hier mit der Wunsch-Funktion belegt (Umwidmung)*/
#define RC5_CODE_UP		RC5_CODE_CH_PLUS		/**< Taste CH +  wird genutzt fuer UP-Funktion */
#define RC5_CODE_DOWN	RC5_CODE_CH_MINUS		/**< Taste CH -  wird genutzt fuer  DOWN-Funktion */
#define RC5_CODE_LEFT	RC5_CODE_VOL_MINUS		/**< Taste VOL - wird genutzt fuer LEFT-Funktion */
#define RC5_CODE_RIGHT	RC5_CODE_VOL_PLUS		/**< Taste VOL + wird genutzt fuer RIGHT-Funktion */
#define RC5_CODE_VIEW	RC5_CODE_TV_VCR			/**< Taste TV/VCR & A.B werden genutzt fuer VIEW-Funktion*/
#define RC5_CODE_SELECT	RC5_CODE_RETURN			/**< Taste Return & Exit werden genutzt fuer SELECT-Funktion */
#define RC5_CODE_BWD	RC5_CODE_GREEN_LEFT		/**< Taste links  (gruen) wird genutzt fuer BWD-Funktion (backward)*/
#define RC5_CODE_FWD	RC5_CODE_GREEN_RIGHT	/**< Taste rechts (gruen) wird genutzt fuer FWD-Funktion (forward)*/
#define RC5_CODE_I_II	RC5_CODE_SELECT			/**< Taste I/II wird genutzt fuer SELECT-Funktion */
#endif // RC_HAVE_VIVANCO_UR89_TV_CODE_089

/**
 * Fernbedienung Technisat_TTS35AI (Receiver Digit CIP)
 * \author Joerg Bullmann
 */
#ifdef RC_HAVE_Technisat_TTS35AI
#define RC5_CODE_0		(0x1289 & RC5_MASK)		/**< Taste 0 			   */
#define RC5_CODE_1		(0x1281 & RC5_MASK)		/**< Taste 1 			   */
#define RC5_CODE_2		(0x1282 & RC5_MASK)		/**< Taste 2 			   */
#define RC5_CODE_3		(0x1283 & RC5_MASK)		/**< Taste 3 			   */
#define RC5_CODE_4		(0x1284 & RC5_MASK)		/**< Taste 4 			   */
#define RC5_CODE_5		(0x1285 & RC5_MASK)		/**< Taste 5 			   */
#define RC5_CODE_6		(0x1286 & RC5_MASK)		/**< Taste 6 			   */
#define RC5_CODE_7		(0x1287 & RC5_MASK)		/**< Taste 7 			   */
#define RC5_CODE_8		(0x1288 & RC5_MASK)		/**< Taste 8 			   */
#define RC5_CODE_9		(0x1289 & RC5_MASK)		/**< Taste 9 			   */

#define RC5_CODE_PWR	(0x128C & RC5_MASK)		/**< Taste An, Aus         */
#define RC5_CODE_INFO	(0x028F & RC5_MASK)		/**< Taste i               */
#define RC5_CODE_OK		(0x0297 & RC5_MASK)		/**< Taste ok              */

#define RC5_CODE_UP		(0x12A0 & RC5_MASK)		/**< Taste Hoch            */
#define RC5_CODE_DOWN	(0x12A1 & RC5_MASK)		/**< Taste Runter          */
#define RC5_CODE_LEFT	(0x1291 & RC5_MASK)		/**< Taste Links           */
#define RC5_CODE_RIGHT	(0x1290 & RC5_MASK)		/**< Taste Rechts          */

#define RC5_CODE_TV		(0x0293 & RC5_MASK)		/**< Taste TV              */
#define RC5_CODE_MENU	(0x0292 & RC5_MASK)		/**< Taste Menu            */
#define RC5_CODE_RED	(0x02AB & RC5_MASK)		/**< rote Taste            */
#define RC5_CODE_GREEN	(0x02AC & RC5_MASK)		/**< gruene Taste          */
#define RC5_CODE_YELLOW	(0x02AD & RC5_MASK)		/**< gelbe Taste           */
#define RC5_CODE_BLUE	(0x02AE & RC5_MASK)		/**< blaue Taste           */

#define RC5_CODE_FWD	RC5_CODE_TV				/**< Taste TV - umgewidmet als FWD-Taste */
#define RC5_CODE_BWD	RC5_CODE_MENU			/**< Taste Menu - umgewidmet als BWD-Taste */
#define RC5_CODE_TV_VCR	RC5_CODE_INFO			/**< Taste INFO - umgewidmet als View-Taste */
#define RC5_CODE_I_II	RC5_CODE_OK				/**< Taste OK - umgewidmet als Select-Taste */
#endif // RC_HAVE_Technisat_TTS35AI

/**
 * Fernbedienung Lifetec LT3607 (aeltere, lernfaehige Medion-Fernbedienung)
 */
#ifdef RC_HAVE_LIFETEC_LT3607
#define RC5_CODE_0		(0x1000 & RC5_MASK)		/**< Taste 0 */
#define RC5_CODE_1		(0x1001 & RC5_MASK)		/**< Taste 1 */
#define RC5_CODE_2		(0x1002 & RC5_MASK)		/**< Taste 2 */
#define RC5_CODE_3		(0x1003 & RC5_MASK)		/**< Taste 3 */
#define RC5_CODE_4		(0x1004 & RC5_MASK)		/**< Taste 4 */
#define RC5_CODE_5		(0x1005 & RC5_MASK)		/**< Taste 5 */
#define RC5_CODE_6		(0x1006 & RC5_MASK)		/**< Taste 6 */
#define RC5_CODE_7		(0x1007 & RC5_MASK)		/**< Taste 7 */
#define RC5_CODE_8		(0x1008 & RC5_MASK)		/**< Taste 8 */
#define RC5_CODE_9		(0x1009 & RC5_MASK)		/**< Taste 9 */

#define RC5_CODE_PWR	(0x100C & RC5_MASK)		/**< Taste Power */
#define RC5_CODE_VIEW	(0x102B & RC5_MASK)		/**< Taste OSD */
#define RC5_CODE_SELECT	(0x100A & RC5_MASK)		/**< Taste -/-- */
#define RC5_CH_PLUS		(0x1020 & RC5_MASK)		/**< Ch + Taste */
#define RC5_CH_MINUS	(0x1021 & RC5_MASK)		/**< Ch - Taste */

#define RC5_CODE_UP		(0x102E & RC5_MASK)		/**< Taste mit Punkt, zwischen VCR-Tasten */
#define RC5_CODE_DOWN	(0x1012 & RC5_MASK)		/**< Taste Bildschirm mit Kreuz oben rechts */
#define RC5_CODE_LEFT	(0x103F & RC5_MASK)		/**< Taste Bildschirm, leer */
#define RC5_CODE_RIGHT	(0x103C & RC5_MASK)		/**< Taste Videotext */
#define RC5_CODE_I_II	(0x1038 & RC5_MASK)		/**< A/B-Taste */
#define RC5_CODE_FWD	(0x1010 & RC5_MASK)		/**< Taste Vol+ */
#define RC5_CODE_BWD	(0x1011 & RC5_MASK)		/**< Taste Vol- */
#define RC5_CODE_RED	(0x1037 & RC5_MASK)		/**< Taste FRWD / Rot */
#define RC5_CODE_GREEN	(0x1036 & RC5_MASK)		/**< Taste STOP / Gruen */
#define RC5_CODE_YELLOW	(0x1032 & RC5_MASK)		/**< Taste PLAY / Gelb */
#define RC5_CODE_BLUE	(0x1034 & RC5_MASK)		/**< Taste FFWD / Blau */
#define RC5_CODE_TV_VCR	(0x100D	& RC5_MASK)		/**< Taste Mute */
#define RC5_CODE_MUTE	RC5_CODE_TV_VCR
#endif // RC_HAVE_LIFETEC_LT3607

/**
 * Fernbedienung Total Control
 */
#ifdef RC_HAVE_TOTAL_CONTROL
/*
 * Alle Tasten mit den Hex-Werten zur Info
 *
 * Power Key-Value	= 100c
 * 1  Key-Value		= 1001
 * 2  Key-Value		= 1002
 * 3  Key-Value		= 1003
 * 4  Key-Value		= 1004
 * 5  Key-Value		= 1005
 * 6  Key-Value		= 1006
 * 7  Key-Value		= 1007
 * 8  Key-Value		= 1008
 * 9  Key-Value		= 1009
 * 0  Key-Value		= 1000
 * Menu Key-Value	= 0012
 * Plus Key-Value	= 1020
 * Minus Key-Value	= 1021
 * more Key-Value	= 1010
 * less Key-Value	= 1011
 * |<  Key-Value	= 100d
 * up  Key-Value	= 0010
 * down Key-Value	= 0011
 * <  Key-Value		= 0015
 * >  Key-Value		= 0016
 * Enter Key-Value	= 0017
 * AV  Key-Value	= 1038
 * <<  Key-Value	= 103c
 * Play Key-Value	= 103f
 * ||  Key-Value	= 1029
 * >>  Key-Value	= 102e
 * red  Key-Value	= 002b
 * green Key-Value	= 002c
 * yellow Key-Value	= 002d
 * blue Key-Value	= 002e
 * -/-- Key-Value	= 100a
 */
#define RC5_CODE_0		(0x1000 & RC5_MASK) /**< Taste 0 - rc5.c */
#define RC5_CODE_1		(0x1001 & RC5_MASK) /**< Taste 1 - rc5.c */
#define RC5_CODE_2		(0x1002 & RC5_MASK) /**< Taste 2 - rc5.c */
#define RC5_CODE_3		(0x1003 & RC5_MASK) /**< Taste 3 - rc5.c */
#define RC5_CODE_4		(0x1004 & RC5_MASK) /**< Taste 4 - rc5.c */
#define RC5_CODE_5		(0x1005 & RC5_MASK) /**< Taste 5 - rc5.c */
#define RC5_CODE_6		(0x1006 & RC5_MASK) /**< Taste 6 - rc5.c */
#define RC5_CODE_7		(0x1007 & RC5_MASK) /**< Taste 7 - rc5.c */
#define RC5_CODE_8		(0x1008 & RC5_MASK) /**< Taste 8 - rc5.c */
#define RC5_CODE_9		(0x1009 & RC5_MASK) /**< Taste 9 - rc5.c */
#define RC5_CODE_UP		(0x0010 & RC5_MASK) /**< Taste Hoch - rc5.c/behaviour_remotecall.c */
#define RC5_CODE_DOWN	(0x0011 & RC5_MASK) /**< Taste Runter - rc5.c/behaviour_remotecall.c */
#define RC5_CODE_LEFT	(0x0015 & RC5_MASK) /**< Taste Links - rc5.c */
#define RC5_CODE_RIGHT	(0x0016 & RC5_MASK) /**< Taste Rechts - rc5.c */
#define RC5_CODE_PWR	(0x100C & RC5_MASK) /**< Enter Taste als Ersatz fuer Taste An/Aus - rc5.c */
#define RC5_CODE_PLAY	(0x0017 & RC5_MASK) /**< Enter-Taste - gui.c/behaviour_ubasic.c/behaviour_remotecall.c */
#define RC5_CODE_STOP	(0x100d & RC5_MASK) /**< Taste - gui.c/behaviour_ubasic.c/behaviour_remotecall.c */
#define RC5_CODE_RED	(0x002B & RC5_MASK) /**< Rote Taste - rc5.c */
#define RC5_CODE_GREEN	(0x002C & RC5_MASK) /**< Gruene Taste - rc5.c */
#define RC5_CODE_YELLOW	(0x002D & RC5_MASK) /**< Gelbe Taste - rc5.c */
#define RC5_CODE_BLUE	(0x002E & RC5_MASK) /**< Blaue Taste - rc5.c */
#define RC5_CODE_CH_PC	(0x0012 & RC5_MASK) /**< Taste Menu - rc5.c */
#define RC5_CH_PLUS		(0x1020 & RC5_MASK) /**< Ch + Taste - rc5.c */
#define RC5_CH_MINUS	(0x1021 & RC5_MASK) /**< Ch - Taste - rc5.c */
#define RC5_CODE_I_II	(0x100A & RC5_MASK) /**< Taste I/II - rc5.c */
#define RC5_CODE_TV_VCR	(0x1038 & RC5_MASK) /**< Tase AV - rc5.c */
#define RC5_CODE_DOT	RC5_CODE_YELLOW		/**< Gelbe Taste - gui.c */
#define RC5_CODE_ENTER	(0x0017 & RC5_MASK) /**< Enter-Taste */
#define RC5_CODE_MUTE	RC5_CODE_ENTER		/**< Enter Taste - behaviour_calbirate_sharps.c */
/* nicht verwendete Codes: */
//#define RC5_CODE_AV		(0x1038 & RC5_MASK) /**< Taste AV */
//#define RC5_VOL_PLUS		(0x1010 & RC5_MASK) /**< Vol + Taste */
//#define RC5_VOL_MINUS		(0x1011 & RC5_MASK) /**< Vol - Taste */
//#define RC5_CODE_REPLAY	(0x100D & RC5_MASK) /**< Taste Anfang */
//#define RC5_CODE_SELECT	(0x0017 & RC5_MASK) /**< Enter Taste */
//#define RC5_CODE_VIEW		(0x0012 & RC5_MASK) /**< Instant View Taste */
//#define RC5_CODE_STILL	(0x1029 & RC5_MASK) /**< Pause Taste */
//#define RC5_CODE_STOP		(0x1029 & RC5_MASK) /**< Stopp Taste */
//#define RC5_CODE_PLAY		(0x103f & RC5_MASK) /**< PLAY-Taste */
//#define RC5_CODE_BWD		(0x103C & RC5_MASK) /**< Backward Taste */
//#define RC5_CODE_FWD		(0x102E & RC5_MASK) /**< Forward Taste */
#endif // RC_HAVE_TOTAL_CONTROL


#ifndef RC5_CODE_1
#define RC_HAVE_DEFAULT
#endif

/**
 * Default-Philips-Fernbedienung
 */
#ifdef RC_HAVE_DEFAULT	/**< Default RC5-Codes falls keine FB definiert wurde */
#define RC5_CODE_0			(0x3940 & RC5_MASK)		/**< Taste 0 */
#define RC5_CODE_1			(0x3941 & RC5_MASK)		/**< Taste 1 */
#define RC5_CODE_2			(0x3942 & RC5_MASK)		/**< Taste 2 */
#define RC5_CODE_3			(0x3943 & RC5_MASK)		/**< Taste 3 */
#define RC5_CODE_4			(0x3944 & RC5_MASK)		/**< Taste 4 */
#define RC5_CODE_5			(0x3945 & RC5_MASK)		/**< Taste 5 */
#define RC5_CODE_6			(0x3946 & RC5_MASK)		/**< Taste 6 */
#define RC5_CODE_7			(0x3947 & RC5_MASK)		/**< Taste 7 */
#define RC5_CODE_8			(0x3948 & RC5_MASK)		/**< Taste 8 */
#define RC5_CODE_9			(0x3949 & RC5_MASK)		/**< Taste 9 */

#define RC5_CODE_UP			(0x2950 & RC5_MASK)		/**< Taste Hoch */
#define RC5_CODE_DOWN		(0x2951 & RC5_MASK)		/**< Taste Runter */
#define RC5_CODE_LEFT		(0x2955 & RC5_MASK)		/**< Taste Links */
#define RC5_CODE_RIGHT		(0x2956 & RC5_MASK)		/**< Taste Rechts */

#define RC5_CODE_PWR		(0x394C & RC5_MASK)		/**< Taste An/Aus */

#define RC5_CODE_RED		(0x100B & RC5_MASK)		/**< Rote Taste */
#define RC5_CODE_GREEN		(0x102E & RC5_MASK)		/**< Gruene Taste */
#define RC5_CODE_YELLOW		(0x1038 & RC5_MASK)		/**< Gelbe Taste */
#define RC5_CODE_BLUE		(0x1029 & RC5_MASK)		/**< Blaue Taste */
#define RC5_CODE_VIEW		(0x000F & RC5_MASK)		/**< Instant View Taste */

#define RC5_CODE_SELECT		(0x100B & RC5_MASK)		/**< Select Taste */

#define RC5_CODE_BWD		(0x1025 & RC5_MASK)		/**< Backward Taste */
#define RC5_CODE_FWD		(0x1026 & RC5_MASK)		/**< Forward Taste */

#define RC5_CODE_TV_VCR	    RC5_CODE_VIEW
#endif // RC_HAVE_DEFAULT

#endif // RC5CODE_H_
