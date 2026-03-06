#pragma once

//#define GSM_URC_SUPPORT

#include "../Common.hpp"
#include "../strv.hpp"

#include <memory>	// unique_ptr
#include <optional>



namespace STM32T
{
#ifdef HAL_UART_MODULE_ENABLED

#ifndef HAL_UART_TIMEOUT_VALUE
#define HAL_UART_TIMEOUT_VALUE		(HAL_MAX_DELAY)
#endif


#define CME_CODE(code)		CME_##code = -(1000 + code)

#define CME_CODE_10(code)	CME_CODE(code##0), CME_CODE(code##1), CME_CODE(code##2), CME_CODE(code##3), CME_CODE(code##4), \
CME_CODE(code##5), CME_CODE(code##6), CME_CODE(code##7), CME_CODE(code##8), CME_CODE(code##9)

#define CME_CODE_100(code)	CME_CODE_10(code##0), CME_CODE_10(code##1), CME_CODE_10(code##2), CME_CODE_10(code##3), CME_CODE_10(code##4), \
CME_CODE_10(code##5), CME_CODE_10(code##6), CME_CODE_10(code##7), CME_CODE_10(code##8), CME_CODE_10(code##9)

	
#define CMS_CODE(code)		CMS_##code = -(2000 + code)

#define CMS_CODE_10(code)	CMS_CODE(code##0), CMS_CODE(code##1), CMS_CODE(code##2), CMS_CODE(code##3), CMS_CODE(code##4), \
CMS_CODE(code##5), CMS_CODE(code##6), CMS_CODE(code##7), CMS_CODE(code##8), CMS_CODE(code##9)

#define CMS_CODE_100(code)	CMS_CODE_10(code##0), CMS_CODE_10(code##1), CMS_CODE_10(code##2), CMS_CODE_10(code##3), CMS_CODE_10(code##4), \
CMS_CODE_10(code##5), CMS_CODE_10(code##6), CMS_CODE_10(code##7), CMS_CODE_10(code##8), CMS_CODE_10(code##9)


	template <uint32_t DEF_RX_TO = 300, uint32_t DEF_IDLE_TO = 20, uint32_t SEND_GUARD = 0, uint32_t SEND_DELAY = 0, size_t DEF_RESP_LEN = 64, size_t DEF_ARG_LEN = 64>
	class GSM
	{
		uint32_t m_lastSend = 0;
		
	public:
		static constexpr size_t IMEI_LEN = 15, IMSI_LEN = 15;
		
		enum ErrorCode : int16_t
		{
			OK				= +0,
			INVALID_PARAM	= -1,
			INVALID			= -1,
			UART_ERR		= -2,
			TIMEOUT			= -3,
			WRONG_FORMAT	= -4,	// In response
			ERR				= -5,	// Failure on the module side
			FAIL			= -6,	// Failure on the MCU side
			BUF_FULL		= -7,	// todo: remove
			BIG_PARAM		= -7,
			ABORT			= -8,	// todo: remove?
			UNKNOWN			= -9,
			
			
			CME_CODE(0),
			CME_CODE(1), CME_CODE(2), CME_CODE(3), CME_CODE(4), CME_CODE(5), CME_CODE(6), CME_CODE(7), CME_CODE(8), CME_CODE(9),
			CME_CODE_10(1), CME_CODE_10(2), CME_CODE_10(3), CME_CODE_10(4), CME_CODE_10(5), CME_CODE_10(6), CME_CODE_10(7), CME_CODE_10(8), CME_CODE_10(9),
			CME_CODE_100(1), CME_CODE_100(2), CME_CODE_100(3), CME_CODE_100(4), CME_CODE_100(5), CME_CODE_100(6), CME_CODE_100(7), CME_CODE_100(8), CME_CODE_100(9),
			
			
			CMS_CODE(0),
			CMS_CODE(1), CMS_CODE(2), CMS_CODE(3), CMS_CODE(4), CMS_CODE(5), CMS_CODE(6), CMS_CODE(7), CMS_CODE(8), CMS_CODE(9),
			CMS_CODE_10(1), CMS_CODE_10(2), CMS_CODE_10(3), CMS_CODE_10(4), CMS_CODE_10(5), CMS_CODE_10(6), CMS_CODE_10(7), CMS_CODE_10(8), CMS_CODE_10(9),
			CMS_CODE_100(1), CMS_CODE_100(2), CMS_CODE_100(3), CMS_CODE_100(4), CMS_CODE_100(5)	//, CMS_CODE_100(6), CMS_CODE_100(7), CMS_CODE_100(8), CMS_CODE_100(9)
		};
		
		class URC : public strv
		{
		public:
			uint32_t m_timestamp;
			
			URC() : strv(), m_timestamp(0) {}
			URC(URC&& other) : strv(std::move((strv)other)), m_timestamp(other.m_timestamp) {}
			
			URC& operator=(URC&& other) &
			{
				(strv)*this = std::move((strv)other);
				m_timestamp = other.m_timestamp;
				
				return *this;
			}
			
			URC(const char* const data, const size_t size, const uint32_t timestamp) : strv(data, size), m_timestamp(timestamp) {}
			URC(const strv& other, const uint32_t timestamp) : strv(other), m_timestamp(timestamp) {}
		};
		
	protected:
		static constexpr uint32_t DEFAUL_RECEIVE_TIMEOUT = DEF_RX_TO, DEFAULT_IDLE_TIMEOUT = DEF_IDLE_TO, SEND_GUARD_TIME = SEND_GUARD, SEND_DELAY_TIME = SEND_DELAY;
		static constexpr size_t DEFAULT_RESPONSE_LEN = DEF_RESP_LEN, DEFAULT_ARG_LEN = DEF_ARG_LEN;
		
		static constexpr strv ESC = "\x1B"sv, CTRL_Z = "\x1A"sv;
		
		
		UART_HandleTypeDef* const p_huart;
		bool m_noSendWait = false, m_noSendDelay = false;
		
		
		virtual void addURC(const strv token) = 0;
	
		void addURCs(const vec<strv>& tokens)
		{
			for (auto& token: tokens)
				addURC(token);
		}
		
		void addURCFromBuf(const strv buf)
		{
			vec<strv> tokens;
			buf.tokenize2("\r\n"sv, tokens, true);
			addURCs(tokens);
		}
		
		enum class CommandType : uint8_t
		{
			Test,		// AT+CMD=?\r
			Read,		// AT+CMD?\r
			Execute,	// AT+CMDARGS\r
			Write,		// AT+CMD=ARGS\r
			Bare		// ARGS
		};
		
		void SendUART(strv data)
		{
			HAL_UART_Transmit(p_huart, (uint8_t*)data.data(), data.length(), HAL_UART_TIMEOUT_VALUE);
		}
		
		virtual int16_t ReceiveUART(char *buffer, uint16_t len, const uint32_t timeout, const uint32_t idle_timeout) = 0;
		
		ErrorCode Command(const uint32_t timeout, const CommandType type, const strv cmd, const strv args, char* buffer, uint16_t& len)
		{
			const bool data_sent = type != CommandType::Bare || !args.empty();
			
			if constexpr (SEND_GUARD_TIME)
			{
				if (data_sent)
				{
					if (m_noSendWait)
						m_noSendWait = false;
					else
					{
						while (HAL_GetTick() - m_lastSend <= SEND_GUARD_TIME);
						m_lastSend = HAL_GetTick();
					}
				}
			}
			
			if (type != CommandType::Bare)
			{
				SendUART("AT"sv);
				SendUART(cmd);
			}
			
			if ((type == CommandType::Write && !args.empty()) || type == CommandType::Test)
				SendUART("="sv);
			
			if (type == CommandType::Write || type == CommandType::Execute || type == CommandType::Bare)
				SendUART(args);
			else
				SendUART("?"sv);
			
			if (type != CommandType::Bare)
				SendUART("\r"sv);
			
			if constexpr (SEND_DELAY_TIME)
			{
				if (data_sent)
				{
					if (m_noSendDelay)
						m_noSendDelay = false;
					else
						HAL_Delay(SEND_DELAY_TIME);
				}
			}
			
			if (buffer && len)
			{
				int16_t len2 = ReceiveUART(buffer, len - 1, timeout, DEFAULT_IDLE_TIMEOUT);
				if (len2 < OK)
					return ErrorCode(len2);
				
				buffer[len = len2] = 0;		// Make it safe for C str functions
			}
			
			return OK;
		}
		
		ErrorCode Standard(vec<strv>& tokens)
		{
			ErrorCode ret = UNKNOWN;
			for (size_t i = 0; i < tokens.size(); i++)
			{
				if (tokens[i] == "OK"sv)
				{
					ret = OK;
					tokens.erase(tokens.begin() + i);
					break;
				}
			}
			
			if (ret == UNKNOWN)
				ret = Error(tokens);
			else
				addURCs(tokens);
			
			return ret;
		}
		
		ErrorCode Error(vec<strv>& tokens)
		{
			ErrorCode ret = UNKNOWN;
			for (size_t i = 0; i < tokens.size(); i++)
			{
				uint16_t code;
				if (1 == std::sscanf(tokens[i].data(), "+CME ERROR: %3hu", &code))
				{
					tokens.erase(tokens.begin() + i);
					ret = ErrorCode(-1000 - code);
					break;
				}
				
				if (1 == std::sscanf(tokens[i].data(), "+CMS ERROR: %3hu", &code))
				{
					tokens.erase(tokens.begin() + i);
					ret = ErrorCode(-2000 - code);
					break;
				}
				
				if (tokens[i].find("ERROR"sv) != strv::npos)
				{
					tokens.erase(tokens.begin() + i);
					ret = ERR;
					break;
				}
			}
			
			addURCs(tokens);
			
			return ret;
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode NoToken(const uint32_t timeout, const CommandType type, const strv cmd, const func<ErrorCode (strv)>& handler, const strv args = strv())
		{
			if (!handler)
				return INVALID_PARAM;
			
			char buffer[LEN];
			uint16_t len = sizeof(buffer);
			ErrorCode code = Command(timeout, type, cmd, args, buffer, len);
			if (code != ErrorCode::OK)
				return code;
			
			return handler(strv(buffer, len));
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode NoToken(const uint32_t timeout, const CommandType type, const strv cmd, const func<ErrorCode (strv)>& handler, const char* const fmt, ...)
		{
			if (!fmt)
				return ErrorCode::INVALID_PARAM;
			
			char args[ARG_LEN];
			
			va_list print_args;
			va_start(print_args, fmt);
			
			int argsLen = vsnprintf(args, sizeof(args), fmt, print_args);
			if (argsLen < 0)
				return ErrorCode::UNKNOWN;
			
			va_end(print_args);
			
			if (argsLen >= sizeof(args))	// did not fit inside {args}
				return BIG_PARAM;
			
			return NoToken<LEN>(timeout, type, cmd, handler, strv(args, argsLen));
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode Tokens2(const uint32_t timeout, const CommandType type, const strv cmd, const strv args, const func<ErrorCode (vec<strv>&)>& op,
			const bool allowSingleEnded = false)
		{
			if (!op)
				return INVALID_PARAM;
			
			char buffer[LEN];
			uint16_t len = sizeof(buffer);
			ErrorCode code = Command(timeout, type, cmd, args, buffer, len);
			if (code != OK)
				return code;
			
			vec<strv> tokens;
			strv(buffer, len).tokenize2("\r\n"sv, tokens, !allowSingleEnded);
			
			return op(tokens);
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode Tokens3(const uint32_t timeout, const CommandType type, const strv cmd, const strv args, const func<ErrorCode (vec<strv>&)>& op,
			const bool allowSingleEnded = false)
		{
			if (!op)
				return INVALID_PARAM;
			
			char buffer[LEN];
			uint16_t len = sizeof(buffer);
			ErrorCode code = Command(timeout, type, cmd, args, buffer, len);
			if (code != OK)
				return code;
			
			vec<strv> tokens;
			strv(buffer, len).tokenize2("\r\n"sv, tokens, !allowSingleEnded);
			
			if (tokens.size() == 0 || tokens.back() != "OK"sv)
				return Error(tokens);
			
			tokens.pop_back();
			
			return op(tokens);
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode SingleToken(const uint32_t timeout, const CommandType type, const strv cmd, const strv args = strv(), const strv token = "OK"sv, const bool allowSingleEnded = false)
		{
			return Tokens2<LEN>(timeout, type, cmd, args, [&](vec<strv>& tokens)
			{
				if (tokens.size() != 1 || tokens[0] != token)
					return Error(tokens);
				
				return OK;
			}, allowSingleEnded);
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode SingleToken(const uint32_t timeout, const CommandType type, const strv cmd, const strv token, const bool allowSingleEnded, const char* const fmt, ...)
		{
			if (!fmt)
				return INVALID;
			
			char args[ARG_LEN];
			
			va_list print_args;
			va_start(print_args, fmt);
			
			int argsLen = vsnprintf(args, sizeof(args), fmt, print_args);
			if (argsLen < 0)
				return ErrorCode::UNKNOWN;
			
			va_end(print_args);
			
			if (argsLen >= sizeof(args))
				return BIG_PARAM;
			
			return SingleToken<LEN>(timeout, type, cmd, strv(args, argsLen), token, allowSingleEnded);
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode ResponseToken(const size_t expectedTokens, const uint32_t timeout, const CommandType type, const strv cmd, const strv args, const size_t ok_pos,
			const func<ErrorCode (vec<strv>&)>& op)
		{
			if (!op)
				return INVALID_PARAM;
			
			return Tokens2<LEN>(timeout, type, cmd, args, [&](vec<strv>& tokens) -> ErrorCode
			{
				bool ok = tokens.size() == expectedTokens;
				
				if (ok && ok_pos < expectedTokens)
				{
					ok &= tokens[ok_pos] == "OK"sv;
					if (ok)
						tokens.erase(tokens.begin() + ok_pos);
				}
				
				if (ok)
				{
					const auto first = tokens[0];
					ok &= tokens[0].remove_prefix(cmd) && (tokens[0].remove_prefix(": "sv) || tokens[0].remove_prefix(":"sv));
					if (!ok)
						tokens[0] = first;
				}
				
				if (!ok)
				{
					//addURCs(tokens);
					//return UNKNOWN;
					return Error(tokens);
				}
				
				return op(tokens);
			});
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode ResponseToken(const uint32_t timeout, const CommandType type, const strv cmd, const strv args, const func<ErrorCode (strv)>& op, const bool ok_last = true)
		{
			if (!op)
				return INVALID_PARAM;
			
			return ResponseToken(2, timeout, type, cmd, args, ok_last, [&](vec<strv>& tokens)
			{
				return op(tokens[0]);
			});
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode ResponseToken(const size_t expectedTokens, const uint32_t timeout, const CommandType type, const strv cmd, const size_t ok_pos,
			const func<ErrorCode (vec<strv>&)>& op, const char * const fmt, ...)
		{
			if (!fmt)
				return INVALID_PARAM;
			
			char args[ARG_LEN];
			
			va_list print_args;
			va_start(print_args, fmt);
			
			int argsLen = vsnprintf(args, sizeof(args), fmt, print_args);
			if (argsLen < 0)
				return INVALID_PARAM;
			
			va_end(print_args);
			
			if (argsLen >= sizeof(args))
				return BIG_PARAM;
			
			return ResponseToken<LEN>(expectedTokens, timeout, type, cmd, strv(args, argsLen), ok_pos, op);
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode ResponseToken(const uint32_t timeout, const CommandType type, const strv cmd,
			const func<ErrorCode (strv)>& op, const bool ok_last, const char * const fmt, ...)
		{
			if (!fmt)
				return INVALID_PARAM;
			
			char args[ARG_LEN];
			
			va_list print_args;
			va_start(print_args, fmt);
			
			int argsLen = vsnprintf(args, sizeof(args), fmt, print_args);
			if (argsLen < 0)
				return INVALID_PARAM;
			
			va_end(print_args);
			
			if (argsLen >= sizeof(args))
				return BIG_PARAM;
			
			return ResponseToken<LEN>(timeout, type, cmd, strv(args, argsLen), op, ok_last);
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode DelayedResponseToken(const uint32_t timeout, const CommandType type, const strv cmd, const strv args, const func<ErrorCode (strv)>& op)
		{
			bool done = false;
			const uint32_t start = HAL_GetTick() + SEND_GUARD_TIME;
			
			ErrorCode code = Tokens2<LEN>(timeout, type, cmd, args, [&](vec<strv>& tokens)
			{
				if (tokens.size() < 1  || tokens.size() > 2 || tokens[0] != "OK"sv)
					return Error(tokens);
				
				if (tokens.size() == 2)		// In case the response wasn't actually delayed
				{
					done = true;
					
					const auto t = tokens[1];
					const bool ok = tokens[1].remove_prefix(cmd) && (tokens[1].remove_prefix(": "sv) || tokens[1].remove_prefix(":"sv));
					if (!ok)
					{
						addURC(t);
						return UNKNOWN;
					}
					
					return op(tokens[1]);
				}
				
				return OK;
			});
			
			if (code != OK || done)
				return code;
			
			m_noSendWait = true;
			
			return ResponseToken<LEN>(1, Time::Remaining_Tick(start, timeout), CommandType::Bare, cmd, strv(), SIZE_MAX, [&](vec<strv>& tokens) { return op(tokens[0]); });
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode DelayedResponseToken(const uint32_t timeout, const CommandType type, const strv cmd, const func<ErrorCode (strv)>& op, const char * const fmt, ...)
		{
			if (!fmt)
				return INVALID_PARAM;
			
			char args[ARG_LEN];
			
			va_list print_args;
			va_start(print_args, fmt);
			
			int argsLen = vsnprintf(args, sizeof(args), fmt, print_args);
			if (argsLen < 0)
				return INVALID_PARAM;
			
			va_end(print_args);
			
			if (argsLen >= sizeof(args))
				return BIG_PARAM;
			
			return DelayedResponseToken<LEN>(timeout, type, cmd, strv(args, argsLen), op);
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		[[deprecated("Use StrToken2() instead.")]]
		ErrorCode StrToken(char * const buf, size_t max_len, const CommandType type, const strv cmd, const strv args = strv())
		{
			return NoToken<LEN>(DEFAUL_RECEIVE_TIMEOUT, type, cmd, [&](strv str)
			{
				const strv orig = str;
				
				if (!str.remove_suffix("\r\n\r\nOK\r\n"sv) || !str.remove_prefix("\r\n"sv))
				{
					addURCFromBuf(orig);
					return UNKNOWN;
				}
				
				const size_t len = std::min(str.length(), max_len - 1);
				memcpy(buf, str.data(), len);
				buf[len] = 0;
				
				return OK;
			}, args);
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		int16_t StrToken2(char * const buf, size_t max_len, const CommandType type, const strv cmd, const strv args = strv())
		{
			return NoToken<LEN>(DEFAUL_RECEIVE_TIMEOUT, type, cmd, [&](strv str) -> ErrorCode
			{
				const strv orig = str;
				
				if (!str.remove_suffix("\r\n\r\nOK\r\n"sv) || !str.remove_prefix("\r\n"sv))
				{
					addURCFromBuf(orig);
					return UNKNOWN;
				}
				
				const size_t len = std::min(str.length(), max_len - 1);
				memcpy(buf, str.data(), len);
				buf[len] = 0;
				
				return ErrorCode(len);
			}, args);
		}
		
	public:
		struct DateTime
		{
			static constexpr uint8_t MonthDays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };	// February can be 29
			
			uint8_t yy, MM, dd, hh, mm, ss;
			int8_t zz;
			
			[[deprecated]] static bool Parse(DateTime& dt, const strv view)
			{
				return 7 == sscanf(view.data(), "\"%2hhu/%2hhu/%2hhu,%2hhu:%2hhu:%2hhu%3hhd", &dt.yy, &dt.MM, &dt.dd, &dt.hh, &dt.mm, &dt.ss, &dt.zz) && dt.IsSet();
			}
			
			static std::optional<DateTime> Parse(strv view)
			{
				DateTime dt;
				
				if (int n; sscanf(view.data(), "\"%2hhu/%2hhu/%2hhu,%2hhu:%2hhu:%2hhu%n", &dt.yy, &dt.MM, &dt.dd, &dt.hh, &dt.mm, &dt.ss, &n) == 6 && n > 0)
				{
					view.remove_prefix(n);
					if (sscanf(view.data(), "%3hhd\"", &dt.zz) != 1)
					{
						if (view == "\""sv)
							dt.zz = 0;
						else
							return std::nullopt;
					}
					
					if (dt.IsSet())
						return dt;
				}
				
				return std::nullopt;
			}
			
			[[deprecated]] static bool ParseNTP(DateTime& dt, const strv view)
			{
				if (int n; sscanf(view.data(), "%2hhu/%2hhu/%2hhu,%2hhu:%2hhu:%2hhu%n", &dt.yy, &dt.MM, &dt.dd, &dt.hh, &dt.mm, &dt.ss, &n) == 6 && n > 0)
				{
					if (sscanf(view.data() + n, "%3hhd", &dt.zz) != 1)
						dt.zz = 0;
					
					return dt.IsSet();
				}
				
				return false;
			}
			
			static std::optional<DateTime> ParseNTP(strv view)
			{
				DateTime dt;
				
				if (int n; sscanf(view.data(), "%2hhu/%2hhu/%2hhu,%2hhu:%2hhu:%2hhu%n", &dt.yy, &dt.MM, &dt.dd, &dt.hh, &dt.mm, &dt.ss, &n) == 6 && n > 0)
				{
					view.remove_prefix(n);
					if (sscanf(view.data() + n, "%3hhd", &dt.zz) != 1)
					{
						if (view.empty())
							dt.zz = 0;
						else
							return std::nullopt;
					}
					
					if (dt.IsSet())
						return dt;
				}
				
				return std::nullopt;
			}
			
			static std::optional<DateTime> ParseSMS(strv view)
			{
				DateTime dt;
				
				if (7 == sscanf(view.data(), "\"20%2hhu/%2hhu/%2hhu %2hhu:%2hhu:%2hhu%3hhd\"", &dt.yy, &dt.MM, &dt.dd, &dt.hh, &dt.mm, &dt.ss, &dt.zz))
				{
					if (dt.IsSet())
						return dt;
				}
				
				return std::nullopt;
			}
			
			static bool IsLeapYear(uint8_t yy)
			{
				// These extra days occur in each year that is an integer multiple of 4 (except for years evenly divisible by 100, but not by 400) [https://en.wikipedia.org/wiki/Leap_year]
				return yy % 4 == 0 && yy != 0;
			}
			
			bool IsSet() const
			{
				return yy <= 99 && MM >= 1 && MM <= 12 && dd >= 1 && dd <= MonthDays[MM - 1] + (MM == 2 && IsLeapYear(yy)) && hh <= 23 && mm <= 59 && ss <= 60 && zz >= -47 && zz <= 48;
			}
			
			const char* Format() const
			{
				static char fmt[2 + 7 * 3] = { 0 };
				
				sprintf(fmt, "20%02hhu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu%+hhd", yy, MM, dd, hh, mm, ss, zz);
				return fmt;
			}
		};
		
		GSM(UART_HandleTypeDef* uart) : p_huart(uart) {}
		
		ErrorCode AT(const uint32_t timeout = DEFAUL_RECEIVE_TIMEOUT)
		{
			return SingleToken(timeout, CommandType::Execute, {});
		}
		
		void Custom(strv command)
		{
			uint16_t temp;
			Command(0, CommandType::Execute, command, strv(), nullptr, temp);
		}
		
		int16_t GetBrand(char *const buf, const size_t max_len)
		{
			return StrToken2(buf, max_len, CommandType::Execute, "+CGMI"sv);
		}
		
		int16_t GetModel(char *const buf, const size_t max_len)
		{
			return StrToken2(buf, max_len, CommandType::Execute, "+CGMM"sv);
		}
		
		/**
		* @param rev - Will be null-terminated.
		*/
		int16_t GetRevision(char *const rev, const size_t max_len)
		{
			return StrToken2(rev, max_len, CommandType::Execute, "+CGMR"sv);
		}
		
		/**
		* @param imei - Must have at least IMEI_LEN + 1 bytes. It will have exactly IMEI_LEN characters and will be null-terminated on success.
		*/
		ErrorCode GetIMEI(char *const imei)
		{
			static_assert(IMEI_LEN + 4 + 6 < DEFAULT_RESPONSE_LEN);
			
			int16_t len = StrToken2(imei, IMEI_LEN + 1, CommandType::Execute, "+CGSN"sv);
			if (len != IMEI_LEN)
				return WRONG_FORMAT;
			
			uint8_t sum = 0;
			for (size_t i = 0; i < IMEI_LEN; i += 2)
				sum += imei[i] - '0';
			
			for (size_t i = 1; i < IMEI_LEN; i += 2)
			{
				const uint8_t _double = (imei[i] - '0') * 2;
				sum += _double / 10 + _double % 10;
			}
			
			return (sum % 10 == 0) ? OK : WRONG_FORMAT;
		}
		
		ErrorCode GetIMEI(uint64_t &imei)
		{
			char imei_s[IMEI_LEN + 1];
			ErrorCode code = GetIMEI(imei_s);
			if (code != OK)
				return code;
			
			return strv(imei_s, IMEI_LEN).ExtractInteger(imei) == IMEI_LEN ? OK : WRONG_FORMAT;
		}
		
		/**
		* @param imsi - Must have at least IMSI_LEN + 1 bytes. It will have exactly IMSI_LEN characters and will be null-terminated on success.
		*/
		ErrorCode GetIMSI(char *const imsi)
		{
			static_assert(IMSI_LEN + 4 + 6 < DEFAULT_RESPONSE_LEN);
			
			const int16_t len = StrToken2(imsi, IMSI_LEN + 1, CommandType::Execute, "+CIMI"sv);
			return len == IMSI_LEN ? OK : WRONG_FORMAT;
		}
		
		ErrorCode GetIMSI(uint16_t &mcc, uint8_t &mnc)
		{
			char imsi[IMSI_LEN + 1];
			
			const ErrorCode code = GetIMSI(imsi);
			if (code != OK)
				return code;
			
			strv imsi_sv(imsi, IMSI_LEN);
			if (imsi_sv.ExtractInteger(mcc, 0, 3) != 3 || imsi_sv.ExtractInteger(mnc, 3, 2) != 2)
				return WRONG_FORMAT;
			
			return OK;
		}
	};
#endif	// HAL_UART_MODULE_ENABLED
}
