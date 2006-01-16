/*! @file 	display.h 
 * @brief 	Routinen zur Displaysteuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/

#ifndef display_H_
#define display_H_

#define DISPLAY_BUFFER	30			///< Grösse des Display Strings

extern volatile char display_update;	///< Muss das Display aktualisiert werden?
extern char display_buf[DISPLAY_BUFFER];		///< Pufferstring für Displayausgaben
/*! 
 * Init Display
 */
void display_init(void);

/*! 
 * Zeigt einen String an 
 * @return -1 falls string zuende 0 falls Zeile (20 zeichen) zuende
 */
//int display_string(char data[20]);

/*!
 * Löscht das ganze Display
 */
void display_clear(void);

/*!
 * Positioniert den Cursor
 * @param row Zeile
 * @param column Spalte
 */
void display_cursor (int row, int column) ;

/*! 
 * Zeigt den String an, der in display_buffer steht. 
 * @return 0 falls 0x00-Zeichen erreicht; -1, falls DISPLAY_LENGTH oder DISPLAY_BUFFER Zeichen ausgegeben wurden
 */
int display_buffer();
//void display_test();
#endif
