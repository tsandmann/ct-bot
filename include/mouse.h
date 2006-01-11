/*! @file 	mouse.h 
 * @brief 	Routinen f�r die Ansteuerung eines opt. Maussensors
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

extern int maus_x;	///< X-Koordinate
extern int maus_y;	///< Y-Koordinate

/*! 
 * Initialisiere Maussensor
 */ 
void maus_sens_init(void);

/*! 
 * Aktualisiere die Position des Maussensors
 */
void maus_sens_pos(void);
