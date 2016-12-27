/**
 * \file   client_server_base.h
 * \author Timo Sandmann
 * \date   01.11.2014
 * \brief  Client / server interface
 */

#ifndef CLIENT_SERVER_BASE_H_
#define CLIENT_SERVER_BASE_H_

#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <streambuf>

namespace tsio {

/**
 * Interface class for clients and servers, provides send and receive operations
 */
class ClientServerBase {
protected:
	bool ready;

public:
	ClientServerBase() : ready(false) {}
	virtual ~ClientServerBase() = default;

	auto get_ready() const noexcept {
		return ready;
	}

	virtual bool init() = 0;
	virtual std::size_t receive(void* data, const std::size_t size) = 0;
	virtual std::size_t receive(std::streambuf& buf, const std::size_t size) = 0;
	virtual std::size_t receive_until(void* data, const char delim, const std::size_t maxsize) = 0;
	virtual std::size_t receive_until(void* data, const std::string& delim, const std::size_t maxsize) = 0;
	virtual std::size_t receive_until(std::streambuf& buf, const std::string& delim, const std::size_t maxsize) = 0;
	virtual std::size_t receive_until(std::streambuf& buf, const char delim, const std::size_t maxsize) = 0;
	virtual std::size_t receive_async(void* data, std::size_t size, const uint32_t timeout_ms) = 0;
	virtual std::size_t receive_async(std::streambuf& buf, std::size_t size, const uint32_t timeout_ms) = 0;
	virtual std::size_t send(const void* data, const std::size_t size) = 0;
	virtual std::size_t send(std::streambuf& buf, const std::size_t size) = 0;
};

/**
 * Exception type for closed connection
 */
class ClientServerEOF : public std::runtime_error {
public:
	ClientServerEOF(const std::string& what) : std::runtime_error(what) {}
};

} /* namespace tsio */

#endif /* CLIENT_SERVER_BASE_H_ */
