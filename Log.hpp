#pragma once

extern "C"
{
#include "main.h"
}

#include <cstdio>



#define FH_STDIN    0x8001
#define FH_STDOUT   0x8002
#define FH_STDERR   0x8003



#define STM32T_SYS_WRITE_GPIO(PORT, PIN, TIM, BAUD) \
extern "C" int stdout_putchar(int ch) { return ch; } \
__attribute__((always_inline)) static void _delay_putchar(const uint32_t delay) { (TIM)->CNT = 0;	while ((TIM)->CNT < delay); } \
extern "C" int _sys_write(int fh, const uint8_t *buf, uint32_t len, int mode) \
{ \
	static constexpr uint16_t BIT_TIME = 48'000'000 / (BAUD) - 21; \
	\
	\
	if (fh != FH_STDOUT) \
		return fh == FH_STDERR ? 0 : -1; \
	\
	for (; len; len--) \
	{ \
		const uint8_t& ch = *buf++; \
		\
		/* Start bit */ \
		(PORT)->BRR = (PIN); \
		_delay_putchar(BIT_TIME); \
		\
		/* Data */ \
		for (uint8_t i = 1; i; i <<= 1) \
		{ \
			(PORT)->BSRR = (PIN) << (16 * ((ch & i) == 0)); \
			_delay_putchar(BIT_TIME); \
		} \
		\
		/* Stop bit */ \
		(PORT)->BSRR = (PIN); \
		_delay_putchar(BIT_TIME); \
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



#define STM32T_SYS_WRITE_USB(connected, init) \
extern "C" USBD_HandleTypeDef hUsbDeviceFS; \
extern "C" int stdout_putchar(int ch) { return ch; } \
extern "C" int _sys_write(int fh, const uint8_t *buf, uint32_t len, int mode) \
{ \
	if (fh != FH_STDOUT) \
		return fh == FH_STDERR ? 0 : -1; \
	\
	uint8_t res = USBD_FAIL; \
	uint32_t start = HAL_GetTick(); \
	while (connected && init && hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED && (res = CDC_Transmit_FS((uint8_t*)buf, len)) == USBD_BUSY && HAL_GetTick() - start < 100);\
	/*return -res;*/ \
	return 0;\
}







#ifndef STM32T_LOG
constexpr bool _LOG = false;
#else
constexpr bool _LOG = true;
#endif



inline void LOG(void (*f)())
{
	if constexpr (_LOG)
		f();
}

inline void LOGA(const uint8_t * arr, size_t len)
{
	if constexpr (_LOG)
	{
		len++;
		while (len)
		{
			for (uint8_t i = 0; --len && i < 8; i++)
				printf("%02X ", *arr++);
			
			printf("\n");
		}
	}
}

template <class... Args>
inline void LOGF(const char *fmt, Args... args)
{
	if constexpr (_LOG)
		printf(fmt, args...);
}

template <class... Args>
inline void LOGFI(const size_t indent, const char *fmt, Args... args)
{
	if constexpr (_LOG)
	{
		printf("%*s", indent * 4, "");
		printf(fmt, args...);
	}
}

template <class... Args>
inline bool LOGFC(const bool c, const char *fmt, Args... args)
{
	if (c)
		LOGF(fmt, args...);
	
	return c;
}

inline void LOGSEP()
{
	if constexpr (_LOG)
		printf("--------------------------------------------------------------------------------\n");
}



#define LOGT(__msg__, __min_time__, ...) \
{ \
	if constexpr (_LOG) \
	{ \
		volatile uint32_t __start__ = HAL_GetTick(); \
		__VA_ARGS__ \
		uint32_t __elapsed__ = HAL_GetTick() - __start__; \
		if (__elapsed__ >= __min_time__) \
			printf(__msg__ ": %ums\n", __elapsed__); \
	} \
	else \
	{ \
		__VA_ARGS__ \
	} \
}

namespace STM32T
{
	inline void Startup()
	{
		LOG([]()
		{
			printf("\n\n\n--------------------------------------------------------------------------------\nStart!\n");
		
			const uint32_t version = HAL_GetHalVersion();
			
			// major.minor.patch-rc
			printf("\nHAL v%hhu.%hhu.%hhu-rc%hhu\n", version >> 24, (version >> 16) & 0xFF, (version >> 8) & 0xFF, version & 0xFF);
			printf("RevID: 0x%X, DevID: 0x%X, UID: 0x%08X%08X%08X\n", HAL_GetREVID(), HAL_GetDEVID(), HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2());
			printf("HCLK: %.1f MHz\n", HAL_RCC_GetHCLKFreq() / 1'000'000.0f);
			printf("\n");
		});
	}
}
