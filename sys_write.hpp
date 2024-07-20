#define FH_STDIN    0x8001
#define FH_STDOUT   0x8002
#define FH_STDERR   0x8003



#define COM_SYS_WRITE_GPIO(PORT, PIN, TIM, BAUD) \
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



#define COM_SYS_WRITE_UART(PHUART) \
extern "C" int stdout_putchar(int ch) { return ch; } \
extern "C" int _sys_write(int fh, const uint8_t *buf, uint32_t len, int mode) \
{ \
	if (fh != FH_STDOUT) \
		return fh == FH_STDERR ? 0 : -1; \
	\
	return HAL_UART_Transmit((PHUART), buf, len, HAL_MAX_DELAY) == HAL_OK ? 0 : -1; \
}



#define COM_SYS_WRITE_USB() \
extern USBD_HandleTypeDef hUsbDeviceFS; \
extern "C" int stdout_putchar(int ch) { return ch; } \
extern "C" int _sys_write(int fh, const uint8_t *buf, uint32_t len, int mode) \
{ \
	if (fh != FH_STDOUT) \
		return fh == FH_STDERR ? 0 : -1; \
	\
	uint8_t res = USBD_FAIL; \
	uint32_t start = HAL_GetTick(); \
	while (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED && (res = CDC_Transmit_FS((uint8_t*)buf, len)) == USBD_BUSY && HAL_GetTick() - start < 100); \
	return -res; \
}
