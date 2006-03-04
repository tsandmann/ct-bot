/*
 * c't-Sim - Robotersimulator fuer den c't-Bot
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

/*! @file 	rc5-codes.h
 * @brief 	RC5-Fernbedienungscodes
 * @author 	Andreas Merkle (mail@blue-andi.de)
 * @date 	15.02.06
 * Wer diese Datei angepasst hat, kann Sie durch einen .cvsignore Eintrag schützen. 
 * Dann überschreibt Eclipse Sie nicht mehr automatisch
*/

#ifndef RC5CODE_H_
#define RC5CODE_H_

/*!
 * Fernbedienung mit Jog-Dial-Rad, 
 * Achtung: Die Adress-Bits muessen auf die Fernbedienung angepasst werden!
 * Siehe hierzu rc5.c @see RC5_ADDRESS
 */
#define noJOG_DIAL

#define RC5_MASK (RC5_COMMAND)	/*!< Welcher Teil des Kommandos wird ausgewertet? */

#define RC5_CODE_0	(0x3940 & RC5_MASK)		/*!< Taste 0 */
#define RC5_CODE_1	(0x3941 & RC5_MASK)		/*!< Taste 1 */
#define RC5_CODE_2	(0x3942 & RC5_MASK)		/*!< Taste 2 */
#define RC5_CODE_3	(0x3943 & RC5_MASK)		/*!< Taste 3 */
#define RC5_CODE_4	(0x3944 & RC5_MASK)		/*!< Taste 4 */
#define RC5_CODE_5	(0x3945 & RC5_MASK)		/*!< Taste 5 */
#define RC5_CODE_6	(0x3946 & RC5_MASK)		/*!< Taste 6 */
#define RC5_CODE_7	(0x3947 & RC5_MASK)		/*!< Taste 7 */
#define RC5_CODE_8	(0x3948 & RC5_MASK)		/*!< Taste 8 */
#define RC5_CODE_9	(0x3949 & RC5_MASK)		/*!< Taste 9 */

#define RC5_CODE_UP	(0x2950 & RC5_MASK)	/*!< Taste Hoch */
#define RC5_CODE_DOWN	(0x2951 & RC5_MASK)	/*!< Taste Runter */
#define RC5_CODE_LEFT	(0x2955 & RC5_MASK)	/*!< Taste Links */
#define RC5_CODE_RIGHT	(0x2956 & RC5_MASK)	/*!< Taste Rechts */

#define RC5_CODE_PWR	(0x394C & RC5_MASK)	/*!< Taste An/Aus */

#define RC5_CODE_RED		(0x100B & RC5_MASK)	/*!< Rote Taste */
#define RC5_CODE_GREEN		(0x102E & RC5_MASK)	/*!< Grüne Taste */
#define RC5_CODE_YELLOW	(0x1038 & RC5_MASK)	/*!< Gelbe Taste */
#define RC5_CODE_BLUE		(0x1029 & RC5_MASK)	/*!< Blaue Taste */
#define RC5_CODE_VIEW		(0x000F & RC5_MASK)	/*!< Instant View Taste */

#ifdef JOG_DIAL		
	/* Jogdial geht nur inkl. Adresscode */
	#undef RC5_MASK
	#define RC5_MASK (RC5_COMMAND | RC5_ADDRESS)

	#define RC5_CODE_JOG_MID	(0x3969 & RC5_MASK)	/*!< Taste Jog-Dial Mitte */
	#define RC5_CODE_JOG_L1		(0x3962 & RC5_MASK)	/*!< Taste Jog-Dial Links 1 */
	#define RC5_CODE_JOG_L2		(0x396F & RC5_MASK)	/*!< Taste Jog-Dial Links 2 */
	#define RC5_CODE_JOG_L3		(0x395F & RC5_MASK)	/*!< Taste Jog-Dial Links 3 */
	#define RC5_CODE_JOG_L4		(0x3A6C & RC5_MASK)	/*!< Taste Jog-Dial Links 4 */
	#define RC5_CODE_JOG_L5		(0x3A6B & RC5_MASK)	/*!< Taste Jog-Dial Links 5 */
	#define RC5_CODE_JOG_L6		(0x396C & RC5_MASK)	/*!< Taste Jog-Dial Links 6 */
	#define RC5_CODE_JOG_L7		(0x3A6A & RC5_MASK)	/*!< Taste Jog-Dial Links 7 */
	
	#define RC5_CODE_JOG_R1		(0x3968 & RC5_MASK)	/*!< Taste Jog-Dial Rechts 1 */
	#define RC5_CODE_JOG_R2		(0x3975 & RC5_MASK)	/*!< Taste Jog-Dial Rechts 2 */
	#define RC5_CODE_JOG_R3		(0x396A & RC5_MASK)	/*!< Taste Jog-Dial Rechts 3 */
	#define RC5_CODE_JOG_R4		(0x3A6D & RC5_MASK)	/*!< Taste Jog-Dial Rechts 4 */
	#define RC5_CODE_JOG_R5		(0x3A6E & RC5_MASK)	/*!< Taste Jog-Dial Rechts 5 */
	#define RC5_CODE_JOG_R6		(0x396E & RC5_MASK)	/*!< Taste Jog-Dial Rechts 6 */
	#define RC5_CODE_JOG_R7		(0x3A6F & RC5_MASK)	/*!< Taste Jog-Dial Rechts 7 */
#endif	/* JOG_DIAL */

#endif /* RC5CODE_H_ */
