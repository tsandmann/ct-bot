/*
 * serial_connection_avr.cpp
 *
 *  Created on: 26.12.2016
 *      Author: ts
 */

#include "ct-Bot.h"

#ifdef BOT_2_RPI_AVAILABLE
#include "serial_connection_avr.h"
#include "crc_engine.h"
#include <cstring>

extern "C" {
#include "uart.h"
#include "os_thread.h"
}

decltype(SerialConnectionAVR::wait_callback) SerialConnectionAVR::wait_callback(nullptr);

uint16_t SerialConnectionAVR::wait_for_data(const uint16_t size, const uint16_t timeout_ms) const noexcept {
	auto available(uart_data_available());
	if (available >= size) {
		return size;
	}

	const auto ticks(static_cast<uint16_t>(MS_TO_TICKS(timeout_ms)));
	const auto start(TIMER_GET_TICKCOUNT_16);
	auto now(start);
	const auto sleep_time(static_cast<uint16_t>((size - available) / 50));
	if (size > available && sleep_time && (timeout_ms >= sleep_time || (! timeout_ms))) {
		if (wait_callback) {
			wait_callback(this);
		}
		os_thread_sleep(sleep_time);
	}
	available = uart_data_available();
	while (available < size && ((now - start) < ticks || (! timeout_ms))) {
		if (wait_callback) {
			wait_callback(this);
		}
		os_thread_sleep_ticks(1);
		available = uart_data_available();
		now = TIMER_GET_TICKCOUNT_16;
	}

	return std::min<uint16_t>(available, size);
}

std::size_t SerialConnectionAVR::receive(void* data, const std::size_t size) {
	if (! size) {
		return 0;
	}

	const auto size16(static_cast<uint16_t>(size));
	wait_for_data(size16, 0);

	return uart_read(data, size16);
}

std::size_t SerialConnectionAVR::receive(std::streambuf& buf, const std::size_t size) {
	if (! size) {
		return 0;
	}

	const auto size16(static_cast<int16_t>(size));
	wait_for_data(size16, 0);

	for (int16_t i(0); i < size16; ++i) {
		buf.sputc(_inline_fifo_get(&uart_infifo, 0));
	}

	return size16;
}

std::size_t SerialConnectionAVR::receive_until(void* data, const char delim, const std::size_t maxsize) {
	const auto size16(static_cast<uint16_t>(maxsize));
	uint16_t n(0);
	char* ptr(reinterpret_cast<char*>(data));
	do {
		*ptr = _inline_fifo_get(&uart_infifo, 0);
		++n;
	} while (*ptr++ != delim && n < size16);

	return n;
}

std::size_t SerialConnectionAVR::receive_until(void* data, const std::string& delim, const std::size_t maxsize) {
	const auto size16(static_cast<uint16_t>(maxsize));
	uint16_t n(0);
	char* ptr(reinterpret_cast<char*>(data));
	do {
		*ptr = _inline_fifo_get(&uart_infifo, 0);
		++ptr;
		++n;
	} while (std::strncmp(reinterpret_cast<const char*>(data), delim.c_str(), n) && n < size16);

	return n;
}

std::size_t SerialConnectionAVR::receive_until(std::streambuf& buf, const char delim, const std::size_t maxsize) {
	const auto size16(static_cast<uint16_t>(maxsize));
	uint16_t n(0);
	char tmp;
	do {
		tmp = _inline_fifo_get(&uart_infifo, 0);
		buf.sputc(tmp);
		++n;
	} while (tmp != delim && n < size16);

	return n;
}

std::size_t SerialConnectionAVR::receive_until(std::streambuf& buf, const std::string& delim, const std::size_t maxsize) {
	auto& buffer(reinterpret_cast<std::stringbuf&>(buf));
	const auto size16(static_cast<uint16_t>(maxsize));

	uint16_t n(0);
	do {
		buf.sputc(_inline_fifo_get(&uart_infifo, 0));
		++n;
	} while (std::strncmp(ctbot::streambuf_helper::get_data(buffer), delim.c_str(), n) && n < size16);

	return n;

	return 0;
}

std::size_t SerialConnectionAVR::receive_async(void* data, const std::size_t size, const uint32_t timeout_ms) {
	const auto timeout(static_cast<uint16_t>(timeout_ms));

	const auto n(wait_for_data(size, timeout));
	return receive(data, n);
}

std::size_t SerialConnectionAVR::receive_async(std::streambuf& buf, std::size_t size, const uint32_t timeout_ms) {
	const auto timeout(static_cast<uint16_t>(timeout_ms));

	const auto n(wait_for_data(size, timeout));
	return receive(buf, n);
}

std::size_t SerialConnectionAVR::send(const void* data, const std::size_t size) {
	return uart_write(data, size);
}

std::size_t SerialConnectionAVR::send(std::streambuf& buf, const std::size_t size) {
	const auto size16(static_cast<uint16_t>(size));
	for (auto i(0U); i < size16; ++i) {
		uart_put(static_cast<uint8_t>(buf.sbumpc()));
	}
	return size;
}
#endif // BOT_2_RPI_AVAILABLE
