/*
 * serial_connection_avr.h
 *
 *  Created on: 26.12.2016
 *      Author: ts
 */

#ifndef SERIAL_CONNECTION_AVR_H_
#define SERIAL_CONNECTION_AVR_H_

#include "client_server_base.h"
#include <streambuf>

class SerialConnectionAVR : public tsio::ClientServerBase {
protected:
	static void (*wait_callback)(const void*);

	uint16_t wait_for_data(const uint16_t size, const uint16_t timeout_ms) const noexcept;

public:
	SerialConnectionAVR() noexcept {
		ready = true;
	}

	virtual ~SerialConnectionAVR() = default;

	SerialConnectionAVR(const SerialConnectionAVR&) = delete;
	SerialConnectionAVR& operator=(const SerialConnectionAVR&) = delete;

	static void set_wait_callback(decltype(wait_callback) callback) noexcept {
		wait_callback = callback;
	}

	virtual bool init() override {
		return true;
	}

	virtual std::size_t receive(void* data, const std::size_t size) override;
	virtual std::size_t receive(std::streambuf& buf, const std::size_t size) override;
	virtual std::size_t receive_until(void* data, const char delim, const std::size_t maxsize) override;
	virtual std::size_t receive_until(void* data, const std::string& delim, const std::size_t maxsize) override;
	virtual std::size_t receive_until(std::streambuf& buf, const std::string& delim, const std::size_t maxsize) override;
	virtual std::size_t receive_until(std::streambuf& buf, const char delim, const std::size_t maxsize) override;
	virtual std::size_t receive_async(std::streambuf& buf, std::size_t size, const uint32_t timeout_ms) override;
	virtual std::size_t receive_async(void* data, std::size_t size, const uint32_t timeout_ms) override;
	virtual std::size_t send(const void* data, const std::size_t size) override;
	virtual std::size_t send(std::streambuf& buf, const std::size_t size) override;
};

#endif /* SERIAL_CONNECTION_AVR_H_ */
