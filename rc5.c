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

/*! @file 	rc5.c
 * @brief 	RC5-Fernbedienung
 * Eventuell muessen die Codes an die jeweilige 
 * Fernbedienung angepasst werden
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/
#include "ct-Bot.h"
#include "ir-rc5.h"
#include "motor.h"
#include "bot-logik.h"

#ifdef RC5_AVAILABLE

/*!
 * Fernbedienung mit Jog-Dial-Rad, 
 * Achtung: Die Adress-Bits muessen auf die Fernbedienung angepasst werden!
 */
//#define JOG_DIAL

#define RC5_TOGGLE		0x0800		///< Das RC5-Toggle-Bit
#define RC5_ADDRESS		0x07C0		///< Der Adressbereich
#define RC5_COMMAND		0x103F		///< Der Kommandobereich

#define RC5_MASK (RC5_COMMAND)		///< Welcher Teil des Kommandos wird ausgewertet?


#define RC5_CODE_0	(0x3940 & RC5_MASK)		///< Taste 0
#define RC5_CODE_1	(0x3941 & RC5_MASK)		///< Taste 1
#define RC5_CODE_2	(0x3942 & RC5_MASK)		///< Taste 2
#define RC5_CODE_3	(0x3943 & RC5_MASK)		///< Taste 3
#define RC5_CODE_4	(0x3944 & RC5_MASK)		///< Taste 4
#define RC5_CODE_5	(0x3945 & RC5_MASK)		///< Taste 5
#define RC5_CODE_6	(0x3946 & RC5_MASK)		///< Taste 6
#define RC5_CODE_7	(0x3947 & RC5_MASK)		///< Taste 7
#define RC5_CODE_8	(0x3948 & RC5_MASK)		///< Taste 8
#define RC5_CODE_9	(0x3949 & RC5_MASK)		///< Taste 9

#define RC5_CODE_UP		(0x2950 & RC5_MASK)	///< Taste Hoch
#define RC5_CODE_DOWN	(0x2951 & RC5_MASK)	///< Taste Runter
#define RC5_CODE_LEFT	(0x2955 & RC5_MASK)	///< Taste Links
#define RC5_CODE_RIGHT	(0x2956 & RC5_MASK)	///< Taste Rechts

#define RC5_CODE_PWR	(0x394C & RC5_MASK)	///< Taste An/Aus

#ifdef JOG_DIAL		
	// Jogdial geht nur inkl. Adresscode
	#undef RC5_MASK
	#define RC5_MASK (RC5_COMMAND |RC5_ADDRESS )

	#define RC5_CODE_JOG_MID	(0x3969 & RC5_MASK)	///< Taste Jog-Dial Mitte
	#define RC5_CODE_JOG_L1		(0x3962 & RC5_MASK)	///< Taste Jog-Dial Links 1
	#define RC5_CODE_JOG_L2		(0x396F & RC5_MASK)	///< Taste Jog-Dial Links 2
	#define RC5_CODE_JOG_L3		(0x395F & RC5_MASK)	///< Taste Jog-Dial Links 3
	#define RC5_CODE_JOG_L4		(0x3A6C & RC5_MASK)	///< Taste Jog-Dial Links 4
	#define RC5_CODE_JOG_L5		(0x3A6B & RC5_MASK)	///< Taste Jog-Dial Links 5
	#define RC5_CODE_JOG_L6		(0x396C & RC5_MASK)	///< Taste Jog-Dial Links 6
	#define RC5_CODE_JOG_L7		(0x3A6A & RC5_MASK)	///< Taste Jog-Dial Links 7
	
	#define RC5_CODE_JOG_R1		(0x3968 & RC5_MASK)	///< Taste Jog-Dial Rechts 1
	#define RC5_CODE_JOG_R2		(0x3975 & RC5_MASK)	///< Taste Jog-Dial Rechts 2
	#define RC5_CODE_JOG_R3		(0x396A & RC5_MASK)	///< Taste Jog-Dial Rechts 3
	#define RC5_CODE_JOG_R4		(0x3A6D & RC5_MASK)	///< Taste Jog-Dial Rechts 4
	#define RC5_CODE_JOG_R5		(0x3A6E & RC5_MASK)	///< Taste Jog-Dial Rechts 5
	#define RC5_CODE_JOG_R6		(0x396E & RC5_MASK)	///< Taste Jog-Dial Rechts 6
	#define RC5_CODE_JOG_R7		(0x3A6F & RC5_MASK)	///< Taste Jog-Dial Rechts 7
#endif

uint16 RC5_Code;        ///< Letzter empfangener RC5-Code

/*!
 * Liest ein RC5-Codeword und wertet es aus
 */
void rc5_control(void){
	uint16 rc5=ir_read();
	if (rc5 !=0) {
		RC5_Code= rc5 & RC5_MASK;		// Alle uninteressanten Bits ausblenden
		switch(RC5_Code){
			case RC5_CODE_PWR:	
					// clear target Speed
					target_speed_l=BOT_SPEED_STOP;
					target_speed_r=BOT_SPEED_STOP;
					// Clear goto system
					bot_goto(0,0);
				break;		

			case RC5_CODE_UP:	
					target_speed_l+=10;
					target_speed_r+=10;
				break;
			case RC5_CODE_DOWN:	
					target_speed_l-=10;
					target_speed_r-=10;
				break;
			case RC5_CODE_LEFT:	
					target_speed_l-=10;
				break;
			case RC5_CODE_RIGHT:	
					target_speed_r-=10;
				break;
			
				
			case RC5_CODE_1:	
					target_speed_l=BOT_SPEED_SLOW;
					target_speed_r=BOT_SPEED_SLOW;	
				break;					
			case RC5_CODE_3:	
					target_speed_l=BOT_SPEED_MAX;
					target_speed_r=BOT_SPEED_MAX;	
				break;
			
			case RC5_CODE_5: bot_goto(0,0);	break;
			case RC5_CODE_6: bot_goto(20,-20);	break;
			case RC5_CODE_4: bot_goto(-20,20);	break;
			case RC5_CODE_2: bot_goto(100,100);	break;				
			case RC5_CODE_8: bot_goto(-100,-100);	break;				
			case RC5_CODE_7: bot_goto(-40,40);	break;
			case RC5_CODE_9: bot_goto(40,-40);	break;

			#ifdef JOGDIAL
				case RC5_CODE_JOG_MID:
						target_speed_l=BOT_SPEED_MAX;
						target_speed_r=BOT_SPEED_MAX;	
					break;
				case RC5_CODE_JOG_L1:
						target_speed_l=BOT_SPEED_FAST;
						target_speed_r=BOT_SPEED_MAX;	
				case RC5_CODE_JOG_L2:
						target_speed_l=BOT_SPEED_NORMAL;
						target_speed_r=BOT_SPEED_MAX;	
					break;
				case RC5_CODE_JOG_L3:
						target_speed_l=BOT_SPEED_SLOW;
						target_speed_r=BOT_SPEED_MAX;	
					break;
				case RC5_CODE_JOG_L4:
						target_speed_l=BOT_SPEED_STOP;
						target_speed_r=BOT_SPEED_MAX;	
					break;
					
				case RC5_CODE_JOG_L5:
						target_speed_l=-BOT_SPEED_NORMAL;
						target_speed_r=BOT_SPEED_MAX;	
					break;
				case RC5_CODE_JOG_L6:
						target_speed_l=-BOT_SPEED_FAST;
						target_speed_r=BOT_SPEED_MAX;	
					break;
				case RC5_CODE_JOG_L7:
						target_speed_l=-BOT_SPEED_MAX;
						target_speed_r=BOT_SPEED_MAX;	
					break;
					
				case RC5_CODE_JOG_R1:
						target_speed_r=BOT_SPEED_FAST;
						target_speed_l=BOT_SPEED_MAX;	
				case RC5_CODE_JOG_R2:
						target_speed_r=BOT_SPEED_NORMAL;
						target_speed_l=BOT_SPEED_MAX;	
					break;
				case RC5_CODE_JOG_R3:
						target_speed_r=BOT_SPEED_SLOW;
						target_speed_l=BOT_SPEED_MAX;	
					break;
				case RC5_CODE_JOG_R4:
						target_speed_r=BOT_SPEED_STOP;
						target_speed_l=BOT_SPEED_MAX;	
					break;
					
				case RC5_CODE_JOG_R5:
						target_speed_r=-BOT_SPEED_NORMAL;
						target_speed_l=BOT_SPEED_MAX;	
					break;
				case RC5_CODE_JOG_R6:
						target_speed_r=-BOT_SPEED_FAST;
						target_speed_l=BOT_SPEED_MAX;	
					break;
				case RC5_CODE_JOG_R7:
						target_speed_r=-BOT_SPEED_MAX;
						target_speed_l=BOT_SPEED_MAX;	
					break;			
			#endif
		}
	}
}
#endif
