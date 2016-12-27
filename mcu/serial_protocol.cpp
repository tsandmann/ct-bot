/**
 * \file   serial_protocol.cpp
 * \author Timo Sandmann
 * \date   26.12.2016
 * \brief  Data transfer protocol for serial connections like UART, SPI, etc.
 */

#include "client_server_base.h"
#include "serial_protocol.h"
#include "crc_engine.h"
#include <cstring>
#include <algorithm>

namespace ctbot {

SerialProtocol::SerialProtocol(tsio::ClientServerBase& connection, uint32_t max_retries, const uint32_t recv_timeout_ms) noexcept : con(connection),
		receive_timeout_ms(recv_timeout_ms), retries(max_retries), sequence_number(0), crc_errors(0), resends(0) {}

bool SerialProtocol::receive_header(header& head, const bool timeout) {
	logger.debug << tslog::lock << "SerialProtocol::receive_header(" << timeout << ")" << tslog::endlF;

	const auto start(get_now());
	const auto n(con.receive_async(&head, sizeof(head), timeout ? receive_timeout_ms : 0));
	if (n != sizeof(head)) {
		logger.error << tslog::lock << "SerialProtocol::receive_header(): receive for header on connection failed, n=" << n << ", timeout=" << timeout << tslog::endl;
		return false;
	}

	while (head.start != STARTBYTE) {
		logger.debug << tslog::lock << "SerialProtocol::receive_header(): receive_timeout_ms=" << receive_timeout_ms << " start=" << static_cast<uint16_t>(head.start)
			<< ", searching for start..." << tslog::endlF;

		std::memmove(&head, reinterpret_cast<char*>(&head) + 1, sizeof(head) - 1);
		const auto now(get_now());
		const auto diff(get_diff_ms(start, now));
		if (diff > receive_timeout_ms) {
			logger.error << tslog::lock << "SerialProtocol::receive_header(): receive for header on connection failed (timeout), abort. timeout=" << timeout << tslog::endl;
			return false;
		}
		const auto dt(timeout ? receive_timeout_ms - diff : 0);
		if (con.receive_async(reinterpret_cast<char*>(&head) + sizeof(head) - 1, 1, dt) != 1) {
			logger.error << tslog::lock << "SerialProtocol::receive_header(): receive2 for header on connection failed, abort. diff=" << diff << ", timeout=" << timeout
				<< tslog::endl;
			return false;
		}
	}

	crc_header crc8;
	crc8.process_bytes(&head, sizeof(head));
	if (crc8.checksum()) {
		logger.debug << tslog::lock<< "SerialProtocol::receive_header(): wrong crc of header packet, CRC=" << static_cast<uint16_t>(crc8.checksum()) << tslog::endlF;

		++crc_errors;
		return false;
	}

	logger.debug << tslog::lock<< "SerialProtocol::receive_header(): header with seq=" << static_cast<uint16_t>(head.seq) << ", type=" << static_cast<uint16_t>(head.type)
		<< ", length=" << head.length << " received." << tslog::endlF;

	return true;
}

bool SerialProtocol::send_header(header& head) const {
	crc_header crc8;
	crc8.process_bytes(&head, sizeof(head) - sizeof(head.crc));
	head.crc = crc8.checksum();

	if (con.send(&head, sizeof(head)) != sizeof(head)) {
		logger.error << tslog::lock << "SerialProtocol::send_header(): send for header on connection failed" << tslog::endl;
		return false;
	}

	logger.debug << tslog::lock << "SerialProtocol::send_header(): header with seq=" << static_cast<uint16_t>(head.seq) << ", type=" << static_cast<uint16_t>(head.type)
		<< ", length=" << head.length << " sent." << tslog::endl;

	return true;
}

bool SerialProtocol::send_data(const void* data, const uint16_t size) const {
	const auto n(con.send(data, size));
	if (n != size) {
		logger.error << tslog::lock << "SerialProtocol::send_data(): send for data on connection failed, abort. size=" << n << tslog::endl;
		return false;
	}

	crc_data crc16;
	crc16.process_bytes(data, size);
	const uint16_t crc_data(crc16.checksum());

	if (con.send(&crc_data, sizeof(crc_data)) != sizeof(crc_data)) {
		logger.error << tslog::lock << "SerialProtocol::send_data(): send for crc of data on connection failed, abort." << tslog::endl;
		return false;
	}

	logger.debug << tslog::lock << "SerialProtocol::send_data(): " << size << " byte of data sent." << tslog::endl;

	return true;
}

bool SerialProtocol::send_data(std::streambuf& buf, const uint16_t size) const {
	crc_data crc16;
	crc16.process_stream(buf, size);

	const auto n(con.send(buf, size));
	if (n != size) {
		logger.error << tslog::lock << "SerialProtocol::send_data(): send for data on connection failed, abort. size=" << n << tslog::endl;
		return false;
	}

	const uint16_t crc_data(crc16.checksum());
	if (con.send(&crc_data, sizeof(crc_data)) != sizeof(crc_data)) {
		logger.error << tslog::lock << "SerialProtocol::send_data(): send for crc of data on connection failed, abort." << tslog::endl;
		return false;
	}

	logger.debug << tslog::lock << "SerialProtocol::send_data(): " << size << " byte of data sent." << tslog::endl;

	return true;
}

bool SerialProtocol::receive_data(char* data, const uint16_t size, const std::size_t max_size) {
	const std::size_t to_recv(std::min<std::size_t>(size, max_size));
	const auto start(get_now());
	if (con.receive_async(data, to_recv, receive_timeout_ms) != to_recv) {
		logger.error << tslog::lock << "SerialProtocol::receive_data(): receive for data on connection failed, abort." << tslog::endl;
		return false;
	}

	/* check CRC16 of received data */
	crc_data crc16;
	crc16.process_bytes(data, to_recv);

	/* discard additional bytes which don't fit in buffer */
	crc_data crc16_additional(crc16.checksum());
	unsigned char tmp;
	for (std::size_t i(0); i < size - to_recv; ++i) {
		logger.debug << tslog::lock << "SerialProtocol::receive_data(): i=" << i << " , size=" << size << ", to_recv=" << to_recv << tslog::endl;

		const auto dt(receive_timeout_ms - get_diff_ms(start, get_now()));
		if (con.receive_async(&tmp, 1, dt) != 1) {
			logger.error << tslog::lock << "SerialProtocol::receive_data(): receive for data on connection failed, abort." << tslog::endl;;
			return false;
		}
		crc16_additional.process_byte(tmp);
	}

	/* receive and check CRC16 */
	uint16_t crc16_from_con;
	const auto dt(receive_timeout_ms - get_diff_ms(start, get_now()));
	if (con.receive_async(&crc16_from_con, sizeof(crc16_from_con), dt) != sizeof(crc16_from_con)) {
		logger.error << tslog::lock << "SerialProtocol::receive_data(): receive for crc16 on connection failed, abort." << tslog::endl;
		return false;
	}

	logger.debug << tslog::lock << "SerialProtocol::receive_data(): " << to_recv << " byte of data received." << tslog::endl;

	if (crc16_additional.checksum() != crc16_from_con) {
		logger.error << tslog::lock << "SerialProtocol::receive_data(): wrong crc of data packet." << tslog::endl;
		logger.debug << tslog::lock << "SerialProtocol::receive_data(): crc16.checksum()=" << crc16.checksum() << ", crc16_from_con=" << crc16_from_con << tslog::endl;

		++crc_errors;
		return false;
	}

	return true;
}

bool SerialProtocol::receive_data(std::streambuf& buf, const uint16_t size, const std::size_t max_size) {
	const std::size_t to_recv(std::min<std::size_t>(size, max_size));
	const auto buf_offset(buf.in_avail());

	logger.debug << tslog::lock << "SerialProtocol::receive_data(): to_recv=" << to_recv << ", buf_offset=" << buf_offset << tslog::endl;

	const auto start(get_now());
	const auto n(con.receive_async(buf, to_recv, receive_timeout_ms));
	if (n != to_recv) {
		logger.error << tslog::lock << "SerialProtocol::receive_data(): receive for data on connection failed, abort. n=" << n << tslog::endl;
		return false;
	}
	logger.debug << tslog::lock << "SerialProtocol::receive_data(): buf.in_avail()=" << buf.in_avail() << tslog::endl;

	/* check CRC16 of received data */
	crc_data crc16;
	crc16.process_stream(buf, to_recv, static_cast<std::ptrdiff_t>(buf_offset));

	/* discard additional bytes which don't fit in buffer */
	crc_data crc16_additional(crc16.checksum());
	unsigned char tmp;
	for (std::size_t i(0); i < size - to_recv; ++i) {
		logger.debug << tslog::lock << "SerialProtocol::receive_data(): i=" << i << " , size=" << size << ", to_recv=" << to_recv << tslog::endl;

		const auto dt(receive_timeout_ms - get_diff_ms(start, get_now()));
		if (con.receive_async(&tmp, 1, dt) != 1) {
			logger.error << tslog::lock << "SerialProtocol::receive_data(): receive for data on connection failed, abort." << tslog::endl;
			return false;
		}
		crc16_additional.process_byte(tmp);
	}

	/* receive and check CRC16 */
	uint16_t crc16_from_con;
	const auto dt(receive_timeout_ms - get_diff_ms(start, get_now()));
	if (con.receive_async(&crc16_from_con, sizeof(crc16_from_con), dt) != sizeof(crc16_from_con)) {
		logger.error << tslog::lock << "SerialProtocol::receive_data(): receive for crc16 on connection failed, abort." << tslog::endl;
		return false;
	}

	logger.debug << tslog::lock << "SerialProtocol::receive_data(): " << to_recv << " byte of data received." << tslog::endl;

	if (crc16_additional.checksum() != crc16_from_con) {
		logger.error << tslog::lock << "SerialProtocol::receive_data(): wrong crc of data packet." << tslog::endl;
		logger.debug << tslog::lock << "SerialProtocol::receive_data(): crc16.checksum()=" << crc16.checksum() << ", crc16_from_con=" << crc16_from_con << tslog::endl;

		++crc_errors;
		return false;
	}

	return true;
}

template <typename T>
std::size_t SerialProtocol::master_receive(T& data, const std::size_t size, const uint8_t tag) {
	const uint16_t to_recv(static_cast<uint16_t>(std::min(size, +max_size)));

	auto i(0U);
	for (; i < retries; ++i) {
		{ /* send request with length of max. receive and CRC8 */
			logger.debug << tslog::lock << "SerialProtocol::master_receive(): sending request for " << to_recv << " byte of data to slave..." << tslog::endl;

			header head {STARTBYTE, static_cast<uint8_t>(sequence_number), static_cast<uint8_t>(header_types::REQUEST), static_cast<uint8_t>(tag & ((1 << TAG_BITS) - 1)),
				static_cast<uint16_t>(to_recv & max_size), 0};
			send_header(head);

			++sequence_number;
			sequence_number %= max_seq_no + 1;
		}

		{ /* receive header with length and CRC8 */
			logger.debug << tslog::lock << "SerialProtocol::master_receive(): waiting for header from slave..." << tslog::endl;

			header head;
			if (! receive_header(head, true)) {
				/* send NACK back */
				logger.debug << tslog::lock << "SerialProtocol::master_receive(): bad header, sending NACK back..." << tslog::endl;

				header head_nack {STARTBYTE, static_cast<uint8_t>(sequence_number), static_cast<uint8_t>(header_types::NACK), 0, 0, 0};
				send_header(head_nack);
				continue;
			}

			if (static_cast<header_types>(head.type) == header_types::ACK && head.seq == sequence_number) {
				/* ACK received, receive data and CRC16 */
				const uint16_t length(head.length);
				if (! length) {
					logger.debug << tslog::lock << "SerialProtocol::master_receive(): slave has no data available, abort." << tslog::endl;

					return 0;
				}
				++sequence_number;
				sequence_number %= max_seq_no + 1;

				logger.debug << tslog::lock << "SerialProtocol::master_receive(): waiting for " << length << " byte of data from slave..." << tslog::endl;

				if (! receive_data(data, length, length)) {
					/* send NACK back */
					logger.debug << tslog::lock << "SerialProtocol::master_receive(): bad data, sending NACK back..." << tslog::endl;

					header head_nack {STARTBYTE, static_cast<uint8_t>(sequence_number), static_cast<uint8_t>(header_types::NACK), 0, 0, 0};
					send_header(head_nack);
					continue;
				}

				/* send ACK back */
				logger.debug << tslog::lock << "SerialProtocol::master_receive(): received data, sending ACK back..." << tslog::endl;

				header head_ack {STARTBYTE, static_cast<uint8_t>(sequence_number), static_cast<uint8_t>(header_types::ACK), 0, 0, 0};
				send_header(head_ack);

				++sequence_number;
				sequence_number %= max_seq_no + 1;

				/* ACK received, all done */
				logger.debug << tslog::lock << "SerialProtocol::master_receive(): got " << to_recv << " byte of data from slave, done. retries needed: " << i << tslog::endl;

				break;
			} else if (static_cast<header_types>(head.type) != header_types::ACK) {
				/* NACK from sender, rerequest */
				logger.debug << tslog::lock << "SerialProtocol::master_receive(): invalid type of received packet (==" << static_cast<uint16_t>(head.type) << ")." << tslog::endl;
			} else if (head.seq != sequence_number) {
				/* wrong seq. no. of received ACK, rerequest */
				logger.debug << tslog::lock << "SerialProtocol::master_receive(): invalid sequence no. of received packet: " << static_cast<uint16_t>(head.seq) << " should be "
					<< sequence_number << tslog::endl;
			}
		}
	}

	if (i == retries) {
		logger.error << tslog::lock << "SerialProtocol::master_receive(): max no. of retries (" << retries << ") reached, abort." << tslog::endl;
	}

	resends += i;

	return i == retries ? 0 : to_recv;
}

template <typename T>
std::size_t SerialProtocol::master_send(T& data, const std::size_t size, const uint8_t tag) {
	const uint16_t to_send(static_cast<uint16_t>(std::min(size, +max_size)));
	uint16_t sent(0);

	auto i(0U);
	for (; i < retries; ++i) {
		{ /* send header with length, sequence no. and CRC8 */
			logger.debug << tslog::lock << "SerialProtocol::master_send(): sending header for " << to_send << " byte of data to slave..." << tslog::endlF;
			header head {STARTBYTE, static_cast<uint8_t>(sequence_number), static_cast<uint8_t>(header_types::SEND), static_cast<uint8_t>(tag & ((1 << TAG_BITS) - 1)),
				static_cast<uint16_t>(to_send & max_size), 0};
			send_header(head);

			/* send data and CRC16 */
			logger.debug << tslog::lock << "SerialProtocol::master_send(): sending " << to_send << " byte of data to slave..." << tslog::endlF;

			send_data(data, to_send);

			++sequence_number;
			sequence_number %= max_seq_no + 1;
		}

		{ /* receive header, check CRC8 and ACK */
			logger.debug << tslog::lock << "SerialProtocol::master_send(): waiting for ACK from slave..." << tslog::endlF;

			header head;
			if (! receive_header(head, true)) {
				logger.debug << tslog::lock << "SerialProtocol::master_send(): no ACK from slave, retry. i=" << i << tslog::endlF;

				continue;
			}

			if (static_cast<header_types>(head.type) == header_types::ACK && head.seq == sequence_number) {
				/* ACK received, all done */
				sent = head.length;
				logger.debug << tslog::lock << "SerialProtocol::master_send(): got ACK from slave for " << sent << " byte of data, done. retries needed: " << i << tslog::endl;

				break;
			} else if (static_cast<header_types>(head.type) != header_types::ACK) {
				/* NACK from receiver, resend */
				if (static_cast<header_types>(head.type) == header_types::NACK) {
					logger.debug << tslog::lock << "SerialProtocol::master_send(): NACK received." << tslog::endl;
				} else {
					logger.debug << tslog::lock << "SerialProtocol::master_send(): invalid type of ack packet (==" << static_cast<uint16_t>(head.type) << ")." << tslog::endl;
				}
			} else if (head.seq != sequence_number) {
				/* wrong seq. no. of received ACK, resend */
				logger.debug << tslog::lock << "SerialProtocol::master_send(): invalid sequence no. of ack packet: " << static_cast<uint16_t>(head.seq) << " should be "
					<< sequence_number << tslog::endl;
			}
		}
	}

	if (i == retries) {
		logger.error << tslog::lock << "SerialProtocol::master_send(): max no. of retries (" << retries << ") reached, abort." << tslog::endl;
	}

	resends += i;

	return i == retries ? 0 : sent;
}

bool SerialProtocol::slave_listen(header& head) {
	/* receive header and check CRC8 */
	logger.debug << tslog::lock << "SerialProtocol::slave_listen(): waiting for header from master..." << tslog::endl;

	if (! receive_header(head)) { // blocking, no timeout
		/* bad CRC8, send NACK back */
		header head_nack {STARTBYTE, static_cast<uint8_t>(head.seq + 1), static_cast<uint8_t>(header_types::NACK), 0, 0, 0};
		logger.debug << tslog::lock << "SerialProtocol::slave_listen(): sending NACK to master..." << tslog::endl;

		send_header(head_nack);

		logger.debug << tslog::lock << "SerialProtocol::slave_listen(): processing failed (bad header)." << tslog::endl;

		return false;
	}

	return true;
}

template <typename T>
int SerialProtocol::slave_process_request(header& head, T& buf, const std::size_t size) {
	const uint8_t seq(static_cast<uint8_t>(head.seq + 1));

	/* check header type */
	switch (static_cast<header_types>(head.type)) {
	case header_types::SEND:
	{
		/* receive data and CRC16 */
		const uint16_t length(std::min<uint16_t>(head.length, static_cast<uint16_t>(size)));
		logger.debug << tslog::lock << "SerialProtocol::slave_process_request(): got SEND request, waiting for " << length << " byte of data from master..." << tslog::endl;

		if (! receive_data(buf, head.length, length)) {
			/* bad CRC16 or timeout, send NACK back */
			logger.debug << tslog::lock << "SerialProtocol::slave_process_request(): sending NACK to master..." << tslog::endl;

			header head_nack {STARTBYTE, static_cast<uint8_t>(seq), static_cast<uint8_t>(header_types::NACK), 0, 0, 0};
			send_header(head_nack);

			logger.debug << tslog::lock << "SerialProtocol::slave_process_request(): processing failed (bad CRC of data or timeout)." << tslog::endl;

			return -1;
		}

		/* send ACK back */
		logger.debug << tslog::lock << "SerialProtocol::slave_process_request(): sending ACK to master..." << tslog::endl;

		header head_ack {STARTBYTE, static_cast<uint8_t>(seq), static_cast<uint8_t>(header_types::ACK), 0, static_cast<uint16_t>(length & max_size), 0};
		send_header(head_ack);

		logger.debug << tslog::lock << "SerialProtocol::slave_process_request(): receiving from master done." << tslog::endl;

		return 1; // data from master received
	}

	case header_types::REQUEST:
	{
		/* send header with length, sequence no. and CRC8 */
		const uint16_t length(std::min<uint16_t>(head.length, static_cast<uint16_t>(size)));
		logger.debug << tslog::lock << "SerialProtocol::slave_process_request(): got REQUEST request, sending " << length << " byte of data to master..." << tslog::endl;

		header head {STARTBYTE, static_cast<uint8_t>(seq), static_cast<uint8_t>(header_types::ACK), 0, static_cast<uint16_t>(length & max_size), 0};
		send_header(head);

		if (length) {
			/* send data and CRC16 */
			send_data(buf, length);

			/* receive header and check CRC8 */
			logger.debug << tslog::lock << "SerialProtocol::slave_process_request(): waiting for header from master..." << tslog::endl;

			header head_ack;
			if (! receive_header(head_ack, true)) {
/** \todo bad ack, do what? */
				logger.error << tslog::lock << "SerialProtocol::slave_process_request(): invalid ack packet received." << tslog::endl;
				return -2;
			}

			/* check ACK */
			if (static_cast<header_types>(head_ack.type) != header_types::ACK) {
/** \todo wrong header type, do what? */
				logger.error << tslog::lock << "SerialProtocol::slave_process_request(): wrong header type of ack packet." << tslog::endl;
				return -3;
			}

			/* check seq no. */
			const uint8_t seq_ack(static_cast<uint8_t>(seq + 1));
			if (head_ack.seq != seq_ack) {
/** \todo bad seq no., do what? */
				logger.error << tslog::lock << "SerialProtocol::slave_process_request(): wrong seq no. of ack packet." << tslog::endl;
				return -4;
			}
		}

		logger.debug << tslog::lock << "SerialProtocol::slave_process_request(): sending to master done." << tslog::endl;

		return 2; // request processed, data sent to master
	}

	default:
		logger.error << tslog::lock << "SerialProtocol::slave_process_request(): invalid header type, abort." << tslog::endl;
		return -5;
	}
}

template std::size_t SerialProtocol::master_send<const char*>(const char*&, const std::size_t, const uint8_t);
template std::size_t SerialProtocol::master_send<char*>(char*&, const std::size_t, const uint8_t tag);
template std::size_t SerialProtocol::master_send<std::streambuf>(std::streambuf&, const std::size_t, const uint8_t);
template std::size_t SerialProtocol::master_receive<char*>(char*&, const std::size_t, const uint8_t);
template std::size_t SerialProtocol::master_receive<std::streambuf>(std::streambuf&, const std::size_t, const uint8_t);
template int SerialProtocol::slave_process_request<char*>(header&, char*&, const std::size_t);
template int SerialProtocol::slave_process_request<std::streambuf>(header&, std::streambuf&, const std::size_t);

} /* namespace ctbot */
