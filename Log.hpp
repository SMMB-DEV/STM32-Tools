#pragma once

#include "./Core/strv.hpp"
#include "./Core/Time.hpp"
#include "./Versioning.hpp"

#include <cstdio>



#define FH_STDIN    0x8001
#define FH_STDOUT   0x8002
#define FH_STDERR   0x8003



#define STM32T_SYS_WRITE_GPIO(PORT, PIN, BAUD) \
extern "C" int stdout_putchar(int ch) { return ch; } \
extern "C" int _sys_write(int fh, const uint8_t *buf, uint32_t len, int mode) \
{ \
	static constexpr uint32_t BIT_TIME = (STM32T_DELAY_CLK) / (BAUD); \
	\
	using namespace STM32T::Time; \
	\
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

#define STM32T_SYS_WRITE_ITM \
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

#define STM32T_SYS_WRITE_DYN \
namespace STM32T::Log \
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
	if (STM32T::Log::_g_dynLog) \
		return STM32T::Log::_g_dynLog(buf, len) ? 0 : -1; \
	else \
		return 0; \
}



#if __has_include("usbd_cdc_if.h")
#include "usbd_cdc_if.h"

extern "C" USBD_HandleTypeDef hUsbDeviceFS;

namespace STM32T::Log
{
	inline void default_output_vcp(strv data, bool last_chunk)
	{
		uint32_t start = HAL_GetTick();
		while (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED && CDC_Transmit_FS((uint8_t*)data.data(), data.size()) == USBD_BUSY && HAL_GetTick() - start < 50);
	}
}
#endif

namespace STM32T::Log
{
	// todo: add Fatal handler
	enum class Level : uint8_t
	{
		None, Fatal, Error, Warning, Info, Debug, Max = 255
	};
	
	inline strv LevelStr(Level level)
	{
		switch (level)
		{
			case Level::None:		return "None "sv;
			case Level::Fatal:		return "Fatal"sv;
			case Level::Error:		return "Error"sv;
			case Level::Warning:	return "Warn "sv;
			case Level::Info:		return "Info "sv;
			case Level::Debug:		return "Debug"sv;
			default:				return " ??? "sv;
		}
	}
	
	using output_t = void (*)(strv data, bool last_chunk);
	using timestamp_t = strv (*)();
	
	inline void default_output_stdout(strv data, bool last_chunk)
	{
		fwrite(data.data(), 1, data.size(), stdout);
		
		if (last_chunk)
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
		
		constexpr Logger Clone() const { return Logger(level, name, outputs, timestamp); }
		constexpr Logger Clone(strv name) const { return Logger(level, name, outputs, timestamp); }
		constexpr Logger Clone(Level level, strv name) const { return Logger(level, name, outputs, timestamp); }
		
		constexpr bool isEnabled() const { return level > Level::None; }
		constexpr bool isEnabled(const Level level) const { return this->level >= level; }
		
		void log(const Level level, const char *fmt, ...) const
		{
			if (!isEnabled(level))
				return;
			
			va_list args;
			va_start(args, fmt);
			log(level, fmt, args);
			va_end(args);
		}
		
		void log(const Level level, const char *fmt, va_list args) const
		{
			if (!isEnabled(level))
				return;
			
			if (level > Level::None)
			{
				if (timestamp)
					dispatch_format(timestamp());
				
				if (level != Level::Max)
					dispatch_format(LevelStr(level));
				
				if (!name.empty())
					dispatch_format(name);
				
				dispatch_chunk(": "sv);
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
				
				char flags[5 + 1], field_width[2 + 1], precision[3 + 1], modifier[2 + 1],
					spec[std::size(flags) - 1 + std::size(field_width) - 1 + std::size(precision) - 1 + std::size(modifier) - 1 + 1 + 1];
				
				if (!extract_conv_spec("-+ #0", flags, sizeof(flags), p))
					return;
				
				if (!extract_conv_spec("*0123456789", field_width, sizeof(field_width), p))
					return;
				
				if (!extract_conv_spec(".*0123456789", precision, sizeof(precision), p))
					return;
				
				if (!extract_conv_spec("hlLzjt", modifier, sizeof(modifier), p))
					return;
				
				memcpy(spec, start, p + 1 - start);
				spec[p + 1 - start] = '\0';
				fmt = p + 1;
				
				const bool has_field_width = strcmp(field_width, "*") == 0, has_precision = strcmp(precision, ".*") == 0;
				
				char var[100];
				int n = 0;
				
				switch (*p)
				{
					case 'd': case 'i':
					{
						if (strcmp(modifier, "l") == 0)
							n = format<long>(var, sizeof(var), spec, has_field_width, has_precision, args);
						else if (strcmp(modifier, "ll") == 0)
							n = format<long long>(var, sizeof(var), spec, has_field_width, has_precision, args);
						else if (strcmp(modifier, "j") == 0)
							n = format<intmax_t>(var, sizeof(var), spec, has_field_width, has_precision, args);
						else if (strcmp(modifier, "z") == 0)
							n = format<std::make_signed<size_t>>(var, sizeof(var), spec, has_field_width, has_precision, args);
						else if (strcmp(modifier, "t") == 0)
							n = format<ptrdiff_t>(var, sizeof(var), spec, has_field_width, has_precision, args);
						else
							n = format<int>(var, sizeof(var), spec, has_field_width, has_precision, args);
						
						break;
					}
					
					case 'u': case 'x': case 'X': case 'o':
					{
						if (strcmp(modifier, "l") == 0)
							n = format<unsigned long>(var, sizeof(var), spec, has_field_width, has_precision, args);
						else if (strcmp(modifier, "ll") == 0)
							n = format<unsigned long long>(var, sizeof(var), spec, has_field_width, has_precision, args);
						else if (strcmp(modifier, "j") == 0)
							n = format<uintmax_t>(var, sizeof(var), spec, has_field_width, has_precision, args);
						else if (strcmp(modifier, "z") == 0)
							n = format<size_t>(var, sizeof(var), spec, has_field_width, has_precision, args);
						else if (strcmp(modifier, "t") == 0)
							n = format<std::make_unsigned<ptrdiff_t>>(var, sizeof(var), spec, has_field_width, has_precision, args);
						else
							n = format<unsigned int>(var, sizeof(var), spec, has_field_width, has_precision, args);
						
						break;
					}
					
					case 'f': case 'F': case 'g': case 'G': case 'e': case 'E': case 'a': case 'A':
					{
						if (strcmp(modifier, "L") == 0)
							n = format<long double>(var, sizeof(var), spec, has_field_width, has_precision, args);
						else
							n = format<double>(var, sizeof(var), spec, has_field_width, has_precision, args);
						
						break;
					}
					
					case 'c':
					{
						if (strcmp(modifier, "l") == 0)
							n = format<wint_t>(var, sizeof(var), spec, has_field_width, has_precision, args);
						else
							n = format<int>(var, sizeof(var), spec, has_field_width, has_precision, args);
						
						break;
					}
					
					case 's':
					{
						if (strcmp(modifier, "l") == 0)
							n = format<const wchar_t *>(var, sizeof(var), spec, has_field_width, has_precision, args);
						else
						{
							if (strcmp(spec, "%s") == 0 || strcmp(spec, "%.*s") == 0)
							{
								int precision = INT_MAX;
								if (has_precision)
									precision = va_arg(args, int);
								
								if (precision < 0)
									precision = INT_MAX;
								
								const char *buf = va_arg(args, const char *);
								const size_t len = std::min(std::strlen(buf), size_t(precision));	// todo: does strlen matter when precision is specified?
								
								if (len > 0)
								{
									dispatch_chunk({buf, len});
									written += len;
								}
							}
							else
								n = format<const char *>(var, sizeof(var), spec, has_field_width, has_precision, args);
						}
						
						break;
					}
					
					case 'p':
					{
						if (strcmp(modifier, "") != 0)
							return;
						
						n = format<void *>(var, sizeof(var), spec, has_field_width, has_precision, args);
						break;
					}
					
					case 'n':
					{
						if (strcmp(flags, "") != 0 || strcmp(field_width, "") != 0 || strcmp(precision, "") != 0)
							return;
						
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
						if (strcmp(spec, "%%") != 0)	// must match
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
				else if (n > 0)
				{
					dispatch_chunk({var, std::min((size_t)n, sizeof(var) - 1)});
					written += std::min((size_t)n, sizeof(var) - 1);
					
					if (n >= sizeof(var))
					{
						dispatch_chunk("..."sv);
						written += 3;
					}
				}
			}
			
			const bool none = level == Level::None;
			dispatch_chunk({fmt, (size_t)(p - fmt)}, none);
			
			if (!none)
				dispatch_chunk("\n"sv, true);
		}
		
		template <class... Args>
		void n(const char *fmt, Args... args) const { if (isEnabled()) log(Level::None, fmt, args...); }
		
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
		void dispatch_chunk(const char *buf, size_t len, bool last = false) const
		{
			for (auto out : outputs)
				if (out)
					out({buf, len}, last);
		}
		
		void dispatch_chunk(strv data, bool last = false) const
		{
			for (auto out : outputs)
				if (out)
					out(data, last);
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
		
		template <typename T>
		static int format(char *const var, const size_t var_len, const char *const spec, const bool has_field_width, const bool has_precision, va_list& args)
		{
			int field_width;
			if (has_field_width)
				field_width = va_arg(args, int);
			
			int precision;
			if (has_precision)
				precision = va_arg(args, int);
			
			if (has_field_width && has_precision)
				return snprintf(var, var_len, spec, field_width, precision, va_arg(args, T));
			else if (has_field_width)
				return snprintf(var, var_len, spec, field_width, va_arg(args, T));
			else if (has_precision)
				return snprintf(var, var_len, spec, precision, va_arg(args, T));
			else
				return snprintf(var, var_len, spec, va_arg(args, T));
		}
	};
	
	#ifndef STM32T_DEFAULT_LOG_LEVEL
	#define	STM32T_DEFAULT_LOG_LEVEL		Level::None
	#endif
	
	#ifndef STM32T_DEFAULT_LOG_NAME
	#define	STM32T_DEFAULT_LOG_NAME			""sv
	#endif
	
	#ifndef STM32T_DEFAULT_LOG_OUTPUT
	#define	STM32T_DEFAULT_LOG_OUTPUT		std::array{default_output_stdout}
	#endif
	
	#ifndef STM32T_DEFAULT_LOG_TIMESTAMP
	#define	STM32T_DEFAULT_LOG_TIMESTAMP	default_timestamp
	#endif
	
	inline constexpr Logger g_defaultLogger(STM32T_DEFAULT_LOG_LEVEL, STM32T_DEFAULT_LOG_NAME, STM32T_DEFAULT_LOG_OUTPUT, STM32T_DEFAULT_LOG_TIMESTAMP);
	
	constexpr inline bool IsEnabled()
	{
		return g_defaultLogger.isEnabled();
	}
	
	constexpr inline bool IsEnabled(const Level level)
	{
		return g_defaultLogger.isEnabled(level);
	}
	
	template <auto& logger, const Level level, class... Args>
	[[gnu::always_inline]] void _LOG(const char *fmt, Args... args)
	{
		if constexpr (logger.isEnabled(level))
			logger.log(level, fmt, args...);
	}
	
	
	
	template <const Level level = Level::None, auto& logger = g_defaultLogger, class... Args>
	[[gnu::always_inline]]
	inline void LOG_N(const char *fmt, Args... args)
	{
		if constexpr (logger.isEnabled() && logger.isEnabled(level))
			logger.log(Level::None, fmt, args...);
	}
	
	
	
	template <auto& logger = g_defaultLogger, class... Args>
	void LOG_F(const char *fmt, Args... args) { _LOG<logger, Level::Fatal>(fmt, args...); }
	
	template <auto& logger = g_defaultLogger, class... Args>
	void LOG_E(const char *fmt, Args... args) { _LOG<logger, Level::Error>(fmt, args...); }
	
	template <auto& logger = g_defaultLogger, class... Args>
	void LOG_W(const char *fmt, Args... args) { _LOG<logger, Level::Warning>(fmt, args...); }
	
	template <auto& logger = g_defaultLogger, class... Args>
	void LOG_I(const char *fmt, Args... args) { _LOG<logger, Level::Info>(fmt, args...); }
	
	template <auto& logger = g_defaultLogger, class... Args>
	void LOG_D(const char *fmt, Args... args) { _LOG<logger, Level::Debug>(fmt, args...); }
	
	template <const Level level = Level::None, auto& logger = g_defaultLogger>
	inline void LOGA(const uint8_t *arr, size_t len, const size_t line_count = 16)
	{
		if constexpr (logger.isEnabled() && logger.isEnabled(level))
		{
			while (len)
			{
				for (size_t i = 0; len && i < line_count; i++, len--)
					LOG_N<level, logger>(" %02X", *arr++);
				
				LOG_N<level, logger>("\n");
			}
		}
	}
	
	template <const Level level = Level::None, auto& logger = g_defaultLogger>
	inline void LOGSEP()
	{
		LOG_N<level, logger>("--------------------------------------------------------------------------------\n");
	}
	
	inline void DoIfEnabled(void (*f)())
	{
		if constexpr (IsEnabled())
			f();
	}
	
	template <typename R>
	inline R DoIfEnabled(R (*f)())
	{
		if constexpr (IsEnabled())
			return f();
		else
			return R();
	}
	
	template <const Level level>
	inline void DoIfEnabled(void (*f)())
	{
		if constexpr (IsEnabled(level))
			f();
	}
	
	template <const Level level, typename R>
	inline R DoIfEnabled(R (*f)())
	{
		if constexpr (IsEnabled(level))
			return f();
		else
			return R();
	}
	
	template <const Level level = Level::None, auto& logger = g_defaultLogger>
	inline void Startup()
	{
		if constexpr (logger.isEnabled() && logger.isEnabled(level))
		{
			LOG_N<level, logger>("\n\n\n--------------------------------------------------------------------------------\nStart!\n");
			
			strv ver = VER;
			LOG_N<level, logger>("\nSTM32T v%.*s\n", ver.size(), ver.data());
			
			const uint32_t version = HAL_GetHalVersion();	// major.minor.patch-rc
			
			LOG_N<level, logger>("HAL v%hhu.%hhu.%hhu-rc%hhu\nRevID: 0x%04X, DevID: 0x%03X, UID: 0x%08X%08X%08X\nHCLK: %.1f MHz\n",
				version >> 24, (version >> 16) & 0xFF, (version >> 8) & 0xFF, version & 0xFF,
				HAL_GetREVID(), HAL_GetDEVID(), HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2(),
				HAL_RCC_GetHCLKFreq() / 1'000'000.0f);
			
			// https://community.st.com/t5/stm32cubeide-mcus/how-can-you-validate-that-independent-watchdog-iwdg-is-resetting/m-p/89220/highlight/true#M2197
			const uint32_t reset_flags = RCC->CSR;
			
			LOG_N<level, logger>("Cause of reset: |");
			
			// b31
			if (reset_flags & RCC_CSR_LPWRRSTF)
				LOG_N<level, logger>(" Low Power |");
			
			// b30
			if (reset_flags & RCC_CSR_WWDGRSTF)
				LOG_N<level, logger>(" WWDG |");
			
			// b29
			if (reset_flags & RCC_CSR_IWDGRSTF)
				LOG_N<level, logger>(" IWDG |");
			
			// b28
			if (reset_flags & RCC_CSR_SFTRSTF)
				LOG_N<level, logger>(" Software |");
			
			// b27
			#ifdef RCC_CSR_PORRSTF
			if (reset_flags & RCC_CSR_PORRSTF)
			#else
			if (reset_flags & RCC_CSR_PWRRSTF)
			#endif
				LOG_N<level, logger>(" POR/PDR |");
			
			// b26
			if (reset_flags & RCC_CSR_PINRSTF)
				LOG_N<level, logger>(" Reset Pin |");
			
			// b25
			#ifdef RCC_CSR_PORRSTF
			if (reset_flags & RCC_CSR_BORRSTF)
				LOG_N<level, logger>(" BOR |");
			#else
			if (reset_flags & RCC_CSR_OBLRSTF)
				LOG_N<level, logger>(" OBL |");
			#endif
			
			LOG_N<level, logger>("\n\n");
		}
	}
}
