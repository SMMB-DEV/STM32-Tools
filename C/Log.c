#include "./Log.h"

#include <stdarg.h>
#include <stdio.h>

void LOG(void (*f)())
{
#ifndef STM32T_LOG
	f();
#endif
}

void LOGA(const uint8_t * arr, size_t len)
{
#ifndef STM32T_LOG
	len++;
	while (len)
	{
		for (uint8_t i = 0; --len && i < 8; i++)
			printf("%02X ", *arr++);
		
		printf("\n");
	}
#endif
}

void LOGF(const char *fmt, ...)
{
#ifndef STM32T_LOG
	va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#endif
}

void LOGFI(const size_t indent, const char *fmt, ...)
{
#ifndef STM32T_LOG
	printf("%*s", indent * 4, "");
	
	va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#endif
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
#ifndef STM32T_LOG
	printf("--------------------------------------------------------------------------------\n");
#endif
}
