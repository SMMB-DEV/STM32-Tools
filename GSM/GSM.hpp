#pragma once

//#define GSM_URC_SUPPORT

#include "../Common.hpp"

#include <memory>	// unique_ptr



#ifndef HAL_UART_TIMEOUT_VALUE
#define HAL_UART_TIMEOUT_VALUE		(HAL_MAX_DELAY)
#endif



namespace STM32T
{
#ifdef HAL_UART_MODULE_ENABLED
	class GSM
	{
	public:
		enum class ErrorCode : uint8_t
		{
			OK,
			INVALID_PARAM,
			UART_ERR,
			WRONG_FORMAT,
			ERR,
			FAIL,
			UNKNOWN
		};
		
		class URC : public strv
		{
		public:
			const uint32_t timestamp;
			
			URC() : strv(), timestamp(0) {}
			URC(const char* const data, const size_t size, const uint32_t timestamp) : strv(data, size), timestamp(timestamp) {}
			URC(const strv& other, const uint32_t timestamp) : strv(other), timestamp(timestamp) {}
			
			bool compare_remove(const std::string_view& remove)
			{
				if (compare(0, remove.size(), remove) == 0)
				{
					remove_prefix(remove.size());
					return true;
				}

				return false;
			}
		};
		
	protected:
		static constexpr uint32_t DEFAUL_RECEIVE_TIMEOUT = 300, DEFAULT_IDLE_TIMEOUT = 20;
		static constexpr size_t DEFAULT_RESPONSE_LEN = 64, DEFAULT_ARG_LEN = 64;
		
		
		UART_HandleTypeDef* const p_huart;
		
		virtual void addURC(const strv token) = 0;
		
		enum class CommandType : uint8_t
		{
			Test,		// AT+CMD=?
			Read,		// AT+CMD?
			Execute,	// AT+CMDARGS
			Write,		// AT+CMD=ARGS
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
			
			if (type == CommandType::Write || type == CommandType::Test)
				SendUART("="sv);
			
			if (type == CommandType::Write || type == CommandType::Execute || type == CommandType::Bare)
				SendUART(args);
			else
				SendUART("?"sv);
			
			SendUART("\r"sv);
			
			if (buffer)
			{
				uint16_t len2 = len - 1;	// Hopefully len isn't 0.
				ErrorCode stat = ReceiveUART(buffer, len2, timeout, DEFAULT_IDLE_TIMEOUT);
				if (stat != ErrorCode::OK)
					return stat;
				
				buffer[len = len2] = 0;	// make it safe for C str functions
			}
			
			return ErrorCode::OK;
		}
		
		ErrorCode Command(vec<strv>& tokens, char* buffer, uint16_t len, const uint32_t timeout, const CommandType type, const strv cmd, const strv args = strv(), const bool allowSingleEnded = false)
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
			
			SendUART("\r"sv);
			
			len--;		// Hopefully len isn't 0.
			ErrorCode stat = ReceiveUART(buffer, len, timeout, DEFAULT_IDLE_TIMEOUT);
			if (stat != ErrorCode::OK)
				return stat;
			
			buffer[len] = 0;	// make it safe for C str functions
			
			Tokenize(strv(buffer, len), "\r\n"sv, tokens, !allowSingleEnded);
			
			return ErrorCode::OK;
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
		
		ErrorCode Standard(const vec<strv>& tokens)
		{
			for (size_t i = 0; i < tokens.size(); i++)
			{
				if (bool ok = tokens[i] == "OK"sv; ok || tokens[i].find("ERROR"sv) != strv::npos)	// OK or ERROR
				{
					for (i = i + 1; i < tokens.size(); i++)
						addURC(tokens[i]);
					
					return ok ? ErrorCode::OK : ErrorCode::ERR;
				}
				
				addURC(tokens[i]);
			}
			
			return ErrorCode::UNKNOWN;
		}
		
		ErrorCode Error(const vec<strv>& tokens)
		{
			for (size_t i = 0; i < tokens.size(); i++)
			{
				if (tokens[i].find("ERROR"sv) != strv::npos)
				{
					for (i = i + 1; i < tokens.size(); i++)
						addURC(tokens[i]);
					
					return ErrorCode::ERR;
				}
				
				addURC(tokens[i]);
			}
			
			return ErrorCode::UNKNOWN;
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode NoToken(const uint32_t timeout, const CommandType type, const strv& cmd, const func<ErrorCode (strv)>& handler, const strv& args = strv())
		{
			char buffer[LEN];
			uint16_t len = sizeof(buffer);
			ErrorCode code = Command(timeout, type, cmd, args, buffer, len);
			if (code != ErrorCode::OK)
				return code;
			
			return handler ? handler(strv(buffer, len)) : ErrorCode::INVALID_PARAM;
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode NoToken(const uint32_t timeout, const CommandType type, const strv& cmd, const func<ErrorCode (strv)>& handler, const char* const fmt = "", ...)
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
		ErrorCode Tokens(const uint32_t timeout, const CommandType type, const strv& cmd, const strv& args = strv(),
			const func<ErrorCode (vec<strv>&)>& op = nullptr, const bool allowSingleEnded = false)
		{
			char buffer[LEN];
			vec<strv> tokens;
			ErrorCode code = Command(tokens, buffer, sizeof(buffer), timeout, type, cmd, args, allowSingleEnded);
			if (code != ErrorCode::OK)
				return code;
			
			return op ? op(tokens) : Standard(tokens);
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode Tokens(const uint32_t timeout, const CommandType type, const strv& cmd,
			const func<ErrorCode (vec<strv>&)>& op, const bool allowSingleEnded = false, const char* const fmt = "", ...)
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
			
			return Tokens<LEN>(timeout, type, cmd, strv(args, argsLen), op, allowSingleEnded);
		}
		
		
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode FirstLastToken(const size_t expectedTokens, const uint32_t timeout, const CommandType type, const strv& cmd, const strv& args = strv(),
			const func<ErrorCode (vec<strv>&)>& op = nullptr)
		{
			return Tokens<LEN>(timeout, type, cmd, args, [&](vec<strv>& tokens) -> ErrorCode
			{
				if (tokens.size() != expectedTokens)
					return Standard(tokens);
				
				const auto first = tokens[0];
				bool ok = CompareAndRemove(tokens[0], cmd) && CompareAndRemove(tokens[0], ": "sv);
				
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
		ErrorCode FirstLastToken(const size_t expectedTokens, const uint32_t timeout, const CommandType type, const strv& cmd,
			const func<ErrorCode (vec<strv>&)>& op, const char* const fmt = "", ...)
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
			
			if (argsLen > sizeof(args))	//did not fit inside {args}
				Error_Handler();
			
			return FirstLastToken<LEN>(expectedTokens, timeout, type, cmd, strv(args, argsLen), op);
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode StrToken(char * const buf, const size_t max_len, const strv& cmd, const CommandType type, const uint32_t timeout = DEFAUL_RECEIVE_TIMEOUT, const strv& args = strv())
		{
			return Tokens<LEN>(timeout, type, cmd, args, [&](vec<strv>& tokens) -> ErrorCode
			{
				if (tokens.size() < 2 || tokens.back() != "OK"sv)
					return Error(tokens);
				
				tokens.pop_back();
				
				size_t len = 0;
				for (auto& token : tokens)
				{
					const size_t copy_len = std::min(token.size(), (max_len - 1) - len - 2 /*\r\n*/);
					
					memcpy(buf + len, token.data(), copy_len);
					len += copy_len;
					
					buf[len++] = '\r';
					buf[len++] = '\n';
				}
				
				buf[len - 2] = 0;	// Ignore last \r\n
				
				return ErrorCode::OK;
			});
		}
		
	public:
		GSM(UART_HandleTypeDef* uart) : p_huart(uart) {}
		
		ErrorCode AT()
		{
			return Tokens(DEFAUL_RECEIVE_TIMEOUT, CommandType::Execute, strv());
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
