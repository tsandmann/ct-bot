/*! @file 	ct-Bot.h
 * @brief 	Demo-Hauptprogramm
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#include "global.h"

/************************************************************
* Module switches, to make code smaller if features are not needed
************************************************************/

#define LED_AVAILABLE		///< LEDs for local control

#define IR_AVAILABLE		///< Infrared Remote Control
#define RC5_AVAILABLE		///< Key-Mapping for IR-RC	

//#define UART_AVAILABLE	///< Serial Communication
//#define COMMAND_AVAILABLE	///< High-Level Communication over Uart, needs UART 

#define DISPLAY_AVAILABLE	///< Display for local control

#define ADC_AVAILABLE		///< A/D-Converter for sensing Power

#define MAUS_AVAILABLE		///< Maus Sensor

#define ENA_AVAILABLE		///< Enable-Leitungen
#define SHIFT_AVAILABLE		///< Shift Register

/************************************************************
* Some Dependencies!!!
************************************************************/

#ifndef DISPLAY_AVAILABLE
	#undef WELCOME_AVAILABLE
#endif

#ifndef IR_AVAILABLE
	#undef RC5_AVAILABLE
#endif

#ifdef PC
	#undef UART_AVAILABLE
	#undef MAUS_AVAILABLE
	#define COMMAND_AVAILABLE
#endif

#ifdef MCU
	#ifndef UART_AVAILABLE
		#undef COMMAND_AVAILABLE
	#endif
#endif


#define F_CPU	16000000L    ///< Crystal frequency in Hz
#define XTAL F_CPU			 ///< Crystal frequency in Hz

