/*! @file 	global.h
 * @brief 	Allgemeine Definitionen
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/

#ifndef global_H
	#define global_H		///< Bereits definiert
	
	#ifndef MCU
		#ifndef PC
			#define PC		///< Zielplattform PC
		#endif
	#endif
	
	#ifndef WIN32
		typedef unsigned char byte;                       ///< vorzeichenlose 8-Bit-Zahl
		typedef byte bool;                                ///< True/False Aussage
	#endif

	//#define DOXYGEN		///< Nur zum generieren von Code!!!!
	#ifdef DOXYGEN		///< Nur zum generieren von Code!!!!
		#define PC		///< Zielplattform PC
		#define MCU		///< Zielplattform MCU
		#define WIN32	///< System Windows
		#define __linux__	///< System Linux
	#endif
	
	typedef unsigned char uint8;                       ///< vorzeichenlose 8-Bit-Zahl 
	typedef unsigned int word;                         ///< vorzeichenlose 16-Bit-Zahl 
	typedef signed char int8;                          ///< vorzeichenbehaftete 8-Bit-Zahl 
	typedef short int int16;                           ///< vorzeichenbehaftete 16-Bit-Zahl 

	#define uint16                  word				///< Int mit 16 Bit
	
	#define True                  1						///< Wahr
	#define False                 0						///< Falsch
	
	#define On                    1						///< An
	#define Off                   0						///< Aus
	
	//#define NULL 0
#endif
