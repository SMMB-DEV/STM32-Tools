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
		
		void PutStrn(const char* str, size_t n)
		{
			while (n--)
				PutChar(*str++);
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
		
		template <size_t BUF_SIZE = 32>
		void PutStrf(const char* fmt, ...)
		{
			char buf[BUF_SIZE];
			
			va_list args;
			va_start(args, fmt);
			const int len = vsnprintf(buf, sizeof(buf), fmt, args);
			va_end(args);
			
			PutStrn(buf, std::min((size_t)len, BUF_SIZE - 1));
		}
		
		template <typename T>
		void PutNum(const T t)
		{
			static_assert(std::is_scalar_v<T>);
			
			PutStrf(std::is_signed_v<T> ? "%d" : "%u", t);
		}
	};
}
