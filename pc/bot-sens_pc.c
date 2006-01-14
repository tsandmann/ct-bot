/*! @file 	bot-sens_pc.c  
 * @brief 	Low-Level Routinen für die Sensor Steuerung des c't-Bots
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	01.12.05
*/

#include "ct-Bot.h"

#ifdef PC

#include "adc.h" 
#include "global.h"
#include "bot-mot.h"
#include "bot-2-sim.h"
#include "command.h"
#include "ir.h"

volatile int16 sensDistL=0;		///< Distanz linker IR-Sensor
volatile int16 sensDistR=0;		///< Distanz rechter IR-Sensor

volatile int16 sensEncL=0;		///< aufbereiteter Encoder linker Motor
volatile int16 sensEncR=0;		///< Encoder rechter Motor

volatile int16 sensBorderL=0;	///< Abgrundsensor links
volatile int16 sensBorderR=0;	///< Abgrundsensor rechts

volatile int16 sensLineL=0;		///< Lininensensor links
volatile int16 sensLineR=0;	///< Lininensensor rechts

volatile int16 sensLdrL=0;		///< Helligkeitssensor links
volatile int16 sensLdrR=0;		///< Helligkeitssensor links

volatile char sensTrans=0;		///< Sensor Überwachung Transportfach

volatile char sensDoor=0;		///< Sensor Überwachung Klappe

volatile char sensError=0;		///< Überwachung Motor oder Batteriefehler

/*!
 * Initialisiere alle Sensoren
 */
void bot_sens_init(void){
}

/*!
 * Alle Sensoren aktualisieren
 */
 /*
void bot_sens_isr(void){
	int16 tmp_l=0;
	int16 tmp_r=0;
	
	//Aktualisiere Distanz-Sensoren
	bot_2_sim_ask(CMD_SENS_IR, SUB_CMD_NORM,(int16*)&sensDistL,(int16*)&sensDistR);
	
	// Radencoder
	bot_2_sim_ask(CMD_SENS_ENC, SUB_CMD_NORM,&tmp_l,&tmp_r);
	// --------------------- links ----------------------------	
	if (mot_l_dir==1)	// Drehrichtung beachten
		encoderL+=tmp_l;	//vorw�rts
	else 
		encoderL-=tmp_l;	//r�ckw�rts
	
	// --------------------- rechts ----------------------------
	if (mot_r_dir==1)	// Drehrichtung beachten
		encoderR+=tmp_r;	//vorw�rts
	else 
		encoderR-=tmp_r;	//r�ckw�rts
		
	//Aktualisiere Abgrund-Sensoren
	bot_2_sim_ask(CMD_SENS_BORDER, SUB_CMD_NORM,(int16*)&sensBorderL,(int16*)&sensBorderR);
	
	//Aktualisiere Linien-Sensoren
	bot_2_sim_ask(CMD_SENS_LINE, SUB_CMD_NORM,(int16*)&sensLineL,(int16*)&sensLlineR);
		
	//Aktualisiere Helligkeits-Sensoren
	bot_2_sim_ask(CMD_SENS_LDR, SUB_CMD_NORM,(int16*)&sensLdrL,(int16*)&sensLdrR);

	// Überwachung Transportfach
	bot_2_sim_ask(CMD_SENS_TRANS, SUB_CMD_NORM,&tmp_l,&tmp_r);
	sensTrans=(char)tmp_l;


	// Überwachung Klappe
	#ifdef DOOR_AVAILABLE
//		bot_2_sim_ask(CMD_SENS_DOOR, SUB_CMD_NORM,&tmp_l,&tmp_r);
		sensDoor=(char)tmp_l;
	#endif

	// Überwachung Motor- oder Batteriefehler
	bot_2_sim_ask(CMD_SENS_ERROR, SUB_CMD_NORM,&tmp_l,&tmp_r);
	sensError=(char)tmp_l;
	
	ir_isr();	
}
*/
#endif
