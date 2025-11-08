#pragma once

#include "../Common.hpp"
#include "../strv.hpp"

#include <cstdint>
#include <cstdarg>
#include <cstdio>		// vsnprintf



namespace STM32T
{
	class ILCD
	{
	public:
		using coord_t = uint16_t;
		using coordx_t = coord_t;
		using coordy_t = coord_t;
		using line_t = uint16_t;
		
		virtual ILCD& NextLine(const line_t lines = 1) = 0;
		
		virtual ILCD& PutChar(const char ch, bool interpret_specials = true, bool auto_next_line = true) = 0;
		
		virtual void Test() {}
		
		ILCD& PutCharn(const char ch, size_t n, bool interpret_specials = true, bool auto_next_line = true)
		{
			while (n--)
				PutChar(ch, interpret_specials, auto_next_line);
			
			return *this;
		}
		
		ILCD& PutStr(const char* str)
		{
			while (*str)
				PutChar(*str++);
			
			return *this;
		}
		
		ILCD& PutStrn(const char* str, size_t n)
		{
			while (n--)
				PutChar(*str++);
			
			return *this;
		}
		
		ILCD& PutStr(const strv view)
		{
			PutStrn(view.data(), view.length());
			return *this;
		}
		
		template <size_t BUF_SIZE = 64>
		ILCD& PutStrf(const char* fmt, ...)
		{
			char buf[BUF_SIZE];
			
			va_list args;
			va_start(args, fmt);
			const int len = vsnprintf(buf, sizeof(buf), fmt, args);
			va_end(args);
			
			PutStrn(buf, std::min((size_t)len, BUF_SIZE - 1));
			
			return *this;
		}
		
		template <typename I, size_t BUF_SIZE = 32>
		ILCD& PutInt(const I i, const int base = 10, const size_t min_field_width = 0, const char filler = '0')
		{
			static_assert(std::is_integral_v<I>);
			
			char buf[BUF_SIZE];
			const std::to_chars_result result = std::to_chars(buf, buf + BUF_SIZE, i, base);
			
			if (result.ec != std::errc())
			{
				PutCharn('?', min_field_width);
				return *this;
			}
			
			const size_t len = result.ptr - buf;
			
			if (len < min_field_width)
				PutCharn(filler, min_field_width - len);
			
			PutStrn(buf, len);
			
			return *this;
		}
		
		template <typename F, size_t BUF_SIZE = 32>
		ILCD& PutFloat(const F f)
		{
			static_assert(std::is_floating_point_v<F>);
			
			char buf[BUF_SIZE];
			const std::to_chars_result result = std::to_chars(buf, buf + BUF_SIZE, f);
			
			if (result.ec != std::errc())
				PutChar('?');
			else
				PutStrn(buf, result.ptr - buf);
			
			return *this;
		}
	};
}
