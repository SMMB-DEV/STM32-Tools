#pragma once

#include "main.h"

#include <stdbool.h>



#define FH_STDIN    0x8001
#define FH_STDOUT   0x8002
#define FH_STDERR   0x8003



#define STM32T_SYS_WRITE_GPIO(PORT, PIN, BAUD) \
extern int stdout_putchar(int ch) { return ch; } \
extern int _sys_write(int fh, const uint8_t *buf, uint32_t len, int mode) \
{ \
	static const uint16_t BIT_TIME = 48'000'000 / (BAUD) - 21; \
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
extern int stdout_putchar(int ch) { return ch; } \
extern int _sys_write(int fh, const uint8_t *buf, uint32_t len, int mode) \
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
extern int stdout_putchar(int ch) { return ch; } \
extern int _sys_write(int fh, const uint8_t *buf, uint32_t len, int mode) \
{ \
	if (fh != FH_STDOUT) \
		return fh == FH_STDERR ? 0 : -1; \
	\
	return HAL_UART_Transmit((PHUART), buf, len, HAL_MAX_DELAY) == HAL_OK ? 0 : -1; \
}



#define STM32T_SYS_WRITE_USB(connected) \
extern USBD_HandleTypeDef hUsbDeviceFS; \
extern int stdout_putchar(int ch) { return ch; } \
extern int _sys_write(int fh, const uint8_t *buf, uint32_t len, int mode) \
{ \
	if (fh != FH_STDOUT) \
		return fh == FH_STDERR ? 0 : -1; \
	\
	uint8_t res = USBD_OK; \
	uint32_t start = HAL_GetTick(); \
	while ((connected) && hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED && (res = CDC_Transmit_FS((uint8_t*)buf, len)) == USBD_BUSY && HAL_GetTick() - start < 100); \
	return 0;	/* If non-zero is returned once, it stops working. */ \
}



#ifdef STM32T_LOG
void LOG(void (*f)());
void LOGA(const uint8_t * arr, size_t len);
void LOGF(const char *fmt, ...);
void LOGFI(const size_t indent, const char *fmt, ...);
bool LOGFC(const bool c, const char *fmt, ...);
void LOGSEP();
#else
inline void LOG(void (*f)()) {}
inline void LOGA(const uint8_t * arr, size_t len) {}
inline void LOGF(const char *fmt, ...) {}
inline void LOGFI(const size_t indent, const char *fmt, ...) {}
inline bool LOGFC(const bool c, const char *fmt, ...) { return c; }
inline void LOGSEP() {}
#endif



#define LOGT(__msg__, __min_time__, ...) \
{ \
	if (_LOG) \
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
