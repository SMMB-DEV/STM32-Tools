#pragma once

#include "./strv.hpp"
#include "./Timing.hpp"

#include "main.h"
#include <cstdio>



#define FH_STDIN    0x8001
#define FH_STDOUT   0x8002
#define FH_STDERR   0x8003



#define STM32T_SYS_WRITE_GPIO(PORT, PIN, BAUD) \
extern "C" int stdout_putchar(int ch) { return ch; } \
extern "C" int _sys_write(int fh, const uint8_t *buf, uint32_t len, int mode) \
{ \
	static constexpr uint32_t BIT_TIME = STM32T_DELAY_CLK / (BAUD); \
	\
	\
	using namespace STM32T::Time; \
	if (fh != FH_STDOUT) \
		return fh == FH_STDERR ? 0 : -1; \
	\
	volatile uint32_t start = GetCycle(); \
	for (; len; len--) \
	{ \
		const uint8_t ch = *buf++; \
		\
		/* Start bit */ \
		(PORT)->BRR = (PIN); \
		WaitAfter(start, BIT_TIME, GetCycle); \
		start += BIT_TIME; \
		\
		/* Data */ \
		for (uint8_t i = 1; i; i <<= 1) \
		{ \
			(PORT)->BSRR = (PIN) << (16 * ((ch & i) == 0)); \
			WaitAfter(start, BIT_TIME, GetCycle); \
			start += BIT_TIME; \
		} \
		\
		/*Odd parity*/ \
		(PORT)->BSRR = (PIN) << (16 * (STM32T::bit_count(ch) % 2)); \
		WaitAfter(start, BIT_TIME, GetCycle); \
		start += BIT_TIME; \
		\
		/* Stop bit */ \
		(PORT)->BSRR = (PIN); \
		WaitAfter(start, BIT_TIME, GetCycle); \
		start += BIT_TIME; \
	} \
	\
	return 0; \
}

#define STM32T_SYS_WRITE_ITM() \
extern "C" int stdout_putchar(int ch) { return ch; } \
extern "C" int _sys_write(int fh, const uint8_t *buf, uint32_t len, int mode) \
{ \
	if (fh != FH_STDOUT) \
		return fh == FH_STDERR ? 0 : -1; \
	\
	if ((ITM_TCR & ITM_TCR_ITMENA_Msk) && /* ITM enabled */ (ITM_TER & (1UL << 0))) /* ITM Port #0 enabled */\
	{\
		for (uint32_t i = 0; i < len; i++)\
		{\
			while (ITM_PORT0_U32 != 0);\
			ITM_PORT0_U8 = buf[i];\
		}\
		return 0;\
	}\
	return -2;\
}

#define STM32T_SYS_WRITE_UART(PHUART) \
extern "C" int stdout_putchar(int ch) { return ch; } \
extern "C" int _sys_write(int fh, const uint8_t *buf, uint32_t len, int mode) \
{ \
	if (fh != FH_STDOUT) \
		return fh == FH_STDERR ? 0 : -1; \
	\
	return HAL_UART_Transmit((PHUART), buf, len, HAL_MAX_DELAY) == HAL_OK ? 0 : -1; \
}

#define STM32T_SYS_WRITE_USB \
extern "C" USBD_HandleTypeDef hUsbDeviceFS; \
extern "C" int stdout_putchar(int ch) { return ch; } \
extern "C" int _sys_write(int fh, const uint8_t *buf, uint32_t len, int mode) \
{ \
	if (fh != FH_STDOUT) \
		return fh == FH_STDERR ? 0 : -1; \
	\
	uint8_t res = USBD_OK; \
	uint32_t start = HAL_GetTick(); \
	while (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED && (res = CDC_Transmit_FS((uint8_t*)buf, len)) == USBD_BUSY && HAL_GetTick() - start < 100); \
	return 0;	/* If non-zero is returned once, it stops working. */ \
}

#define STM32T_SYS_WRITE_DYN() \
namespace STM32T \
{ \
	inline bool (*_g_dynLog)(const uint8_t *buf, uint32_t len) = nullptr; \
	inline void RedirectStdout(bool (*logger)(const uint8_t *buf, uint32_t len)) \
	{ \
		_g_dynLog = logger; \
	} \
} \
extern "C" int stdout_putchar(int ch) { return ch; } \
extern "C" int _sys_write(int fh, const uint8_t *buf, uint32_t len, int mode) \
{ \
	if (fh != FH_STDOUT) \
		return fh == FH_STDERR ? 0 : -1; \
	\
	if (STM32T::_g_dynLog) \
		return STM32T::_g_dynLog(buf, len) ? 0 : -1; \
	else \
		return 0; \
}



#if __has_include("usbd_cdc_if.h")
#include "usbd_cdc_if.h"

extern "C" USBD_HandleTypeDef hUsbDeviceFS;

namespace STM32T::Log
{
	inline void default_output_vcp(strv data)
	{
		uint8_t res;
		uint32_t start = HAL_GetTick();
		while (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED && (res = CDC_Transmit_FS((uint8_t*)data.data(), data.size())) == USBD_BUSY && HAL_GetTick() - start < 10);
	}
}
#endif

namespace STM32T
{
	namespace Log
	{
		enum class Level : uint8_t
		{
			None, Fatal, Error, Warning, Info, Debug, Max = 255
		};
		
		inline strv LevelStr(Level level)
		{
			switch (level)
			{
				case Level::None:
					return "None"sv;
				
				case Level::Fatal:
					return "Fatal"sv;
				
				case Level::Error:
					return "Error"sv;
				
				case Level::Warning:
					return "Warning"sv;
				
				case Level::Info:
					return "Info"sv;
				
				case Level::Debug:
					return "Debug"sv;
				
				default:
					return "Unknown"sv;
			}
		}
		
		using output_t = void (*)(strv data);
		using timestamp_t = strv (*)();
		
		inline void default_output_stdout(strv data)
		{
			fwrite(data.data(), 1, data.size(), stdout);
			fflush(stdout);
		}
		
		inline strv default_timestamp()
		{
			static char str[16];
			
			const uint32_t now = HAL_GetTick();
			int len = sprintf(str, "%10u", now);
			
			return strv(str, len);
		}
		
		template <size_t OUTPUT_COUNT = 1>
		class Logger
		{
		public:
			Level level;
			strv name;
			std::array<output_t, OUTPUT_COUNT> outputs;
			timestamp_t timestamp = default_timestamp;
			
			constexpr Logger(Level level, strv name) : level(level), name(name), outputs(std::array{default_output_stdout}) {}
			constexpr Logger(Level level, strv name, std::array<output_t, OUTPUT_COUNT> outputs) : level(level), name(name), outputs(outputs) {}
			constexpr Logger(Level level, strv name, std::array<output_t, OUTPUT_COUNT> outputs, timestamp_t timestamp) :
				level(level), name(name), outputs(outputs), timestamp(timestamp) {}
			constexpr Logger(Level level, strv name, timestamp_t timestamp) : level(level), name(name), outputs(std::array{default_output_stdout}), timestamp(timestamp) {}
			
			constexpr bool isEnabled() const
			{
				return level > Level::None;
			}
			
			constexpr bool isEnabled(const Level level) const
			{
				return this->level >= level;
			}
			
			void log(const Level level, const char *fmt, ...) const
			{
				if (isEnabled(level))
				{
					va_list args;
					va_start(args, fmt);
					log(level, fmt, args);
					va_end(args);
				}
			}
			
			void log(const Level level, const char *fmt, va_list args) const
			{
				if (isEnabled(level))
				{
					if (level > Level::None)
					{
						if (timestamp)
							dispatch_format(timestamp());
						
						dispatch_format(LevelStr(level));
						
						if (!name.empty())
							dispatch_format(name);
						
						dispatch_chunk(": ", 2);
					}
					
					size_t written = 0;
					
					const char *p = fmt;
					for (; *p; p++)
					{
						if (*p != '%')
							continue;
						
						dispatch_chunk({fmt, (size_t)(p - fmt)});	// dispatch plain text
						written += p - fmt;
						
						const char *const start = p++;
						
						char spec[16], flags[5 + 1], field_width[4 + 1], precision[4 + 1], modifier[2 + 1];
						
						if (!extract_conv_spec("-+ #0", flags, sizeof(flags), p))
							return;
						
						if (!extract_conv_spec("0123456789*", field_width, sizeof(field_width), p))
							return;
						
						if (!extract_conv_spec(".0123456789*", precision, sizeof(precision), p))
							return;
						
						if (!extract_conv_spec("hlLzjt", modifier, sizeof(modifier), p))
							return;
						
						memcpy(spec, start, p - start + 1);
						spec[p - start + 1] = '\0';
						fmt = p + 1;
						
						int field_width_val;
						bool field_width_present;
						if ((field_width_present = strcmp(field_width, "*") == 0))
							field_width_val = va_arg(args, int);
						
						int precision_val;
						bool precision_present;
						if ((precision_present = strcmp(precision, ".*") == 0))
							precision_val = va_arg(args, int);
						
						char var[64];
						int n = 0;
						
						auto format = [&]<typename T>()
						{
							if (field_width_present && precision_present)
								n = snprintf(var, sizeof(var), spec, field_width_val, precision_val, va_arg(args, T));
							else if (field_width_present)
								n = snprintf(var, sizeof(var), spec, field_width_val, va_arg(args, T));
							else if (precision_present)
								n = snprintf(var, sizeof(var), spec, precision_val, va_arg(args, T));
							else
								n = snprintf(var, sizeof(var), spec, va_arg(args, T));
						};
						
						// todo: check ll, etc.
						switch (*p)
						{
							case 'd': case 'i':
							{
								if (strcmp(modifier, "l") == 0)
									format.template operator()<long>();
								else if (strcmp(modifier, "ll") == 0)
									format.template operator()<long long>();
								else if (strcmp(modifier, "j") == 0)
									format.template operator()<intmax_t>();
								else if (strcmp(modifier, "z") == 0)
									format.template operator()<std::make_signed<size_t>>();
								else if (strcmp(modifier, "t") == 0)
									format.template operator()<ptrdiff_t>();
								else
									format.template operator()<int>();
								
								break;
							}
							
							case 'u': case 'x': case 'X': case 'o':
							{
								if (strcmp(modifier, "l") == 0)
									format.template operator()<unsigned long>();
								else if (strcmp(modifier, "ll") == 0)
									format.template operator()<unsigned long long>();
								else if (strcmp(modifier, "j") == 0)
									format.template operator()<uintmax_t>();
								else if (strcmp(modifier, "z") == 0)
									format.template operator()<size_t>();
								else if (strcmp(modifier, "t") == 0)
									format.template operator()<std::make_unsigned<ptrdiff_t>>();
								else
									format.template operator()<unsigned int>();
								
								break;
							}
							
							case 'f': case 'F': case 'g': case 'G': case 'e': case 'E': case 'a': case 'A':
							{
								if (strcmp(modifier, "L") == 0)
									format.template operator()<long double>();
								else
									format.template operator()<double>();
								
								break;
							}
							
							case 'c':
							{
								if (strcmp(modifier, "l") == 0)
									format.template operator()<wint_t>();
								else
									format.template operator()<int>();
								
								break;
							}
							
							case 's':
							{
								if (strcmp(modifier, "l") == 0)
									format.template operator()<const wchar_t *>();
								else
									format.template operator()<const char *>();
								
								break;
							}
							
							case 'p':
							{
								format.template operator()<void *>();
								break;
							}
							
							case 'n':
							{
								if (strcmp(modifier, "hh") == 0)
									*va_arg(args, signed char *) = written;
								else if (strcmp(modifier, "h") == 0)
									*va_arg(args, short *) = written;
								else if (strcmp(modifier, "l") == 0)
									*va_arg(args, long *) = written;
								else if (strcmp(modifier, "ll") == 0)
									*va_arg(args, long long *) = written;
								else if (strcmp(modifier, "j") == 0)
									*va_arg(args, intmax_t *) = written;
								else if (strcmp(modifier, "z") == 0)
									*va_arg(args, size_t *) = written;
								else if (strcmp(modifier, "t") == 0)
									*va_arg(args, ptrdiff_t *) = written;
								else
									*va_arg(args, int *) = written;
								
								break;
							}
							
							case '%':
							{
								if (spec[0] != '\0') // must be empty
									return;
								
								dispatch_chunk("%", 1);
								written++;
								break;
							}
							
							default:
								return;
						}
						
						if (n < 0)
							return;
						
						dispatch_chunk({var, std::min((size_t)n, sizeof(var) - 1)});
						written += std::min((size_t)n, sizeof(var) - 1);
						
						if (n > sizeof(var) - 1)
						{
							dispatch_chunk("..."sv);
							written += 3;
						}
					}
					
					dispatch_chunk(fmt, p - fmt);
				}
			}
			
			template <class... Args>
			void f(const char *fmt, Args... args) const { log(Level::Fatal, fmt, args...); }
			
			template <class... Args>
			void e(const char *fmt, Args... args) const { log(Level::Error, fmt, args...); }
			
			template <class... Args>
			void w(const char *fmt, Args... args) const { log(Level::Warning, fmt, args...); }
			
			template <class... Args>
			void i(const char *fmt, Args... args) const { log(Level::Info, fmt, args...); }
			
			template <class... Args>
			void d(const char *fmt, Args... args) const { log(Level::Debug, fmt, args...); }
			
		private:
			void dispatch_chunk(const char *buf, size_t len) const
			{
				for (auto& out : outputs)
					out({buf, len});
			}
			
			void dispatch_chunk(strv data) const
			{
				for (auto& out : outputs)
					out(data);
			}
			
			void dispatch_format(strv format) const
			{
				dispatch_chunk("["sv);
				dispatch_chunk(format);
				dispatch_chunk("]"sv);
			}
			
			static bool extract_conv_spec(const char *chars, char *spec, size_t spec_len, const char * &p)
			{
				const char *const start = p;
				while (strchr(chars, *p))
				{
					*spec++ = *p++;
					if (p - start >= spec_len)
						return false;
				}
				
				*spec = '\0';
				
				return *p != '\0';
			}
		};
		
		#ifndef STM32T_DEFAULT_LOG
		#define STM32T_DEFAULT_LOG	(Level::None, ""sv, std::array{default_output_stdout})
		#endif
		
		#ifdef STM32T_DEFAULT_LOG_DEBUG
		inline constexpr Logger g_defaultLogger(Level::Debug, ""sv, std::array{default_output_stdout});
		#elifdef STM32T_DEFAULT_LOG_INFO
		inline constexpr Logger g_defaultLogger(Level::Info, ""sv, std::array{default_output_stdout});
		#elifdef STM32T_DEFAULT_LOG_WARNING
		inline constexpr Logger g_defaultLogger(Level::Warning, ""sv, std::array{default_output_stdout});
		#elifdef STM32T_DEFAULT_LOG_ERROR
		inline constexpr Logger g_defaultLogger(Level::Error, ""sv, std::array{default_output_stdout});
		#elifdef STM32T_DEFAULT_LOG_FATAL
		inline constexpr Logger g_defaultLogger(Level::Fatal, ""sv, std::array{default_output_stdout});
		#else
		inline constexpr Logger g_defaultLogger STM32T_DEFAULT_LOG;
		#endif
		
		constexpr inline bool IsEnabled()
		{
			return g_defaultLogger.isEnabled();
		}
		
		template <auto& logger, const Level level, class... Args>
		[[gnu::always_inline]] void _LOG(const char *fmt, Args... args)
		{
			if constexpr (logger.isEnabled(level))
				logger.log(level, fmt, args...);
		}
		
		template <auto& logger, class... Args>
		void LOG_N(const char *fmt, Args... args)
		{
			if constexpr (logger.isEnabled())
				logger.log(Level::None, fmt, args...);
		}
		
		template <auto& logger, const Level level, class... Args>
		void LOG_N(const char *fmt, Args... args)
		{
			if constexpr (logger.isEnabled() && logger.isEnabled(level))
				logger.log(Level::None, fmt, args...);
		}
		
		template <class... Args>
		[[gnu::always_inline]]
		inline void LOG_N(const char *fmt, Args... args)
		{
			LOG_N<g_defaultLogger>(fmt, args...);
		}
		
		template <const Level level, class... Args>
		[[gnu::always_inline]]
		inline void LOG_N(const char *fmt, Args... args)
		{
			LOG_N<g_defaultLogger, level>(fmt, args...);
		}
		
		template <auto& logger, class... Args>
		void LOG_F(const char *fmt, Args... args) { _LOG<logger, Level::Fatal>(fmt, args...); }
		
		template <class... Args>
		inline void LOG_F(const char *fmt, Args... args) { _LOG<g_defaultLogger, Level::Fatal>(fmt, args...); }
		
		template <auto& logger, class... Args>
		void LOG_E(const char *fmt, Args... args) { _LOG<logger, Level::Error>(fmt, args...); }
		
		template <class... Args>
		inline void LOG_E(const char *fmt, Args... args) { _LOG<g_defaultLogger, Level::Error>(fmt, args...); }
		
		template <auto& logger, class... Args>
		void LOG_W(const char *fmt, Args... args) { _LOG<logger, Level::Warning>(fmt, args...); }
		
		template <class... Args>
		inline void LOG_W(const char *fmt, Args... args) { _LOG<g_defaultLogger, Level::Warning>(fmt, args...); }
		
		template <auto& logger, class... Args>
		void LOG_I(const char *fmt, Args... args) { _LOG<logger, Level::Info>(fmt, args...); }
		
		template <class... Args>
		inline void LOG_I(const char *fmt, Args... args) { _LOG<g_defaultLogger, Level::Info>(fmt, args...); }
		
		template <auto& logger, class... Args>
		void LOG_D(const char *fmt, Args... args) { _LOG<logger, Level::Debug>(fmt, args...); }
		
		template <class... Args>
		inline void LOG_D(const char *fmt, Args... args) { _LOG<g_defaultLogger, Level::Debug>(fmt, args...); }
		
		template <auto& logger>
		inline void LOGA(const uint8_t *arr, size_t len, const size_t line_count = 16)
		{
			if constexpr (logger.isEnabled())
			{
				while (len)
				{
					for (size_t i = 0; len && i < line_count; i++, len--)
						LOG_N<logger>("%02X ", *arr++);
					
					LOG_N<logger>("\n");
				}
			}
		}
		
		inline void LOGA(const uint8_t *arr, size_t len, const size_t line_count = 16)
		{
			LOGA<g_defaultLogger>(arr, len, line_count);
		}
		
		template <auto& logger>
		inline void LOGSEP()
		{
			LOG_N<logger>("--------------------------------------------------------------------------------\n");
		}
		
		inline void LOGSEP()
		{
			LOG_N("--------------------------------------------------------------------------------\n");
		}
		
		inline void DoIfEnabled(void (*f)())
		{
			if constexpr (IsEnabled())
				f();
		}
		
		template <auto& logger = g_defaultLogger>
		inline void Startup()
		{
			DoIfEnabled([]()
			{
				LOG_N<logger>("\n\n\n--------------------------------------------------------------------------------\nStart!\n");
				
				const uint32_t version = HAL_GetHalVersion();	// major.minor.patch-rc
				
				LOG_N<logger>("\nHAL v%hhu.%hhu.%hhu-rc%hhu\n" "RevID: 0x%X, DevID: 0x%X, UID: 0x%08X%08X%08X\n" "HCLK: %.1f MHz\n\n",
					version >> 24, (version >> 16) & 0xFF, (version >> 8) & 0xFF, version & 0xFF,
					HAL_GetREVID(), HAL_GetDEVID(), HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2(),
					HAL_RCC_GetHCLKFreq() / 1'000'000.0f);
			});
		}
	}
}



using STM32T::Log::LOG_N;
using STM32T::Log::LOG_F;
using STM32T::Log::LOG_E;
using STM32T::Log::LOG_W;
using STM32T::Log::LOG_I;
using STM32T::Log::LOG_D;
using STM32T::Log::LOGA;
using STM32T::Log::LOGSEP;
