/*
 * serial_protocol_clock.h
 *
 *  Created on: 26.12.2016
 *      Author: ts
 */

#ifndef SERIAL_PROTOCOL_CLOCK_H_
#define SERIAL_PROTOCOL_CLOCK_H_

extern "C" {
#include "timer.h"
}

class SerialProtocolClock {
protected:
	static auto get_now() {
		return timer_get_us32() / 1000U;
	}

	static auto get_diff_ms(const uint32_t start, const uint32_t end) {
		return end - start;
	}
};

#endif /* SERIAL_PROTOCOL_CLOCK_H_ */
