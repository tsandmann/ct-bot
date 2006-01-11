/*! @file 	bot-2-sim.h 
 * @brief 	Verbindung c't-Bot zu c't-Sim
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/
#ifndef __bot_2_sim
#define __bot_2_sim

#include "global.h"

/*!
 * Ein wenig Initilisierung kann nicht schaden 
 */
void bot_2_sim_init(void);

/*!
 *  Ask simulator for data 
 */
int bot_2_sim_ask(uint8 command, uint8 subcommand,int16* data_l,int16* data_r);

/*!
 *  Tell simulator data -- dont wait for answer!
 */
void bot_2_sim_tell(uint8 command, uint8 subcommand, int16* data_l,int16* data_r);


/*!
 * Schickt einen Thread in die Warteposition
 * @param timeout_us Wartezeit in µs
 */
void wait_for_time(long timeout_us);
#endif
