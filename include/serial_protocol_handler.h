/*
 * serial_protocol_handler.h
 *
 *  Created on: 26.12.2016
 *      Author: ts
 */

#ifndef SERIAL_PROTOCOL_HANDLER_H_
#define SERIAL_PROTOCOL_HANDLER_H_

#ifdef __cplusplus
namespace ctbot {

extern "C" {
#endif // __cplusplus

void update_sens_cmd(void);

int process_recv_cmd(void);

int serial_protocol_handler(void);

void bot_2_linux_init(void);

void bot_2_linux_listen(void);

void bot_2_linux_inform(void);

void SerialConnection_set_wait_callback(void (*)(const void*));

/**
 * Display Screen fuer Inhalte vom Linux-Board
 */
void linux_display(void);

#ifdef __cplusplus
}
} /* namespace ctbot */
#endif // __cplusplus


#endif /* SERIAL_PROTOCOL_HANDLER_H_ */
