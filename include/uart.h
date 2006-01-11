/*! @file 	uart.c 
 * @brief 	Routinen zur seriellen Kommunikation
 * @author 	Benjamin Benz (bbe@heise.de)
 * @date 	26.12.05
*/

extern char uart_timeout;	///< 0, wenn uart_read/uart_send erfolgreich 1, wenn timeout erreicht

/*!
 * Überträgt ein Zeichen per UART
 * Achtung ist noch blockierend!!!!
 * @param data Das Zeichen
 */
void uart_send(char data); // Achtung ist noch blockierend!!!!

/*! 
 * Prüft, ob daten verfügbar 
 * @return 1, wenn daten verfügbar, sonst 0
 */
char uart_data_available(void);

/*!
 * Initialisiere UART
 */
void uart_init(void);

/*!
 * Liest Zeichen von der UART
 */
char uart_read(char* data, int length);
