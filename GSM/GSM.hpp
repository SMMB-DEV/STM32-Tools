#pragma once

//#define GSM_URC_SUPPORT

#include "../Common.hpp"

#include <memory>	// unique_ptr
#include <optional>



namespace STM32T
{
#ifdef HAL_UART_MODULE_ENABLED
	
#ifndef HAL_UART_TIMEOUT_VALUE
#define HAL_UART_TIMEOUT_VALUE		(HAL_MAX_DELAY)
#endif
	
	class GSM
	{
	public:
		enum ErrorCode : uint8_t
		{
			OK,
			INVALID_PARAM,
			UART_ERR,
			TIMEOUT,
			WRONG_FORMAT,	// In response
			ERR,
			FAIL,			// To execute command (received data was OK)
			BUF_FULL,
			ABORT,
			UNKNOWN
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
		static constexpr uint32_t DEFAUL_RECEIVE_TIMEOUT = 300, DEFAULT_IDLE_TIMEOUT = 20;
		static constexpr size_t DEFAULT_RESPONSE_LEN = 64, DEFAULT_ARG_LEN = 64;
		
		
		UART_HandleTypeDef* const p_huart;
		
		virtual void addURC(const strv token) = 0;
	
		void addURCs(const vec<strv>& tokens)
		{
			for (auto& token: tokens)
				addURC(token);
		}
		
		void addURCFromBuf(const strv buf)
		{
			vec<strv> tokens;
			buf.tokenize("\r\n"sv, tokens, true);
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
		
		virtual void SendUART(strv data)
		{
			HAL_UART_Transmit(p_huart, (uint8_t*)data.data(), data.length(), HAL_UART_TIMEOUT_VALUE);
		}
		
		/**
		* @param len - The length to be received is passed to the function, then len is modified inside the function to the number of bytes actually received.
		*/
		virtual ErrorCode ReceiveUART(char *buffer, uint16_t& len, const uint32_t timeout, const uint32_t idle_timeout) = 0;
		
		ErrorCode Command(const uint32_t timeout, const CommandType type, const strv cmd, const strv args, char* buffer, uint16_t& len)
		{
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
			
			if (buffer)
			{
				uint16_t len2 = len - 1;	// Hopefully len isn't 0.
				if (const ErrorCode code = ReceiveUART(buffer, len2, timeout, DEFAULT_IDLE_TIMEOUT); code != OK)
					return code;
				
				buffer[len = len2] = 0;		// Make it safe for C str functions
			}
			
			return OK;
		}
		
		[[deprecated]] ErrorCode Command(vec<strv>& tokens, char* buffer, uint16_t len, const uint32_t timeout, const CommandType type, const strv cmd, const strv args = strv(), const bool allowSingleEnded = false)
		{
			if (type != CommandType::Bare)
			{
				SendUART("AT"sv);
				SendUART(cmd);
			}
			
			if (type == CommandType::Write || type == CommandType::Test)
				SendUART("="sv);
			
			if (type == CommandType::Write || type == CommandType::Execute || type == CommandType::Bare)
				SendUART(args);
			else
				SendUART("?"sv);
			
			if (type != CommandType::Bare)
				SendUART("\r"sv);
			
			len--;		// Hopefully len isn't 0.
			if (ReceiveUART(buffer, len, timeout, DEFAULT_IDLE_TIMEOUT) != OK)
				return UART_ERR;
			
			buffer[len] = 0;	// make it safe for C str functions
			
			strv(buffer, len).tokenize("\r\n"sv, tokens, !allowSingleEnded);
			
			return OK;
		}
		
		bool CompareAndRemove(strv& token, const strv& remove)
		{
			if (token.compare(0, remove.size(), remove) == 0)
			{
				token.remove_prefix(remove.size());
				return true;
			}

			return false;
		}
		
		ErrorCode Standard(vec<strv>& tokens)
		{
			ErrorCode ret = UNKNOWN;
			for (size_t i = 0; i < tokens.size(); i++)
			{
				if (bool ok = tokens[i] == "OK"sv; ok || tokens[i].find("ERROR"sv) != strv::npos)	// OK or ERROR
				{
					ret = ok ? OK : ERR;
					tokens.erase(tokens.begin() + i);
					break;
				}
			}
			
			addURCs(tokens);
			
			return ret;
		}
		
		ErrorCode Error(vec<strv>& tokens)
		{
			ErrorCode ret = UNKNOWN;
			for (size_t i = 0; i < tokens.size(); i++)
			{
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
			
			if (argsLen > sizeof(args))	// did not fit inside {args}
				Error_Handler();
			
			return NoToken<LEN>(timeout, type, cmd, handler, strv(args, argsLen));
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		[[deprecated]] ErrorCode Tokens(const uint32_t timeout, const CommandType type, const strv cmd, const strv args = strv(),
			const func<ErrorCode (vec<strv>&)>& op = nullptr, const bool allowSingleEnded = false)
		{
			char buffer[LEN];
			uint16_t len = sizeof(buffer);
			ErrorCode code = Command(timeout, type, cmd, args, buffer, len);
			if (code != OK)
				return code;
			
			vec<strv> tokens;
			strv(buffer, len).tokenize("\r\n"sv, tokens, !allowSingleEnded);
			
			// fixme: Standard() might not return the correct code.
			return op ? op(tokens) : Standard(tokens);
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode Tokens2(const uint32_t timeout, const CommandType type, const strv cmd, const strv args,
			const func<ErrorCode (vec<strv>&)>& op, const bool allowSingleEnded = false)
		{
			if (!op)
				return INVALID_PARAM;
			
			char buffer[LEN];
			uint16_t len = sizeof(buffer);
			ErrorCode code = Command(timeout, type, cmd, args, buffer, len);
			if (code != OK)
				return code;
			
			vec<strv> tokens;
			strv(buffer, len).tokenize("\r\n"sv, tokens, !allowSingleEnded);
			
			return op(tokens);
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		[[deprecated]] ErrorCode Tokens(const uint32_t timeout, const CommandType type, const strv cmd,
			const func<ErrorCode (vec<strv>&)>& op, const bool allowSingleEnded, const char* const fmt, ...)
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
			
			if (argsLen > sizeof(args))	// did not fit inside {args}
			{
				Error_Handler();
				return ERR;
			}
			
			return Tokens<LEN>(timeout, type, cmd, strv(args, argsLen), op, allowSingleEnded);
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
				return ErrorCode::INVALID_PARAM;
			
			char args[ARG_LEN];
			
			va_list print_args;
			va_start(print_args, fmt);
			
			int argsLen = vsnprintf(args, sizeof(args), fmt, print_args);
			if (argsLen < 0)
				return ErrorCode::UNKNOWN;
			
			va_end(print_args);
			
			if (argsLen > sizeof(args))	// did not fit inside {args}
			{
				Error_Handler();
				return ERR;
			}
			
			return SingleToken<LEN>(timeout, type, cmd, strv(args, argsLen), token, allowSingleEnded);
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		[[deprecated]] ErrorCode FirstLastToken(const size_t expectedTokens, const uint32_t timeout, const CommandType type, const strv& cmd, const strv& args = strv(),
			const func<ErrorCode (vec<strv>&)>& op = nullptr)
		{
			return Tokens<LEN>(timeout, type, cmd, args, [&](vec<strv>& tokens) -> ErrorCode
			{
				if (tokens.size() != expectedTokens)
					return Standard(tokens);
				
				const auto first = tokens[0];
				bool ok = CompareAndRemove(tokens[0], cmd) && (CompareAndRemove(tokens[0], ": "sv) || CompareAndRemove(tokens[0], ":"sv));
				
				if (expectedTokens > 1)
				{
					ok &= tokens.back() == "OK"sv;
					tokens.pop_back();
				}
				
				if (!ok)
				{
					tokens[0] = first;
					
					for (const auto& token : tokens)
						addURC(token);
					
					return ErrorCode::UNKNOWN;
				}
				
				//return op ? op(tokens) : Standard(tokens);
				return op ? op(tokens) : ErrorCode::OK;		// fixme: Doesn't handle URCs!
			});
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		[[deprecated]] ErrorCode FirstLastToken(const size_t expectedTokens, const uint32_t timeout, const CommandType type, const strv cmd,
			const func<ErrorCode (vec<strv>&)>& op, const char* const fmt, ...)
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
			
			if (argsLen > sizeof(args))	// Did not fit inside {args}
			{
				Error_Handler();
				return INVALID_PARAM;
			}
			
			return FirstLastToken<LEN>(expectedTokens, timeout, type, cmd, strv(args, argsLen), op);
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
					ok &= CompareAndRemove(tokens[0], cmd) && (CompareAndRemove(tokens[0], ": "sv) || CompareAndRemove(tokens[0], ":"sv));
					if (!ok)
						tokens[0] = first;
				}
				
				if (!ok)
				{
					addURCs(tokens);
					return UNKNOWN;
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
			
			if (argsLen > sizeof(args))		// Did not fit inside {args}
			{
				Error_Handler();
				return INVALID_PARAM;
			}
			
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
			
			if (argsLen > sizeof(args))		// Did not fit inside {args}
			{
				Error_Handler();
				return INVALID_PARAM;
			}
			
			return ResponseToken<LEN>(timeout, type, cmd, strv(args, argsLen), op, ok_last);
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode DelayedResponseToken(const uint32_t timeout, const CommandType type, const strv cmd, const strv args, const func<ErrorCode (strv)>& op)
		{
			bool done = false;
			
			ErrorCode code = Tokens2<LEN>(timeout, type, cmd, args, [&](vec<strv>& tokens)
			{
				if (tokens.size() < 1  || tokens.size() > 2 || tokens[0] != "OK"sv)
					return Error(tokens);
				
				if (tokens.size() == 2)		// In case the response wasn't actually delayed
				{
					done = true;
					
					const auto t = tokens[1];
					const bool ok = CompareAndRemove(tokens[1], cmd) && (CompareAndRemove(tokens[1], ": "sv) || CompareAndRemove(tokens[1], ":"sv));
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
			
			return ResponseToken<LEN>(1, timeout, CommandType::Bare, cmd, strv(), SIZE_MAX, [&](vec<strv>& tokens) { return op(tokens[0]); });
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
			
			if (argsLen > sizeof(args))		// Did not fit inside {args}
			{
				Error_Handler();
				return INVALID_PARAM;
			}
			
			return DelayedResponseToken<LEN>(timeout, type, cmd, strv(args, argsLen), op);
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode StrToken(char * const buf, size_t max_len, const strv cmd, const CommandType type, const strv args = strv())
		{
			return NoToken<LEN>(DEFAUL_RECEIVE_TIMEOUT, type, cmd, [&](strv str)
			{
				if (str.length() <= 10 || str.compare(str.length() - 8, strv::npos, "\r\n\r\nOK\r\n"sv) != 0 || str.compare(0, 2, "\r\n"sv) != 0)
				{
					addURCFromBuf(str);
					return UNKNOWN;
				}
				
				str.remove_suffix(8);
				str.remove_prefix(2);
				
				const size_t len = std::min(str.length(), max_len - 1);
				memcpy(buf, str.data(), len);
				buf[len] = 0;
				
				return OK;
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
		
		ErrorCode AT()
		{
			return SingleToken(DEFAUL_RECEIVE_TIMEOUT, CommandType::Execute, strv());
		}
		
		void Custom(strv command)
		{
			uint16_t temp;
			Command(0, CommandType::Execute, command, strv(), nullptr, temp);
		}
		
		ErrorCode GetBrand(char * const buf, const size_t max_len)
		{
			return StrToken(buf, max_len, "+CGMI"sv, CommandType::Execute);
		}
		
		ErrorCode GetModel(char * const buf, const size_t max_len)
		{
			return StrToken(buf, max_len, "+CGMM"sv, CommandType::Execute);
		}
		
		ErrorCode GetRevision(char * const buf, const size_t max_len)
		{
			return StrToken(buf, max_len, "+CGMR"sv, CommandType::Execute);
		}
		
		ErrorCode GetIMEI(char * const buf, const size_t max_len)
		{
			return StrToken(buf, max_len, "+CGSN"sv, CommandType::Execute);
		}
	};
#endif	// HAL_UART_MODULE_ENABLED
}
