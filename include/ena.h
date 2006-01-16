/*! @file 	ena.h 
 * @brief 	Routinen zur Steuerung der Enable-Leitungen
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

#ifndef ENA_H_
#define ENA_H_

#define ENA_ABSTAND		(1<<0)		///< Enable-Leitung Abstandssensoren
#define ENA_RADLED		(1<<1)		///< Enable-Leitung Radencoder
#define ENA_SCHRANKE	(1<<2)		///< Enable-Leitung Fachüberwachung
#define ENA_KANTLED		(1<<3)		///< Enable-Leitung Angrundsensor
#define ENA_KLAPPLED	(1<<4)		///< Enable-Leitung Schieberüberwachung
#define ENA_MAUS		(1<<5)		///< Enable-Leitung Maussensor
#define ENA_ERW1		(1<<6)		///< Enable-Leitung Reserve 1
#define ENA_ERW2		(1<<7)		///< Enable-Leitung Reserve 2
/*!
 * Initialisiert die Enable-Leitungen
 */
void ENA_init();

/*! 
 * Schaltet einzelne Enable-Leitungen an
 * andere werden nicht beeinflusst
 * @param enable Bitmaske der anzuschaltenden LEDs
 */
void ENA_on(char enable);

/*! 
 * Schaltet einzelne Enable-Leitungen aus
 * andere werden nicht beeinflusst
 * @param enable Bitmaske der anzuschaltenden LEDs
 */
void ENA_off(char enable);

/*!
 * Schaltet die Enable-Leitungen
 * @param LED Wert der gezeigt werden soll
 */
void ENA_set(char enable);
#endif
