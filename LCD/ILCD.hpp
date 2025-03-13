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
		virtual void PutChar(const char ch, bool interpret_specials = true, bool auto_next_line = true) = 0;
		
		void PutStr(const char* str)
		{
			while (*str)
				PutChar(*str++);
		}
		
		void PutStrn(const char* str, uint16_t n)
		{
			for (uint16_t i = 0; i < n; i++)
				PutChar(str[i]);
		}
		
		[[deprecated("Use PutStr() instead.")]]
		void PutStrv(strv view)
		{
			PutStrn(view.data(), view.length());
		}
		
		void PutStr(strv view)
		{
			PutStrn(view.data(), view.length());
		}
		
		void PutStrf(const char* fmt, ...)
		{
			char buf[64];
			
			va_list args;
			va_start(args, fmt);
			vsnprintf(buf, sizeof(buf), fmt, args);
			va_end(args);
			
			PutStr(buf);
		}
		
		template <typename T>
		void PutNum(const T t)
		{
			static_assert(std::is_scalar_v<T>);
			
			PutStrf(std::is_signed_v<T> ? "%d" : "%u", t);
		}
	};
}
