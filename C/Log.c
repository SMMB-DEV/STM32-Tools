#define STM32T_LOG
#include "./Log.h"

#include <stdarg.h>
#include <stdio.h>

void LOG(void (*f)())
{
	f();
}

void LOGA(const uint8_t * arr, size_t len)
{
	len++;
	while (len)
	{
		for (uint8_t i = 0; --len && i < 8; i++)
			printf("%02X ", *arr++);
		
		printf("\n");
	}
}

void LOGF(const char *fmt, ...)
{
	va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void LOGFI(const size_t indent, const char *fmt, ...)
{
	printf("%*s", indent * 4, "");
	
	va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

bool LOGFC(const bool c, const char *fmt, ...)
{
	if (c)
	{
		va_list args;
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
	}
	
	return c;
}

void LOGSEP()
{
	printf("--------------------------------------------------------------------------------\n");
}
