#pragma once

#include "./Core/strv.hpp"
#include "./Core/Time.hpp"
#include "./Core/Utils.hpp"
#include "./Versioning.hpp"
#include "./IO.hpp"

#include <cstdio>



#define STM32T_SYS_WRITE_GPIO(PORT, PIN, BAUD) \
static_assert(0, "STM32T_SYS_WRITE_GPIO is obsolete and has no effect." \
	" Use STM32T_LOG_SYS_WRITE and call STM32T::Log::RedirectStdout(STM32T::Log::default_output_gpio) and STM32T::Log::SetGPIOConfig().");

#define STM32T_SYS_WRITE_ITM \
static_assert(0, "STM32T_SYS_WRITE_ITM is obsolete and has no effect." \
	" Use STM32T_LOG_SYS_WRITE and call STM32T::Log::RedirectStdout(STM32T::Log::default_output_itm).");

#define STM32T_SYS_WRITE_UART(PHUART) \
static_assert(0, "STM32T_SYS_WRITE_UART is obsolete and has no effect." \
	" Use STM32T_LOG_SYS_WRITE and call STM32T::Log::RedirectStdout(STM32T::Log::default_output_uart) and STM32T::Log::SetUARTHandle().");

#define STM32T_SYS_WRITE_UART_DMA(PHUART) \
static_assert(0, "STM32T_SYS_WRITE_UART_DMA is obsolete and has no effect." \
	" Use STM32T_LOG_SYS_WRITE and call STM32T::Log::RedirectStdout(STM32T::Log::default_output_uart_dma) and STM32T::Log::SetUARTHandle().");

#define STM32T_SYS_WRITE_USB \
static_assert(0, "STM32T_SYS_WRITE_USB is obsolete and has no effect." \
	" Use STM32T_LOG_SYS_WRITE and call STM32T::Log::RedirectStdout(STM32T::Log::default_output_vcp).");

#define STM32T_LOG_SYS_WRITE \
extern "C" int _sys_write(int fh, const uint8_t *buf, uint32_t len, int mode) \
{ \
	static constexpr int FH_STDIN = 0x8001, FH_STDOUT = 0x8002, FH_STDERR = 0x8003; \
	\
	if (fh != FH_STDOUT) \
		return fh == FH_STDERR ? 0 : -1; \
	\
	if (STM32T::Log::_g_stdout) \
		STM32T::Log::_g_stdout({reinterpret_cast<const char *>(buf), len}, true); \
	\
	return 0; \
}



namespace STM32T::Log
{
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
	
	using timestamp_t = const char * (*)();
	using handler_t = void (*)();
	using output_t = void (*)(strv data, bool last_chunk);
	
	inline const char * default_timestamp()
	{
		static char str[10 + 1];
		
		sprintf(str, "%10u", HAL_GetTick());
		return str;
	}
	
	inline void default_output_stdout(strv data, bool last_chunk)
	{
		fwrite(data.data(), 1, data.size(), stdout);
		
		if (last_chunk)
			fflush(stdout);
	}
	
	template <size_t SIZE, size_t COUNT = 1>
	struct Buffer
	{
		char data[COUNT][SIZE];
		size_t index = 0;
		ClampedInt<size_t, 0, COUNT - 1> current = 0;
	};
	
	template <size_t SIZE, size_t COUNT>
	inline void default_output_buffer(strv data, bool last_chunk, Buffer<SIZE, COUNT> *buf, void (*send)(const char *buf, size_t len))
	{
		// todo: Keep adding to the current buffer after a last chunk if the previous buffer hasn't been completely sent yet.
		
		if (buf->index)
		{
			if (buf->index + data.size() < SIZE)
			{
				std::memcpy(buf->data[buf->current] + buf->index, data.data(), data.size());
				buf->index += data.size();
				
				if (last_chunk)
				{
					send(buf->data[buf->current], buf->index);
					buf->index = 0;
					buf->current++;
				}
				
				return;
			}
			
			const size_t copied = SIZE - buf->index;
			std::memcpy(buf->data[buf->current] + buf->index, data.data(), copied);
			
			send(buf->data[buf->current], SIZE);
			buf->index = 0;
			buf->current++;
			data.remove_prefix(copied);
		}
		
		// todo: Send all at once for last_chunk?
		while (data.size() >= SIZE)
		{
			send(data.data(), SIZE);
			data.remove_prefix(SIZE);
		}
		
		if (!last_chunk)
		{
			std::memcpy(buf->data[buf->current], data.data(), data.size());
			buf->index = data.size();
		}
		else
			send(data.data(), data.size());
	}
	
	struct GPIOConfig
	{
		enum Parity : uint8_t {None, Even, Odd, Mark, Space};
		
		GPIO_TypeDef *port;
		uint16_t pin;
		uint32_t baud;
		uint8_t data_bits;
		Parity parity;
	} inline _g_stdout_gpio;
	
	inline void SetGPIOConfig(GPIO_TypeDef *port, uint16_t pin, uint32_t baud, uint8_t data_bits = 8, GPIOConfig::Parity parity = GPIOConfig::None)
	{
		_g_stdout_gpio.port = port;
		_g_stdout_gpio.pin = pin;
		_g_stdout_gpio.baud = baud;
		_g_stdout_gpio.data_bits = data_bits;
		_g_stdout_gpio.parity = parity;
	}
	
	inline void default_output_gpio(strv data, bool last_chunk)
	{
		static volatile Time::cycle_t s_start = 0;	// fixme: this doesn't work
		
		GPIO_TypeDef *const port = _g_stdout_gpio.port;
		const uint16_t pin = _g_stdout_gpio.pin;
		const Time::cycle_t BIT_TIME = (STM32T_TIME_CLK) / _g_stdout_gpio.baud;
		const uint8_t bits = _g_stdout_gpio.data_bits;
		
		for (auto ch : data)
		{
			// Start bit
			Time::WaitAfter(s_start, BIT_TIME);
			s_start += BIT_TIME;
			port->BSRR = pin << 16;
			
			// Data
			const uint8_t byte = ch & (0xFF >> (8 - bits));
			for (uint8_t i = 0; i < bits; ++i)
			{
				Time::WaitAfter(s_start, BIT_TIME);
				s_start += BIT_TIME;
				port->BSRR = pin << 16 * !(ch & 1);
				ch >>= 1;
			}
			
			// Parity
			Time::WaitAfter(s_start, BIT_TIME);
			s_start += BIT_TIME;
			
			switch (_g_stdout_gpio.parity)
			{
				case GPIOConfig::Even:
				{
					port->BSRR = pin << 16 * !parity(byte);
					break;
				}
				
				case GPIOConfig::Odd:
				{
					port->BSRR = pin << 16 * parity(byte);
					break;
				}
				
				case GPIOConfig::Mark:
				{
					port->BSRR = pin;
					break;
				}
				
				case GPIOConfig::Space:
				{
					port->BSRR = pin << 16;
					break;
				}
				
				default:
					goto stop;
			}
			
			// Stop bit
			Time::WaitAfter(s_start, BIT_TIME);
			s_start += BIT_TIME;
			
		stop:
			port->BSRR = pin;
		}
	}
	
	#ifdef ITM
	inline void default_output_itm(strv data, bool last_chunk)	// todo: Rewrite (Unlock and enable ITM, check DWT status if necessary)
	{
		// ITM enabled & ITM port #0 enabled
		if (READ_BIT(ITM->TCR, ITM_TCR_ITMENA_Msk) &&  READ_BIT(ITM->TER, (1UL << 0)))
		{
			for (auto ch : data)
			{
				while (ITM->PORT[0].u32);
				ITM->PORT[0].u8 = ch;
			}
		}
	}
	#endif	// ITM
	
	#ifdef HAL_UART_MODULE_ENABLED
	inline UART_HandleTypeDef *_g_huart = nullptr;
	
	inline void SetUARTHandle(UART_HandleTypeDef *huart) { _g_huart = huart; }
	
	inline void default_output_uart(strv data, bool last_chunk)
	{
		if (_g_huart)
			HAL_UART_Transmit(_g_huart, reinterpret_cast<const uint8_t *>(data.data()), data.size(), HAL_MAX_DELAY);
	}
	
	inline void default_output_uart_dma(strv data, bool last_chunk)
	{
		static Buffer<1024, 2> s_buf;
		
		
		if (!_g_huart)
			return;
		
		default_output_buffer(data, last_chunk, &s_buf, [](const char *buf, size_t len)
		{
			while (HAL_DMA_GetState(_g_huart->hdmatx) == HAL_DMA_STATE_BUSY);
			HAL_UART_Transmit_DMA(_g_huart, reinterpret_cast<const uint8_t *>(buf), len);
		});
	}
	#endif	// HAL_UART_MODULE_ENABLED
	
	#if __has_include("usbd_cdc_if.h")
	#include "usbd_cdc_if.h"
	
	extern "C" USBD_HandleTypeDef hUsbDeviceFS;
	
	inline void default_output_vcp(strv data, bool last_chunk)
	{
		static Buffer<CDC_DATA_FS_MAX_PACKET_SIZE> s_buf;
		
		default_output_buffer(data, last_chunk, &s_buf, [](const char *buf, uint16_t len)
		{
			static constexpr uint32_t TIMEOUT = 50;
			
			const uint32_t start = HAL_GetTick();
			
			while (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED
				&& CDC_Transmit_FS(const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(buf)), len) == USBD_BUSY
				&& HAL_GetTick() - start < TIMEOUT);
		});
	}
	#endif	// __has_include("usbd_cdc_if.h")
	
	template <size_t OUTPUT_COUNT = 1>
	class Logger
	{
	public:
		Level level;
		strv name;
		std::array<output_t, OUTPUT_COUNT> outputs;
		timestamp_t timestamp = default_timestamp;
		handler_t error_handler = nullptr, fatal_handler = nullptr;
		
		constexpr Logger(Level level, strv name) : level(level), name(name), outputs(std::array{default_output_stdout}) {}
		constexpr Logger(Level level, strv name, std::array<output_t, OUTPUT_COUNT> outputs) : level(level), name(name), outputs(outputs) {}
		
		constexpr Logger(Level level, strv name, timestamp_t timestamp) :
			level(level), name(name), outputs(std::array{default_output_stdout}), timestamp(timestamp) {}
		
		constexpr Logger(Level level, strv name, std::array<output_t, OUTPUT_COUNT> outputs, timestamp_t timestamp) :
			level(level), name(name), outputs(outputs), timestamp(timestamp) {}
		
		constexpr Logger(Level level, strv name, std::array<output_t, OUTPUT_COUNT> outputs, timestamp_t timestamp, handler_t error_handler, handler_t fatal_handler) :
			level(level), name(name), outputs(outputs), timestamp(timestamp), error_handler(error_handler), fatal_handler(fatal_handler) {}
		
		constexpr Logger Clone() const { return Logger(level, name, outputs, timestamp, error_handler, fatal_handler); }
		constexpr Logger Clone(strv name) const { return Logger(level, name, outputs, timestamp, error_handler, fatal_handler); }
		constexpr Logger Clone(Level level, strv name) const { return Logger(level, name, outputs, timestamp, error_handler, fatal_handler); }
		
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
				
				char flags[5 + 1], field_width[2 + 1], precision[3 + 1] = {}, modifier[2 + 1],		// precision must be initialized
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
								const size_t len = std::min(std::strlen(buf), size_t(precision));
								
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
					
					case 'b': case 'B':	// custom - modifiers same as o, x, X, and u
					{
						int fw = 0, prec = 1;
						
						if (has_field_width)
							fw = va_arg(args, int);
						else
							strv(field_width).to_num(fw);
						
						if (has_precision)
							prec = va_arg(args, int);
						else
							strv(precision + 1).to_num(prec);
						
						uintmax_t val;
						size_t size;
						
						if (strcmp(modifier, "l") == 0)
						{
							val = va_arg(args, unsigned long);
							size = sizeof(unsigned long);
						}
						else if (strcmp(modifier, "ll") == 0)
						{
							val = va_arg(args, unsigned long long);
							size = sizeof(unsigned long long);
						}
						else if (strcmp(modifier, "j") == 0)
						{
							val = va_arg(args, uintmax_t);
							size = sizeof(uintmax_t);
						}
						else if (strcmp(modifier, "z") == 0)
						{
							val = va_arg(args, size_t);
							size = sizeof(size_t);
						}
						else if (strcmp(modifier, "t") == 0)
						{
							val = va_arg(args, ptrdiff_t);		// should be unsigned
							size = sizeof(ptrdiff_t);
						}
						else
						{
							val = va_arg(args, unsigned int);
							
							if (strcmp(modifier, "h") == 0)
							{
								val = (unsigned short)(val);
								size = sizeof(unsigned short);
							}
							else if (strcmp(modifier, "hh") == 0)
							{
								val = (unsigned char)(val);
								size = sizeof(unsigned char);
							}
							else
								size = sizeof(unsigned int);
						}
						
						size *= 8;
						
						const bool alt = std::strchr(flags, '#'), left = std::strchr(flags, '-');
						const char pad = (!std::strchr(flags, '0') || left || precision[0]) ? ' ' : '0';
						
						bool found = false;
						
						size_t i = 0;
						for (uintmax_t b = uintmax_t(1) << (size - 1); b && n < sizeof(var) - 1; b >>= 1, ++i)
						{
							if (!found)
							{
								if ((val & b) || size - i <= prec)
								{
									found = true;
									
									if (alt)
									{
										var[n++] = '0';
										var[n++] = *p;
									}
									
									while (prec > size && n < sizeof(var) - 1 - size)
									{
										var[n++] = '0';
										--prec;
									}
								}
								else
									continue;
							}
							
							var[n++] = (val & b) ? '1' : '0';
						}
						
						if (!found && prec > 0)
							var[n++] = '0';
						
						if (n < fw)
						{
							if (left)
							{
								while (n < fw && n < sizeof(var) - 1)
									var[n++] = pad;
							}
							else
							{
								const size_t len = std::min(size_t(fw) - n, sizeof(var) - 1 - n);
								std::fill_n(var + n, len, pad);
								dispatch_chunk({var + n, len});
							}
						}
						
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
			
			if (level == Level::Error && error_handler)
				error_handler();
			
			if (level == Level::Fatal && fatal_handler)
				fatal_handler();
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
	#define	STM32T_DEFAULT_LOG_LEVEL			Level::None
	#endif
	
	#ifndef STM32T_DEFAULT_LOG_NAME
	#define	STM32T_DEFAULT_LOG_NAME				""sv
	#endif
	
	#ifndef STM32T_DEFAULT_LOG_OUTPUT
	#define	STM32T_DEFAULT_LOG_OUTPUT			std::array{default_output_stdout}
	#endif
	
	#ifndef STM32T_DEFAULT_LOG_TIMESTAMP
	#define	STM32T_DEFAULT_LOG_TIMESTAMP		default_timestamp
	#endif
	
	#ifndef STM32T_DEFAULT_LOG_ERROR_HANDLER
	#define	STM32T_DEFAULT_LOG_ERROR_HANDLER	nullptr
	#endif
	
	#ifndef STM32T_DEFAULT_LOG_FATAL_HANDLER
	#define	STM32T_DEFAULT_LOG_FATAL_HANDLER	&Error_Handler
	#endif
	
	inline constexpr Logger g_defaultLogger(STM32T_DEFAULT_LOG_LEVEL, STM32T_DEFAULT_LOG_NAME, STM32T_DEFAULT_LOG_OUTPUT, STM32T_DEFAULT_LOG_TIMESTAMP,
		STM32T_DEFAULT_LOG_ERROR_HANDLER, STM32T_DEFAULT_LOG_FATAL_HANDLER);
	
	template <auto& logger = g_defaultLogger>
	[[gnu::always_inline]]
	constexpr inline bool IsEnabled()
	{
		return logger.isEnabled();
	}
	
	template <auto& logger = g_defaultLogger>
	[[gnu::always_inline]]
	constexpr inline bool IsEnabled(const Level level)
	{
		return logger.isEnabled(level);
	}
	
	template <const Level level, auto& logger = g_defaultLogger, class... Args>
	[[gnu::always_inline]]
	void _LOG(const char *fmt, Args... args)
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
	void LOG_F(const char *fmt, Args... args) { _LOG<Level::Fatal, logger>(fmt, args...); }
	
	template <auto& logger = g_defaultLogger, class... Args>
	void LOG_E(const char *fmt, Args... args) { _LOG<Level::Error, logger>(fmt, args...); }
	
	template <auto& logger = g_defaultLogger, class... Args>
	void LOG_W(const char *fmt, Args... args) { _LOG<Level::Warning, logger>(fmt, args...); }
	
	template <auto& logger = g_defaultLogger, class... Args>
	void LOG_I(const char *fmt, Args... args) { _LOG<Level::Info, logger>(fmt, args...); }
	
	template <auto& logger = g_defaultLogger, class... Args>
	void LOG_D(const char *fmt, Args... args) { _LOG<Level::Debug, logger>(fmt, args...); }
	
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
			#ifdef RCC_CSR_BORRSTF
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

#if __has_include("RTE_Components.h")
namespace STM32T::Log
{
	#include "RTE_Components.h"
	
	#ifdef RTE_Compiler_IO_STDOUT_User
	
	inline output_t _g_stdout = nullptr;
	
	/**
	* @note The logger must not call fflush().
	*/
	inline void RedirectStdout(const output_t logger)
	{
		if (logger != default_output_stdout)
			_g_stdout = logger;
	}
	
	extern "C" [[gnu::used]] inline int stdout_putchar(int ch) { return ch; }
	
	#endif	// RTE_Compiler_IO_STDOUT_User
}
#endif	// __has_include("RTE_Componetns.h")
