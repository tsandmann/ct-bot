/*! @file 	ct-Bot.h
 * @brief 	Demo-Hauptprogramm
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#include "global.h"

/************************************************************
* Module switches, to make code smaller if features are not needed
************************************************************/
//#define RTC_AVAILABLE		///< Real Time Clock
//#define RTC_HTTPTIME_AVAILABLE	///< RTC fetches Time from HTTP-Server
//#define RTC_FULL_AVAILABLE	///< Full means RTC counts days, month, years
//#define EVENT_AVAILABLE		///< Eventmanagement, needs RTC

#define LED_AVAILABLE		///< LEDs for local control
//#define KEY_AVAILABLE		///< Keys for local control

#define IR_AVAILABLE		///< Infrared Remote Control
#define RC5_AVAILABLE		///< Key-Mapping for IR-RC	

//#define UART_AVAILABLE	///< Serial Communication
//#define COMMAND_AVAILABLE	///< High-Level Communication over Uart, needs UART 

#define DISPLAY_AVAILABLE	///< Display for local control
//#define TICKER_AVAILABLE	///< Just nice Info, needs DISPLAY
//#define WELCOME_AVAILABLE	///< Just nice Info, needs DISPLAY

//#define TOOLS_AVAILABLE		///< String conversiona and beep

#define ADC_AVAILABLE		///< A/D-Converter for sensing Power
//#define FILTER_AVAILABLE	///< Filter for ADC, eliminates 100 Hz noise
//#define PT100_AVAILABLE	///< Temperatur sensors

//#define MAUS_AVAILABLE		///< Maus Sensor

//#define DREH_AVAILABLE	///< Local control by "drehgeber"

#define ENA_AVAILABLE		///< Enable-Leitungen
#define SHIFT_AVAILABLE		///< Shift Register

/************************************************************
* Some Dependencies!!!
************************************************************/

#ifndef DISPLAY_AVAILABLE
	#undef TICKER_AVAILABLE
	#undef WELCOME_AVAILABLE
#endif

#ifndef RTC_AVAILABLE
	#undef EVENT_AVAILABLE
	#undef RTC_HTTPTIME_AVAILABLE
	#undef RTC_FULL_AVAILABLE
#endif

#ifndef IR_AVAILABLE
	#undef RC5_AVAILABLE
#endif

#ifdef PC
	#undef UART_AVAILABLE
#endif

#ifndef UART_AVAILABLE
	#undef COMMAND_AVAILABLE
	#undef RTC_HTTPTIME_AVAILABLE
#endif



#define F_CPU	16000000L    ///< Crystal frequency in Hz
#define XTAL F_CPU			 ///< Crystal frequency in Hz

