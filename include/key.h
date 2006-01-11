/*! @file 	key.h 
 * @brief 	Routinen zum Einlesen der Taster
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

/*!
 * Lies alle Tasten aus. 
 * @return Liefert die Hex Werte der Keys zurück, wenn diese
 * gedrückt sind (d.h. nichts negiert, obwohl Taster als Pull-Downs)
 */
char key_read(void);

/*!
 * Kümert sich regelmässig um das auslesden der Tasten
 */
void key_isr(void);
