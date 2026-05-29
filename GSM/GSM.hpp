#pragma once

#include "main.h"

#ifdef HAL_UART_MODULE_ENABLED

#include "../Core/Utils.hpp"
#include "../Core/strv.hpp"
#include "../Core/span.hpp"
#include "../Log.hpp"

#include <memory>	// unique_ptr
#include <optional>
#include <variant>



#if defined(STM32T_GSM_URC_SUPPORT) && USE_HAL_UART_REGISTER_CALLBACKS == 1

#define STM32T_GSM_URC_ENABLED

#ifndef STM32T_GSM_URC_BUF_SIZE
#define STM32T_GSM_URC_BUF_SIZE		512
#endif	// STM32T_GSM_URC_BUF_SIZE

#endif	// defined(STM32T_GSM_URC_SUPPORT) && USE_HAL_UART_REGISTER_CALLBACKS == 1



#define CM_CODE(name, code)		name = -(code)

#define CM_CODE_10(name, code)		CM_CODE(name##0, (code) * 10 + 0), CM_CODE(name##1, (code) * 10 + 1), \
CM_CODE(name##2, (code) * 10 + 2), CM_CODE(name##3, (code) * 10 + 3), CM_CODE(name##4, (code) * 10 + 4), CM_CODE(name##5, (code) * 10 + 5), \
CM_CODE(name##6, (code) * 10 + 6), CM_CODE(name##7, (code) * 10 + 7), CM_CODE(name##8, (code) * 10 + 8), CM_CODE(name##9, (code) * 10 + 9)

#define CM_CODE_100(name, code)		CM_CODE_10(name##0, (code) * 10 + 0), CM_CODE_10(name##1, (code) * 10 + 1), \
CM_CODE_10(name##2, (code) * 10 + 2), CM_CODE_10(name##3, (code) * 10 + 3), CM_CODE_10(name##4, (code) * 10 + 4), CM_CODE_10(name##5, (code) * 10 + 5), \
CM_CODE_10(name##6, (code) * 10 + 6), CM_CODE_10(name##7, (code) * 10 + 7), CM_CODE_10(name##8, (code) * 10 + 8), CM_CODE_10(name##9, (code) * 10 + 9)



#define _FORMAT_ARGS()	\
	if (!fmt) \
		return FAIL; \
	\
	char args[ARG_LEN]; \
	\
	va_list print_args; \
	va_start(print_args, fmt); \
	const ErrorCode argsLen = (ErrorCode)FormatArgs(args, sizeof(args), fmt, print_args); \
	\
	if (argsLen < OK) \
		return argsLen
	

#define _GET_ARGS(var_name) \
strv args; \
char _args_buf[ARG_LEN]; \
\
if (const strv *p = std::get_if<const strv>(&var_name)) \
	args = *p; \
else \
{ \
	const char *const *fmt = std::get_if<const char *>(&var_name); \
	if (!fmt) \
		return FAIL; \
	\
	va_list print_args; \
	va_start(print_args, var_name); \
	const ErrorCode argsLen = (ErrorCode)FormatArgs(_args_buf, sizeof(_args_buf), *fmt, print_args); \
	\
	if (argsLen < OK) \
		return argsLen; \
	\
	args = {_args_buf, size_t(argsLen)}; \
}



namespace STM32T
{
	template <uint32_t DEF_RX_TO = 300, uint32_t DEF_IDLE_TO = 20, uint32_t DEF_O2C_TO = 1000>
	class GSM
	{
	public:
		static constexpr size_t IMEI_LEN = 15, IMSI_LEN = 15, DEFAULT_RESPONSE_LEN = 64, DEFAULT_ARG_LEN = 64, RESPONSE_EXTRA = 12;		// \r\n+CME: xxx\r\n
		static constexpr uint32_t
			DEFAUL_TRANSMIT_TIMEOUT			= 10'000,
			DEFAUL_RECEIVE_TIMEOUT			= DEF_RX_TO,
			DEFAULT_IDLE_TIMEOUT			= DEF_IDLE_TO,
			DEFAULT_ONLINE_TO_CMD_TIMEOUT	= DEF_O2C_TO;
		
		enum ErrorCode : int32_t
		{
			OK				= +0,
			NOT_ALLOWED		= -1,
			INVALID			= -2,
			INVALID_PARAM	= INVALID,	// todo: remove?
			BIG_PARAM		= -3,
			TIMEOUT			= -4,
			WRONG_FORMAT	= -5,	// In response
			ERR				= -6,	// Failure on the module side
			FAIL			= -7,	// Failure on the MCU side or in library
			BUF_FULL		= -8,	// User's buffer
			UNKNOWN			= -9,
			
			// GL865
			CM_CODE_100(CME_0, 10), CM_CODE_100(CME_1, 11),
			CM_CODE_100(CME_5, 15), CM_CODE_100(CME_6, 16), CM_CODE_100(CME_7, 17), CM_CODE_100(CME_8, 18), CM_CODE_100(CME_9, 19),
			
			CM_CODE_100(CMS_0, 20), CM_CODE_100(CMS_1, 21), CM_CODE_100(CMS_2, 22), CM_CODE_100(CMS_3, 23),
			CM_CODE(CMS_500, 2500), CM_CODE(CMS_512, 2512),
		};
		
	protected:
		static constexpr STM32T::Log::Logger LG = STM32T::Log::g_defaultLogger.Clone(STM32T::Log::Level::Debug, "GSM"sv);
		
		static constexpr strv ESC = "\x1B"sv, CTRL_Z = "\x1A"sv, CMD_MODE = "+++"sv;
		
		enum class CommandType : uint8_t
		{
			Test,		// AT+CMD=?\r
			Read,		// AT#CMD?\r
			Execute,	// AT@CMDARGS\r
			Write,		// AT+CMD=ARGS\r
			Bare		// ARGS
		};
		
		UART_HandleTypeDef* const p_huart;
		bool m_urcEnabled = false;
		
		void addURCs(const vec<strv>& tokens, size_t len = SIZE_MAX)
		{
			for (size_t i = 0; i < tokens.size() && i < len; ++i)
				addURC(tokens[i]);
		}
		
		void addURCFromBuf(const strv buf)
		{
			vec<strv> tokens;
			buf.tokenize("\r\n"sv, tokens, true);
			addURCs(tokens);
		}
		
		void SendUART(strv data)
		{
			HAL_UART_Transmit(p_huart, reinterpret_cast<const uint8_t *>(data.data()), data.length(), DEFAUL_TRANSMIT_TIMEOUT);
		}
		
		void SendUART(const char ch)
		{
			HAL_UART_Transmit(p_huart, reinterpret_cast<const uint8_t *>(&ch), 1, DEFAUL_TRANSMIT_TIMEOUT);
		}
		
		void SendHEX(strv data)
		{
			for (auto ch : data)
			{
				SendUART(STM32T::H2C(ch >> 4));
				SendUART(STM32T::H2C(ch));
			}
		}
		
		[[deprecated]]
		void SendUCS2(u16strv data)
		{
			for (auto ch : data)
			{
				SendUART(STM32T::H2C(ch >> 12));
				SendUART(STM32T::H2C(ch >> 8));
				SendUART(STM32T::H2C(ch >> 4));
				SendUART(STM32T::H2C(ch));
			}
		}
		
		void SendUCS2(strv data)
		{
			for (auto ch : data)
			{
				SendUART("00"sv);
				SendUART(STM32T::H2C(ch >> 4));
				SendUART(STM32T::H2C(ch));
			}
		}
		
		void SendUCS2(wstrv data)
		{
			static_assert(sizeof(wstrv::value_type) == 2);
			
			for (auto ch : data)
			{
				SendUART(STM32T::H2C(ch >> 12));
				SendUART(STM32T::H2C(ch >> 8));
				SendUART(STM32T::H2C(ch >> 4));
				SendUART(STM32T::H2C(ch));
			}
		}
		
		int32_t ReceiveUART(char *buffer, uint16_t len, const uint32_t timeout, const uint32_t idle_timeout)
		{
			const auto start = HAL_GetTick();
			
			__HAL_UART_CLEAR_OREFLAG(p_huart);
			HAL_StatusTypeDef stat = HAL_TIMEOUT;
			
			#ifdef STM32T_IWDG_TIMEOUT
			while (1)
			{
				HAL_IWDG_Refresh(&Time::hiwdg);
				
				if (stat == HAL_OK)
					goto ok;
				else if (stat != HAL_TIMEOUT)
					break;
				
				const uint32_t t = std::min(STM32T::Time::Remaining_Tick(start, timeout), (STM32T_IWDG_TIMEOUT));
				if (!t)
					break;
				
				stat = HAL_UART_Receive(p_huart, (uint8_t *)buffer, 1, t);
			}
			#else
			stat = HAL_UART_Receive(p_huart, (uint8_t *)buffer, 1, timeout);
			if (stat == HAL_OK)
				goto ok;
			#endif	// STM32T_IWDG_TIMEOUT
			
			return stat == HAL_TIMEOUT ? TIMEOUT : FAIL;
			
		ok:
			const uint16_t orig_len = len;
			
			for (uint16_t i = 1; i < len; i++)
			{
				if (HAL_GetTick() - start > timeout)
					return i;
				
				stat = HAL_UART_Receive(p_huart, (uint8_t *)&buffer[i], 1, idle_timeout);
				if (stat != HAL_OK)
					return i;
			}
			
			return orig_len;
		}
		
		int32_t Command(const uint32_t timeout, const CommandType type, const strv cmd, const strv args, char* buffer, const uint16_t len)
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
		* @note Calls va_end().
		*/
		int32_t FormatArgs(char *buf, size_t buf_len, const char *fmt, std::va_list args)
		{
			if (!fmt)
				return FAIL;
			
			const int len = vsnprintf(buf, buf_len, fmt, args);
			va_end(args);
			
			if (len < 0)
				return FAIL;
			
			if (len >= buf_len)
				return BIG_PARAM;
			
			return len;
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode NoToken(const uint32_t timeout, const CommandType type, const strv cmd, const func<ErrorCode (strv)>& handler,
			const std::variant<const char *, const strv> fmt_args = ""sv, ...)
		{
			_GET_ARGS(fmt_args);
			
			if (!handler)
				return INVALID_PARAM;
			
			char buffer[LEN];
			int32_t len = Command(timeout, type, cmd, args, buffer, sizeof(buffer));
			if (len < OK)
				return ErrorCode(len);
			
			return handler(strv(buffer, len));
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode EnterOnline(const uint32_t timeout, const CommandType type, const strv cmd, const std::variant<const char *, const strv> fmt_args = ""sv, ...)
		{
			_GET_ARGS(fmt_args);
			
			ErrorCode code = SingleToken<LEN>(timeout, type, cmd, args, {{"CONNECT"sv, OK}, {"NO CARRIER"sv, FAIL}}, false);
			
			if (code != OK)
				ExitOnline();
			
			return code;
		}
		
		ErrorCode ExitOnline(const uint32_t timeout = DEFAULT_ONLINE_TO_CMD_TIMEOUT)
		{
			HAL_Delay(timeout);	// Set with S12 - todo: check the last tranmisson/reception (CONNECT) time
			return SingleToken(timeout, CommandType::Bare, strv(), CMD_MODE, {{"NO CARRIER"sv, OK}}, false);
		}
		
		template <size_t CHUNK_LEN = 600, size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		int32_t ReceiveOnline(const uint32_t timeout, const uint32_t dl_to, const CommandType type, const strv cmd,
			const func<ErrorCode (strv, size_t)>& chunk_handler, const std::variant<const char *, const strv> fmt_args = ""sv, ...)
		{
			_GET_ARGS(fmt_args);
			
			std::unique_ptr<char[]> buf[2] = {std::make_unique<char[]>(CHUNK_LEN), std::make_unique<char[]>(CHUNK_LEN)};
			if (!buf[0] || !buf[1])
				return FAIL;
			
			ErrorCode code = EnterOnline<DEFAULT_ARG_LEN, LEN>(timeout, type, cmd, args);
			
			if (code != OK)
				return code;
			
			const bool urcEnabled = m_urcEnabled;
			
			ScopeActionF exit([this, urcEnabled]()
			{
				HAL_UART_AbortReceive_IT(p_huart);
				ExitOnline();
				
				if (urcEnabled)
					EnableURC(true);	// Can't use stopURC() and startURC() because the EventCallback is bound and we're using HAL_UARTEx_ReceiveToIdle_IT().
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
					HAL_IWDG_Refresh(&Time::hiwdg);
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
				return FAIL;
			
			if (code != OK && !len)
				return code;
			
			if (!rem && !len)
				return TIMEOUT;
			
			return len;
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
			strv(buffer, len).tokenize("\r\n"sv, tokens, !allowSingleEnded);
			
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
			strv(buffer, len).tokenize("\r\n"sv, tokens, !allowSingleEnded);
			
			if (tokens.size() == 0 || tokens.back() != "OK"sv)
				return Error(tokens);
			
			tokens.pop_back();
			
			return op(tokens);
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode SingleToken(const uint32_t timeout, const CommandType type, const strv cmd, const strv args,
			const span<const std::pair<strv, ErrorCode>> responses, const bool allowSingleEnded)
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
			_FORMAT_ARGS();
			return SingleToken<LEN>(timeout, type, cmd, strv(args, argsLen), responses, allowSingleEnded);
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode ReceiveOK(const uint32_t timeout, const CommandType type, const strv cmd, const std::variant<const char*, const strv> fmt_args = ""sv, ...)
		{
			_GET_ARGS(fmt_args);
			return SingleToken<LEN>(timeout, type, cmd, args, {{"OK"sv, OK}}, false);
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN>
		ErrorCode WaitForReady(const uint32_t timeout, const CommandType type, const strv cmd, const std::variant<const char *, const strv> fmt_args = ""sv, ...)
		{
			_GET_ARGS(fmt_args);
			return SingleToken(timeout, type, cmd, args, {{"> "sv, OK}}, true);
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode ResponseToken(const uint32_t timeout, const CommandType type, const strv cmd,
			const func<ErrorCode (vec<strv>&)>& op, const size_t ok_pos = 1, size_t expectedTokens = 2, const std::variant<const char *, const strv> fmt_args = ""sv, ...)
		{
			if (!op)
				return INVALID_PARAM;
			
			_GET_ARGS(fmt_args);
			
			return Tokens2<LEN + RESPONSE_EXTRA>(timeout, type, cmd, args, [ok_pos, cmd, this, &expectedTokens, &op](vec<strv>& tokens) mutable -> ErrorCode
			{
				bool ok = tokens.size() >= expectedTokens;
				size_t offset = 0;
				
				if (ok && ok_pos < expectedTokens)
				{
					--expectedTokens;
					ok = false;
					for (; offset + ok_pos < tokens.size(); ++offset)
					{
						if (tokens[offset + ok_pos] == "OK"sv)
						{
							tokens.erase(tokens.begin() + offset + ok_pos);
							ok = true;
							break;
						}
					}
				}
				
				if (ok)
				{
					const auto first = tokens[offset];
					ok &= tokens[offset].remove_prefix(cmd) && (tokens[offset].remove_prefix(": "sv) || tokens[offset].remove_prefix(":"sv));
					if (!ok)
						tokens[offset] = first;
				}
				
				if (!ok)
					return Error(tokens);
				
				addURCs(tokens, offset);
				for (size_t i = offset + expectedTokens; i < tokens.size(); ++i)
					addURC(tokens[i]);
				
				tokens.erase(tokens.begin(), tokens.begin() + offset);
				tokens.erase(tokens.begin() + expectedTokens, tokens.end());
				
				return op(tokens);
			});
		}
		
		template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode DelayedResponseToken(const uint32_t timeout, const CommandType type, const strv cmd, const func<ErrorCode (strv)>& op,
			const std::variant<const char *, const strv> fmt_args = ""sv, ...)
		{
			_GET_ARGS(fmt_args);
			
			bool done = false;
			const uint32_t start = HAL_GetTick();
			
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
			
			return ResponseToken<ARG_LEN, LEN>(Time::Remaining_Tick(start, timeout), CommandType::Bare, cmd,
				[&](vec<strv>& tokens) { return op(tokens[0]); }, SIZE_MAX, 1);
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		int32_t StrToken(char * const buf, size_t max_len, const CommandType type, const strv cmd, const strv args = strv())
		{
			return NoToken<DEFAULT_ARG_LEN, LEN>(DEFAUL_RECEIVE_TIMEOUT, type, cmd, [&](strv str) -> ErrorCode
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
		
		
		
		virtual ErrorCode Setup(const uint32_t timeout_ms = 1000)
		{
			static constexpr strv CMD = "E0;+CMEE=1;+CMGF=1;+CSCS=\"UCS2\";"
				"+CSMP=49,167,0,8;"sv;
				//"+CSAS;&W"sv;
			
			static_assert(CMD.size() + 9 < DEFAULT_RESPONSE_LEN);	// Echo might be enabled
			
			return ReceiveOK(timeout_ms, CommandType::Execute, CMD);
		}
		
	public:
		struct DateTime
		{
			uint16_t yyyy;
			uint8_t MM, dd, hh, mm, ss;
			int8_t zz;
			
			static std::optional<DateTime> Parse(strv view)
			{
				DateTime dt;
				
				size_t s1 = 0, s2 = 0;
				
				int n = sscanf(view.data(), "\"%4hu/%2hhu/%2hhu%*1c%2hhu:%2hhu:%2hhu%zn%3hhd%zn", &dt.yyyy, &dt.MM, &dt.dd, &dt.hh, &dt.mm, &dt.ss, &s1, &dt.zz, &s2);
				if (n >= 6 && s1 > 0)
				{
					if (s1 == 18 && dt.yyyy < 100)
						dt.yyyy = (dt.yyyy < 70 ? 2000 : 1900) + dt.yyyy;
					else if (s1 != 20)
						return std::nullopt;
					
					view.remove_prefix(std::max(s1, s2));
					if (!view.starts_with('"'))
						return std::nullopt;
					
					if (n != 7)
						dt.zz = 0;
					
					if (dt.IsSet())
						return dt;
				}
				
				return std::nullopt;
			}
			
			/*static std::optional<DateTime> ParseNTP(strv view)
			{
				DateTime dt;
				
				if (int n; sscanf(view.data(), "%2hhu/%2hhu/%2hhu,%2hhu:%2hhu:%2hhu%n", &dt.yyyy, &dt.MM, &dt.dd, &dt.hh, &dt.mm, &dt.ss, &n) == 6 && n > 0)
				{
					dt.yyyy = (dt.yyyy < 70 ? 2000 : 1900) + dt.yyyy;
					
					view.remove_prefix(size_t(n));
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
			}*/
			
			static std::optional<DateTime> ParseSMS(strv view)
			{
				DateTime dt;
				
				size_t s = 0;
				
				if (7 == sscanf(view.data(), "\"%4hhu/%2hhu/%2hhu%*1c%2hhu:%2hhu:%2hhu%3hhd\"%zn", &dt.yyyy, &dt.MM, &dt.dd, &dt.hh, &dt.mm, &dt.ss, &dt.zz, &s) && s > 0)
				{
					if (s == 22 && dt.yyyy < 100)
						dt.yyyy = (dt.yyyy < 70 ? 2000 : 1900) + dt.yyyy;
					else if (s != 24)
						return std::nullopt;
					
					if (dt.IsSet())
						return dt;
				}
				
				return std::nullopt;
			}
			
			bool IsSet() const
			{
				return MM >= 1 && MM <= 12 && dd >= 1 && dd <= Time::MonthDays(MM - 1, yyyy) && hh <= 23 && mm <= 59 && ss <= 60 && zz >= -47 && zz <= 48;
			}
			
			const char* Format() const
			{
				static char fmt[32] = { 0 };
				
				const int16_t mins = zz * 15;
				const int8_t hour = mins / 60, min = mins % 60;
				
				sprintf(fmt, "%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu%+03hhd:%02hhu", yyyy, MM, dd, hh, mm, ss, hour, std::abs(min));
				return fmt;
			}
		};
		
		GSM(UART_HandleTypeDef* huart) : p_huart(huart) {}
		
		ErrorCode AT(const uint32_t timeout = DEFAUL_RECEIVE_TIMEOUT)
		{
			return ReceiveOK(timeout, CommandType::Execute, {}, "");
		}
		
		template <size_t LEN = DEFAULT_RESPONSE_LEN>
		ErrorCode Custom(strv command, const uint32_t timeout = DEFAUL_RECEIVE_TIMEOUT)
		{
			return ReceiveOK<DEFAULT_ARG_LEN, LEN>(timeout, CommandType::Execute, command);
		}
		
		
		// ****************************** V.25TER / Hayes *****************************
		
		ErrorCode Call(strv number, const uint32_t timeout = 30'000)	// GL865: 30s, SIM800: 20s
		{
			using STM32T::Log::IsEnabled;
			using STM32T::Log::LOG_D;
			using STM32T::Log::LOG_W;
			
			if constexpr (IsEnabled<LG>(Log::Level::Debug))
			{
				std::string hidden(std::min(number.size(), number.size() - 4), 'x');
				
				if (!hidden.empty())
					LOG_D<LG>("Calling %s%.*s...", hidden.data(), number.size() >= 4 ? number.size() - 4 : 0, number.data() + number.size() - 4);
				else
					LOG_D<LG>("Calling...");
			}
			
			return SingleToken(timeout, CommandType::Execute, "D"sv, {{"OK"sv, OK}, {"NO CARRIER"sv, FAIL}}, false, "%.*s;", number.size(), number.data());
		}
		
		ErrorCode FactoryReset()
		{
			return ReceiveOK(DEFAUL_RECEIVE_TIMEOUT, CommandType::Execute, "&F"sv);
		}
		
		
		// ****************************** 3GPP TS 27.007 ******************************
		
		int32_t GetBrand(char *const buf, const size_t max_len)
		{
			return StrToken(buf, max_len, CommandType::Execute, "+CGMI"sv);
		}
		
		int32_t GetModel(char *const buf, const size_t max_len)
		{
			return StrToken(buf, max_len, CommandType::Execute, "+CGMM"sv);
		}
		
		/**
		* @param rev - Will be null-terminated.
		*/
		int32_t GetRevision(char *const rev, const size_t max_len)
		{
			return StrToken(rev, max_len, CommandType::Execute, "+CGMR"sv);
		}
		
		/**
		* @param imei - Must have at least IMEI_LEN + 1 bytes. It will have exactly IMEI_LEN characters and will be null-terminated on success.
		*/
		ErrorCode GetIMEI(char *const imei)
		{
			static_assert(IMEI_LEN + 4 + 6 < DEFAULT_RESPONSE_LEN);
			
			int32_t len = StrToken(imei, IMEI_LEN + 1, CommandType::Execute, "+CGSN"sv);
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
			
			return strv(imei_s, IMEI_LEN).to_num(imei) == IMEI_LEN ? OK : WRONG_FORMAT;
		}
		
		/**
		* @param imsi - Must have at least IMSI_LEN + 1 bytes. It will have exactly IMSI_LEN characters and will be null-terminated on success.
		*/
		ErrorCode GetIMSI(char *const imsi)
		{
			static_assert(IMSI_LEN + 4 + 6 < DEFAULT_RESPONSE_LEN);
			
			const int32_t len = StrToken(imsi, IMSI_LEN + 1, CommandType::Execute, "+CIMI"sv);
			return len == IMSI_LEN ? OK : WRONG_FORMAT;
		}
		
		ErrorCode GetIMSI(uint16_t &mcc, uint8_t &mnc)
		{
			char imsi[IMSI_LEN + 1];
			
			const ErrorCode code = GetIMSI(imsi);
			if (code != OK)
				return code;
			
			strv imsi_sv(imsi, IMSI_LEN);
			if (imsi_sv.to_num(mcc, 0, 3) != 3 || imsi_sv.to_num(mnc, 3, 2) != 2)
				return WRONG_FORMAT;
			
			return OK;
		}
		
		/**
		* @param rssi - Signal strength in dbm. Not available if positive or zero.
		* @param ber - Bit error rate (average) per ten thousand. Not available if negative.
		*/
		ErrorCode GetSignalQuality(int8_t& rssi, int16_t& ber)
		{
			return ResponseToken(DEFAUL_RECEIVE_TIMEOUT, CommandType::Execute, "+CSQ"sv, [&rssi, &ber](const std::vector<strv>& tokens) -> ErrorCode
			{
				uint8_t t1, t2;
				if (2 != sscanf(tokens[0].data(), "%2hhu,%2hhu", &t1, &t2) || (t1 > 31 && t1 != 99) || (t2 > 7 && t2 != 99))
					return WRONG_FORMAT;
				
				if (t1 <= 31)
					rssi = -113 + t1 * 2;
				else
					rssi = 0;
				
				if (t2 <= 7)
				{
					static constexpr int16_t AVG[8] = {14, 28, 57, 113, 226, 453, 905, 1810};
					
					ber = AVG[t2];
				}
				else
					ber = -1;
				
				return OK;
			});
		}
		
		
		// ****************************** 3GPP TS 27.005 ******************************
		
		[[deprecated]]
		ErrorCode SMSend(u16strv number, const STM32T::span<const u16strv> msgs, const uint32_t timeout = 60'000)
		{
			using STM32T::Log::IsEnabled;
			using STM32T::Log::LOG_D;
			using STM32T::Log::LOG_W;
			
			if constexpr (IsEnabled<LG>(Log::Level::Debug))
			{
				const auto end_opt = number.size() >= 4 ? U16ToU8(number.substr(number.size() - 4)) : std::string();
				std::string hidden(std::min(number.size(), number.size() - 4), 'x');
				
				if (end_opt && !hidden.empty())
					LOG_D<LG>("Sending SM to %s%s...", hidden.data(), end_opt->data());
				else
					LOG_D<LG>("Sending SM...");
			}
			
			SendUART("AT+CMGS=\""sv);
			SendUCS2(number);
			ErrorCode code = WaitForReady(1000, CommandType::Bare, ""sv, "\"\r"sv);
			
			uint8_t n;
			
			if (code != OK)
			{
				SendUART(ESC);
				goto ret;
			}
			
			for (auto msg : msgs)
				SendUCS2(msg);
			
			// \r\n+CMGS: 255\r\n\r\nOK\r\n
			code = ResponseToken(timeout, CommandType::Bare, "+CMGS"sv,
				[&n](const std::vector<strv>& tokens) -> ErrorCode { return sscanf(tokens[0].data(), "%3hhu", &n) == 1 ? OK : WRONG_FORMAT; }, 1, 2, CTRL_Z);
			
		ret:
			if (code == OK)
				LOG_D<LG>("SM sent successfully (%hhu).", n);
			else
				LOG_W<LG>("SM could not be sent (%d)!", code);
			
			return code;
		}
		
		ErrorCode SMSend(strv number, const STM32T::span<const wstrv> msgs, const uint32_t timeout = 60'000)
		{
			using STM32T::Log::IsEnabled;
			using STM32T::Log::LOG_D;
			using STM32T::Log::LOG_W;
			
			if constexpr (IsEnabled<LG>(Log::Level::Debug))
			{
				std::string hidden(std::min(number.size(), number.size() - 4), 'x');
				
				if (!hidden.empty())
					LOG_D<LG>("Sending SM to %s%.*s...", hidden.data(), number.size() >= 4 ? number.size() - 4 : 0, number.data() + number.size() - 4);
				else
					LOG_D<LG>("Sending SM...");
			}
			
			SendUART("AT+CMGS=\""sv);
			SendUCS2(number);
			ErrorCode code = WaitForReady(1000, CommandType::Bare, ""sv, "\"\r"sv);
			
			uint8_t n;
			
			if (code != OK)
			{
				SendUART(ESC);
				goto ret;
			}
			
			for (auto msg : msgs)
				SendUCS2(msg);
			
			// \r\n+CMGS: 255\r\n\r\nOK\r\n
			code = ResponseToken(timeout, CommandType::Bare, "+CMGS"sv,
				[&n](const std::vector<strv>& tokens) -> ErrorCode { return sscanf(tokens[0].data(), "%3hhu", &n) == 1 ? OK : WRONG_FORMAT; }, 1, 2, CTRL_Z);
			
		ret:
			if (code == OK)
				LOG_D<LG>("SM sent successfully (%hhu).", n);
			else
				LOG_W<LG>("SM could not be sent (%d)!", code);
			
			return code;
		}
		
		
		#ifdef STM32T_GSM_URC_ENABLED
	private:
		class URC
		{
			friend class GSM;
			friend class LinkedList<URC, 32>;
			
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
		
		uint8_t m_buf[(STM32T_GSM_URC_BUF_SIZE)];
		LinkedList<URC, 32> m_urcs;
		
		static inline GSM *s_this = nullptr;
		
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
					data.tokenize("\r\n"sv, [](const strv token) { s_this->addURC(token); }, false);
					
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
		* @retval - The number of urcs handled.
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
		#endif	// STM32T_GSM_URC_ENABLED
	};
}
#endif	// HAL_UART_MODULE_ENABLED
