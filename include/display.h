/*! @file 	display.h 
 * @brief 	Routinen zur Displaysteuerung
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	20.12.05
*/

extern volatile char display_update;	///< Muss das Display aktualisiert werden?

/*! 
 * Init Display
 */
void display_init(void);

/*! 
 * Zeigt einen String an 
 * @return -1 falls string zuende 0 falls Zeile (20 zeichen) zuende
 */
int display_string(char data[20]);

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
