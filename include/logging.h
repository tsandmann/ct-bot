/**
 * \file   logging.h
 * \author Timo Sandmann
 * \date   26.12.2016
 * \brief  Logging functions (just dummy version for AVR)
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include <cstring>

namespace tslog {

template <int level>
struct LogLevel {
	static constexpr int value = level;
};

using L_OFF = LogLevel<0>;
using L_ERROR = LogLevel<1>;
using L_INFO = LogLevel<2>;
using L_DEBUG = LogLevel<3>;
using L_FINE = LogLevel<4>;


template <bool flush>
struct EndLine {
	constexpr EndLine() noexcept {}
};

static constexpr EndLine<false> endl;
static constexpr EndLine<true> endlF;


struct Lock {
	constexpr Lock() noexcept {}
};

static constexpr Lock lock;


class LockPolicyST {
public:
	static void lock() noexcept {}
	static void unlock() noexcept {}
	static void line_lock() noexcept {}
	static void line_unlock() noexcept {}

};

struct NewLinePolicyNoCheck {};


template <bool active, class Level = tslog::LogLevel<0>, class NewLinePol = NewLinePolicyNoCheck, class LockPol = LockPolicyST>
class LogImpl : public Level, public NewLinePol, public LockPol {
public:
	LogImpl() noexcept {}

	~LogImpl() noexcept {}

	static constexpr bool on() noexcept {
		return active;
	}

	void flush() const noexcept {}

	LogImpl& operator<<(LogImpl& (*pf)(LogImpl&)) {
		return pf(*this);
	}

	const LogImpl& operator<<(const LogImpl& (*pf)(const LogImpl&)) const {
		return pf(*this);
	}
};

template <typename T, class Level, class NewLinePol, class LockPol>
inline LogImpl<false, Level, NewLinePol, LockPol>& operator<<(LogImpl<false, Level, NewLinePol, LockPol>& v, const T&) noexcept {
	return v;
}

template <typename T, class Level, class NewLinePol, class LockPol>
inline const LogImpl<false, Level, NewLinePol, LockPol>& operator<<(const LogImpl<false, Level, NewLinePol, LockPol>& v, const T&) noexcept {
	return v;
}

template <class Level, bool locking = false, bool check_newline = true>
struct Log {
	LogImpl<(Level::value >= tslog::L_ERROR::value), tslog::L_ERROR, NewLinePolicyNoCheck, LockPolicyST> error;
	LogImpl<(Level::value >= tslog::L_INFO::value), tslog::L_INFO, NewLinePolicyNoCheck, LockPolicyST> info;
	LogImpl<(Level::value >= tslog::L_DEBUG::value), tslog::L_DEBUG, NewLinePolicyNoCheck, LockPolicyST> debug;
	LogImpl<(Level::value >= tslog::L_FINE::value), tslog::L_FINE, NewLinePolicyNoCheck, LockPolicyST> fine;

	Log() : error(), info(), debug(), fine() {}

	static constexpr int get_level() noexcept {
		return Level::value;
	}

	void flush() const noexcept {}
};

} /* namespace tslog */

#endif // DEBUG_H_
