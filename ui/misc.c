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
 * @file 	misc.c
 * @brief 	Sonstige Display-Anzeigefunktionen, die in keine andere Datei so richtig passen
 * @author 	Timo Sandmann (mail@timosandmann.de)
 * @date 	12.02.2007
 */
 
#include "ct-Bot.h"
#include "global.h"
#include "ui/available_screens.h"
#include "display.h"
#include "bot-logic/bot-logik.h"
#include "timer.h"
#ifdef MCU
	#include <avr/eeprom.h>
#endif

#ifdef MCU
	uint8 __attribute__ ((section (".eeprom"))) resetsEEPROM = 0;	/*!< Reset-Counter-Wert im EEPROM */
#endif

#ifdef DISPLAY_AVAILABLE
	#ifdef MISC_DISPLAY_AVAILABLE
		/*! 
		 * @brief	Zeigt ein paar Infos an, die man nicht naeher zuordnen kann
		 */
		void misc_display(void){
			#ifdef TIME_AVAILABLE
				display_cursor(1,1);
				display_printf("Zeit: %04d:%03d", timer_get_s(), timer_get_ms());
			#endif
			
			#ifdef BEHAVIOUR_AVAILABLE
				display_cursor(2,1);
				display_printf("TS=%+4d %+4d",target_speed_l,target_speed_r);
			#endif
			
			#ifdef SRF10_AVAILABLE		
				display_cursor(2,15);
				display_printf("US%+4d",sensSRF10);
			#endif
			
			display_cursor(3,1);
			display_printf("RC=%+4d %+4d",sensEncL,sensEncR);
	
			display_cursor(4,1);
			display_printf("Speed= %04d",(int16)v_center);		
		}
	#endif	// MISC_DISPLAY_AVAILABLE
	
	#ifdef RESET_INFO_DISPLAY_AVAILABLE
		uint8 reset_flag;
		/*!
		 * @brief Zeigt Informationen ueber Resets an
		 */
		void reset_info_display(void){
			display_cursor(1,1);
			display_printf("#Resets:%3u | (C)SR:", (uint8)eeprom_read_byte(&resetsEEPROM));
									
			display_cursor(2,1);
			display_printf("PORF :%d  WDRF :%d",binary(reset_flag,0),binary(reset_flag,3)); 

			display_cursor(3,1);
			display_printf("EXTRF:%d  JTRF :%d",binary(reset_flag,1),binary(reset_flag,4)); 
						
			display_cursor(4,1);
			display_printf("BORF :%d",binary(reset_flag,2));
		}	
	#endif	// RESET_INFO_DISPLAY_AVAILABLE
	
	#ifdef RAM_DISPLAY_AVAILABLE
		void ram_display(void){
			extern unsigned char __heap_start;
			uint16 frei = SP - (uint16) &__heap_start;
			display_cursor(1,1);
			display_printf("free RAM: %4d ",frei);			
		}
	#endif	// RAM_DISPLAY_AVAILABLE
#endif	// DISPLAY_AVAILABLE
