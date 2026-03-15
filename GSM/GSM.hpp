#pragma once

#include "../Common.hpp"
#include "../strv.hpp"
#include "../span.hpp"
#include "../Log.hpp"

#include <memory>	// unique_ptr
#include <optional>



#ifdef STM32T_IWDG_TIMEOUT
extern "C" IWDG_HandleTypeDef hiwdg;
#endif	// STM32T_IWDG_TIMEOUT



#ifdef HAL_UART_MODULE_ENABLED
namespace STM32T
{
#define CM_CODE(name, code)		name = -(code)

#define CM_CODE_10(name, code)		CM_CODE(name##0, (code) * 10 + 0), CM_CODE(name##1, (code) * 10 + 1), \
CM_CODE(name##2, (code) * 10 + 2), CM_CODE(name##3, (code) * 10 + 3), CM_CODE(name##4, (code) * 10 + 4), CM_CODE(name##5, (code) * 10 + 5), \
CM_CODE(name##6, (code) * 10 + 6), CM_CODE(name##7, (code) * 10 + 7), CM_CODE(name##8, (code) * 10 + 8), CM_CODE(name##9, (code) * 10 + 9)

#define CM_CODE_100(name, code)		CM_CODE_10(name##0, (code) * 10 + 0), CM_CODE_10(name##1, (code) * 10 + 1), \
CM_CODE_10(name##2, (code) * 10 + 2), CM_CODE_10(name##3, (code) * 10 + 3), CM_CODE_10(name##4, (code) * 10 + 4), CM_CODE_10(name##5, (code) * 10 + 5), \
CM_CODE_10(name##6, (code) * 10 + 6), CM_CODE_10(name##7, (code) * 10 + 7), CM_CODE_10(name##8, (code) * 10 + 8), CM_CODE_10(name##9, (code) * 10 + 9)


	template <uint32_t DEF_RX_TO = 300, uint32_t DEF_IDLE_TO = 20, uint32_t SEND_GUARD = 0, uint32_t SEND_DELAY = 0, size_t DEF_RESP_LEN = 64, size_t DEF_ARG_LEN = 64>
	class GSM
	{
		using This = GSM<DEF_RX_TO, DEF_IDLE_TO, SEND_GUARD, SEND_DELAY, DEF_RESP_LEN, DEF_ARG_LEN>;
		
	public:
		static constexpr size_t IMEI_LEN = 15, IMSI_LEN = 15;
		static constexpr uint32_t DEFAUL_RECEIVE_TIMEOUT = DEF_RX_TO, DEFAULT_IDLE_TIMEOUT = DEF_IDLE_TO, SEND_GUARD_TIME = SEND_GUARD, SEND_DELAY_TIME = SEND_DELAY;
		
		#ifdef HAL_UART_TIMEOUT_VALUE
		static constexpr uint32_t DEFAUL_TRANSMIT_TIMEOUT = HAL_UART_TIMEOUT_VALUE;
		#else
		static constexpr uint32_t DEFAUL_TRANSMIT_TIMEOUT = (HAL_MAX_DELAY);
		#endif
		
		enum ErrorCode : int32_t
		{
			OK				= +0,
			INVALID			= -1,
			INVALID_PARAM	= INVALID,
			UART_ERR		= -2,
			TIMEOUT			= -3,
			WRONG_FORMAT	= -4,	// In response
			ERR				= -5,	// Failure on the module side
			FAIL			= -6,	// Failure on the MCU side
			BIG_PARAM		= -7,
			BUF_FULL		= -8,
			ABORT			= -9,	// todo: remove?
			UNKNOWN			= -10,
			
			// GL865
			CM_CODE_100(CME_0, 10), CM_CODE_100(CME_1, 11),
			CM_CODE_100(CME_5, 15), CM_CODE_100(CME_6, 16), CM_CODE_100(CME_7, 17), CM_CODE_100(CME_8, 18), CM_CODE_100(CME_9, 19),
			
			CM_CODE_100(CMS_0, 20), CM_CODE_100(CMS_1, 21), CM_CODE_100(CMS_2, 22), CM_CODE_100(CMS_3, 23),
			CM_CODE(CMS_500, 2500), CM_CODE(CMS_512, 2512),
		};
		
	protected:
		static constexpr STM32T::Log::Logger LG = STM32T::Log::g_defaultLogger.Clone(STM32T::Log::Level::Debug, "GSM"sv);
		
		static constexpr size_t DEFAULT_RESPONSE_LEN = DEF_RESP_LEN, DEFAULT_ARG_LEN = DEF_ARG_LEN;
		
		static constexpr strv ESC = "\x1B"sv, CTRL_Z = "\x1A"sv, CMD_MODE = "+++"sv;
		
		
		enum class CommandType : uint8_t
		{
			Test,		// AT+CMD=?\r
			Read,		// AT#CMD?\r
			Execute,	// AT@CMDARGS\r
			Write,		// AT+CMD=ARGS\r
			Bare		// ARGS
		};
		
		
		uint32_t m_lastSend = 0;
		UART_HandleTypeDef* const p_huart;
		bool m_urcEnabled = false, m_noSendWait = false, m_noSendDelay = false;
		
		
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
		
		void SendUART(strv data)
		{
			HAL_UART_Transmit(p_huart, (uint8_t*)data.data(), data.length(), DEFAUL_TRANSMIT_TIMEOUT);
		}
		
		virtual int32_t ReceiveUART(char *buffer, uint16_t len, const uint32_t timeout, const uint32_t idle_timeout) = 0;
		
		int32_t Command(const uint32_t timeout, const CommandType type, const strv cmd, const strv args, char* buffer, const uint16_t len)
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
				if (m_urcEnabled)
					stopURC();
				
				const int32_t len2 = ReceiveUART(buffer, len - 1, timeout, DEFAULT_IDLE_TIMEOUT);
				
				if (m_urcEnabled)
					startURC();
				
				if (len2 < OK)
					return ErrorCode(len2);
				
				buffer[len2] = 0;		// Make it safe for C str functions
				
				return len2;
			}
			
			return 0;
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
		
		/**
		* @note Calls va_end()
		*/
		std::pair<int, std::unique_ptr<char[]>> FormatArgsDyn(const char *fmt, std::va_list args, size_t arg_len = DEFAULT_ARG_LEN)
		{
			if (!fmt)
				return {INVALID, nullptr};
			
			auto buf = std::make_unique<char[]>(arg_len);
			
			const int len = vsnprintf(buf.get(), arg_len, fmt, args);
			va_end(args);
			
			if (len < 0)
				return {FAIL, nullptr};
			
			if (len >= arg_len)
				return {BIG_PARAM, nullptr};
			
			return {len, std::move(buf)};
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode NoToken(const uint32_t timeout, const CommandType type, const strv cmd, const func<ErrorCode (strv)>& handler, const strv args = strv())
		{
			if (!handler)
				return INVALID_PARAM;
			
			char buffer[LEN];
			int32_t len = Command(timeout, type, cmd, args, buffer, sizeof(buffer));
			if (len < OK)
				return ErrorCode(len);
			
			return handler(strv(buffer, len));
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode NoToken(const uint32_t timeout, const CommandType type, const strv cmd, const func<ErrorCode (strv)>& handler, const char *const fmt, ...)
		{
			if (!fmt)
				return INVALID;
			
			char args[ARG_LEN];
			
			va_list print_args;
			va_start(print_args, fmt);
			const int argsLen = vsnprintf(args, sizeof(args), fmt, print_args);
			va_end(print_args);
			
			if (argsLen < 0)
				return UNKNOWN;
			
			if (argsLen >= sizeof(args))	// did not fit inside {args}
				return BIG_PARAM;
			
			return NoToken<LEN>(timeout, type, cmd, handler, strv(args, argsLen));
		}
		
		template <size_t CHUNK_LEN = 600, size_t LEN = DEFAULT_RESPONSE_LEN>
		int32_t ReceiveOnline(const uint32_t timeout, const uint32_t dl_to, const CommandType type, const strv cmd,
			const func<ErrorCode (strv, size_t)>& chunk_handler = nullptr, const strv args = strv())
		{
			std::unique_ptr<char[]> buf[2] = {std::make_unique<char[]>(CHUNK_LEN), std::make_unique<char[]>(CHUNK_LEN)};
			if (!buf[0] || !buf[1])
				return FAIL;
			
			ErrorCode code = SingleToken<LEN>(timeout, type, cmd, args, {{"CONNECT"sv, OK}, {"NO CARRIER"sv, FAIL}});
			
			if (code != OK)
				return code;
			
			const bool urcEnabled = m_urcEnabled;
			
			ScopeActionF exit([this, urcEnabled]()
			{
				HAL_UART_AbortReceive_IT(p_huart);
				
				Command(0, CommandType::Bare, ""sv, CMD_MODE, nullptr, 0);
				
				if (urcEnabled)
					EnableURC(true);
			});
			
			if (urcEnabled)
				EnableURC(false);
			
			const uint32_t start = HAL_GetTick();
			uint32_t rem = dl_to;
			size_t len = 0;
			ClampedInt<uint8_t, 0, std::size(buf) - 1> index = 0;
			
			HAL_UART_StateTypeDef state = HAL_UART_STATE_READY;
			HAL_StatusTypeDef stat = HAL_UARTEx_ReceiveToIdle_IT(p_huart, reinterpret_cast<uint8_t *>(buf[index].get()), CHUNK_LEN);
			
			while (stat == HAL_OK && code == OK && state == HAL_UART_STATE_READY && rem)
			{
				do
				{
					#ifdef STM32T_IWDG_TIMEOUT
					HAL_IWDG_Refresh(&hiwdg);
					#endif	// STM32T_IWDG_TIMEOUT
					
					state = HAL_UART_GetState(p_huart);
					rem = Time::Remaining_Tick(start, dl_to);
				}
				while (state == HAL_UART_STATE_BUSY_RX && rem);
				
				strv chunk = {buf[index].get(), size_t(p_huart->RxXferSize - p_huart->RxXferCount)};
				if (chunk == "\r\nNO CARRIER\r\n"sv)
					return len;		// end
				
				if (state == HAL_UART_STATE_READY && rem)
					stat = HAL_UARTEx_ReceiveToIdle_IT(p_huart, reinterpret_cast<uint8_t *>(buf[++index].get()), CHUNK_LEN);
				
				if (chunk_handler)
					code = chunk_handler(chunk, len);
				
				len += chunk.size();
			}
			
			if ((state != HAL_UART_STATE_READY || stat != HAL_OK) && !len)
				return UART_ERR;
			
			if (code != OK && !len)
				return code;
			
			if (!rem && !len)
				return TIMEOUT;
			
			return len;
		}
		
		template <size_t CHUNK_LEN = 600, size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		int32_t ReceiveOnline(const uint32_t timeout, const uint32_t dl_to, const CommandType type, const strv cmd,
			const func<ErrorCode (strv, size_t)>& chunk_handler, const char *const fmt, ...)
		{
			if (!fmt)
				return INVALID;
			
			char args[ARG_LEN];
			
			va_list print_args;
			va_start(print_args, fmt);
			const int argsLen = vsnprintf(args, sizeof(args), fmt, print_args);
			va_end(print_args);
			
			if (argsLen < 0)
				return UNKNOWN;
			
			if (argsLen >= sizeof(args))	// did not fit inside {args}
				return BIG_PARAM;
			
			return ReceiveOnline<CHUNK_LEN, LEN>(timeout, dl_to, type, cmd, chunk_handler, strv(args, argsLen));
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode Tokens2(const uint32_t timeout, const CommandType type, const strv cmd, const strv args, const func<ErrorCode (vec<strv>&)>& op,
			const bool allowSingleEnded = false)
		{
			if (!op)
				return INVALID_PARAM;
			
			char buffer[LEN];
			int32_t len = Command(timeout, type, cmd, args, buffer, sizeof(buffer));
			if (len < OK)
				return ErrorCode(len);
			
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
			int32_t len = Command(timeout, type, cmd, args, buffer, sizeof(buffer));
			if (len < OK)
				return ErrorCode(len);
			
			vec<strv> tokens;
			strv(buffer, len).tokenize2("\r\n"sv, tokens, !allowSingleEnded);
			
			if (tokens.size() == 0 || tokens.back() != "OK"sv)
				return Error(tokens);
			
			tokens.pop_back();
			
			return op(tokens);
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode SingleToken(const uint32_t timeout, const CommandType type, const strv cmd, const strv args = strv(),
			const span<const std::pair<strv, ErrorCode>> responses = {{"OK"sv, OK}}, const bool allowSingleEnded = false)
		{
			return Tokens2<LEN>(timeout, type, cmd, args, [&](vec<strv>& tokens)
			{
				if (tokens.size() == 1)
				{
					for (auto resp : responses)
					{
						if (tokens[0] == resp.first)
							return resp.second;
					}
				}
				
				return Error(tokens);
			}, allowSingleEnded);
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode SingleToken(const uint32_t timeout, const CommandType type, const strv cmd, const span<const std::pair<strv, ErrorCode>> responses,
			const bool allowSingleEnded, const char* const fmt, ...)
		{
			if (!fmt)
				return INVALID;
			
			char args[ARG_LEN];
			
			va_list print_args;
			va_start(print_args, fmt);
			const int argsLen = vsnprintf(args, sizeof(args), fmt, print_args);
			va_end(print_args);
			
			if (argsLen < 0)
				return ErrorCode::UNKNOWN;
			
			if (argsLen >= sizeof(args))
				return BIG_PARAM;
			
			return SingleToken<LEN>(timeout, type, cmd, strv(args, argsLen), responses, allowSingleEnded);
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode ReceiveOK(const uint32_t timeout, const CommandType type, const strv cmd, const char* const fmt, ...)
		{
			if (!fmt)
				return INVALID;
			
			char args[ARG_LEN];
			
			va_list print_args;
			va_start(print_args, fmt);
			const int argsLen = vsnprintf(args, sizeof(args), fmt, print_args);
			va_end(print_args);
			
			if (argsLen < 0)
				return ErrorCode::UNKNOWN;
			
			if (argsLen >= sizeof(args))
				return BIG_PARAM;
			
			return SingleToken<LEN>(timeout, type, cmd, strv(args, argsLen));
		}
		
		ErrorCode WaitForReady(const uint32_t timeout, const CommandType type, const strv cmd, const strv args = strv())
		{
			return SingleToken(timeout, type, cmd, args, {{"> "sv, OK}}, true);
		}
		
		ErrorCode WaitForReady(const uint32_t timeout, const CommandType type, const strv cmd, const char *fmt, ...)
		{
			va_list args;
			va_start(args, fmt);
			auto [len, buf] = FormatArgsDyn(fmt, args);
			
			if (len < OK)
				return ErrorCode(len);
			
			return WaitForReady(timeout, type, cmd, {buf.get(), size_t(len)});
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
			const int argsLen = vsnprintf(args, sizeof(args), fmt, print_args);
			va_end(print_args);
			
			if (argsLen < 0)
				return INVALID_PARAM;
			
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
			const int argsLen = vsnprintf(args, sizeof(args), fmt, print_args);
			va_end(print_args);
			
			if (argsLen < 0)
				return INVALID_PARAM;
			
			if (argsLen >= sizeof(args))
				return BIG_PARAM;
			
			return ResponseToken<LEN>(timeout, type, cmd, strv(args, argsLen), op, ok_last);
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode DelayedResponseToken(const uint32_t timeout, const CommandType type, const strv cmd, const strv args, const func<ErrorCode (strv)>& op)
		{
			bool done = false;
			const uint32_t start = HAL_GetTick();	// todo: handle send guard time
			
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
			const int argsLen = vsnprintf(args, sizeof(args), fmt, print_args);
			va_end(print_args);
			
			if (argsLen < 0)
				return INVALID_PARAM;
			
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
		int32_t StrToken2(char * const buf, size_t max_len, const CommandType type, const strv cmd, const strv args = strv())
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
				static char fmt[32] = { 0 };
				
				const int mins = zz * 15, hour = mins / 60, min = mins % 60;
				
				sprintf(fmt, "%hhu%02hhu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu%+03hhd:%02hhu", yy < 70 ? 20u : 19u, yy, MM, dd, hh, mm, ss, hour, std::abs(min));
				return fmt;
			}
		};
		
		GSM(UART_HandleTypeDef* uart) : p_huart(uart) {}
		
		ErrorCode AT(const uint32_t timeout = DEFAUL_RECEIVE_TIMEOUT)
		{
			return SingleToken(timeout, CommandType::Execute, {});
		}
		
		ErrorCode Custom(strv command, const uint32_t timeout = DEFAUL_RECEIVE_TIMEOUT)
		{
			return SingleToken(timeout, CommandType::Execute, command);
		}
		
		int32_t GetBrand(char *const buf, const size_t max_len)
		{
			return StrToken2(buf, max_len, CommandType::Execute, "+CGMI"sv);
		}
		
		int32_t GetModel(char *const buf, const size_t max_len)
		{
			return StrToken2(buf, max_len, CommandType::Execute, "+CGMM"sv);
		}
		
		/**
		* @param rev - Will be null-terminated.
		*/
		int32_t GetRevision(char *const rev, const size_t max_len)
		{
			return StrToken2(rev, max_len, CommandType::Execute, "+CGMR"sv);
		}
		
		/**
		* @param imei - Must have at least IMEI_LEN + 1 bytes. It will have exactly IMEI_LEN characters and will be null-terminated on success.
		*/
		ErrorCode GetIMEI(char *const imei)
		{
			static_assert(IMEI_LEN + 4 + 6 < DEFAULT_RESPONSE_LEN);
			
			int32_t len = StrToken2(imei, IMEI_LEN + 1, CommandType::Execute, "+CGSN"sv);
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
			
			const int32_t len = StrToken2(imsi, IMSI_LEN + 1, CommandType::Execute, "+CIMI"sv);
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
		
		#if defined(STM32T_GSM_URC_SUPPORT) && USE_HAL_UART_REGISTER_CALLBACKS == 1
	private:
		class URC
		{
			friend class GSM<DEF_RX_TO, DEF_IDLE_TO, SEND_GUARD, SEND_DELAY, DEF_RESP_LEN, DEF_ARG_LEN>;
			friend class LinkedList<URC, 64>;
			
			char *m_buf;
			size_t m_size;
			uint32_t m_timestamp;
			
			URC(const URC& other) = delete;
			URC(URC&& other) : m_buf(other.m_buf), m_size(other.m_size), m_timestamp(other.m_timestamp)
			{
				other.m_buf = nullptr;
				other.m_size = 0;
			}
			
			~URC()
			{
				delete[] m_buf;
				m_buf = nullptr;
				m_size = 0;
			}
			
			URC(const strv& urc) : URC(urc, HAL_GetTick()) {}
			URC(const strv& urc, const uint32_t timestamp) : m_buf(new char[urc.size() + 1]), m_size(urc.size()), m_timestamp(timestamp)
			{
				memcpy(m_buf, urc.data(), m_size);
				m_buf[m_size] = 0;
			}
			
			operator strv() const { return {m_buf, m_size}; }
		};
		
		uint8_t m_buf[512];
		LinkedList<URC, 64> m_urcs;
		
		static inline This *s_this = nullptr;
		
		void startURC()
		{
			__HAL_UART_CLEAR_OREFLAG(p_huart);
			
			HAL_UARTEx_ReceiveToIdle_DMA(p_huart, m_buf, sizeof(m_buf));
			__HAL_DMA_DISABLE_IT(p_huart->hdmarx, DMA_IT_HT);	//Disable half transfer interrupt if it is enabled in HAL_UARTEx_ReceiveToIdle_DMA()
		}
		
		void stopURC() { HAL_UART_DMAStop(p_huart); }	// todo: handle half received URC
		
	protected:
		void addURC(const strv token)
		{
			m_urcs.push_back(URC{token});
		}
		
	public:
		void EnableURC(bool enable = true)
		{
			if (enable)
			{
				if (m_urcEnabled)
					return;
				
				s_this = this;
				
				HAL_UART_RegisterRxEventCallback(p_huart, [](UART_HandleTypeDef *huart, uint16_t size)
				{
					if (!s_this)	// todo: find a better way
						return;
					
					const Time::cycle_t start = Time::GetCycle();
					
					strv data = {reinterpret_cast<const char *>(huart->pRxBuffPtr), size};
					data.tokenize2("\r\n"sv, [](const strv token) { s_this->addURC(token); }, false);
					
					const Time::cycle_t end = Time::GetCycle();
					const auto time = Time::CyclesTo_us(end - start);
					
					static const Time::us_time_t MaxTime = 1'000'000u * 10u / huart->Init.BaudRate;		// Time of 1 byte
					
					if (time >= MaxTime)	// todo: Can't use LG directly (LOG_W<LG>)
						LG.w("Rx event proccessing time (%u us) has exceeded maximum (%u us).", time, MaxTime);
					
					s_this->startURC();
					
				});
				
				startURC();
				m_urcEnabled = true;
			}
			else
			{
				if (!m_urcEnabled)
					return;
				
				m_urcEnabled = false;
				s_this = nullptr;
				// todo: clear m_urcs?
				
				stopURC();
				HAL_UART_UnRegisterRxEventCallback(p_huart);
			}
		}
		
		/**
		* @param handler - If it returns true, the urc will be removed and considered handled.
		* @retval - The nummber of urcs handled.
		*/
		size_t HandleURCs(const func<bool (strv, uint32_t ts)>& handler, const bool stop_when_handled = false)
		{
			if (!handler)
				return 0;
			
			size_t handled = 0;
			auto it = m_urcs.begin();
			while (it != m_urcs.end())
			{
				if (handler(*it, it->m_timestamp))
				{
					it = m_urcs.erase(it);
					++handled;
					
					if (stop_when_handled)
						return handled;
				}
				else
					++it;
			}
			
			return handled;
		}
		#else
	protected:
		void addURC(const strv token) {}
		void startURC() {}
		void stopURC() {}
		
	public:
		void EnableURC(bool enable = true) {}
		#endif	// USE_HAL_UART_REGISTER_CALLBACKS == 1
	};
}
#endif	// HAL_UART_MODULE_ENABLED
