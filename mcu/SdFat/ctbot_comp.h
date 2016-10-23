/*
 * ctbot_comp.h
 *
 *  Created on: 16.10.2016
 *      Author: ts
 */

#ifndef MCU_SDFAT_CTBOT_COMP_H_
#define MCU_SDFAT_CTBOT_COMP_H_

#ifdef MCU
#include "FatLib/FatLibConfig.h"

#if SDFAT_PRINT_SUPPORT
#include "FatLib/FatVolume.h"
#include "Print.h"

class Uart_Print : public Print {
public:
    virtual size_t write(uint8_t data);
    virtual size_t write(const uint8_t* buffer, size_t size);
};

extern Uart_Print Serial;
#endif // SDFAT_PRINT_SUPPORT

#endif // MCU
#endif /* MCU_SDFAT_CTBOT_COMP_H_ */
