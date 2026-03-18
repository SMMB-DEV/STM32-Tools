# GSM Module

This module contains headers for using different GSM module (currently SIM800x, GL865, and M66).

## Macros

### `STM32T_IWDG_TIMEOUT`:

Can be defined (as a `uint32_t` value) before including any of this module's headers to refresh the IWDG during time consuming tasks.

### `STM32T_GSM_URC_SUPPORT`:

If defined and UART registered callbacks are enabled (`USE_HAL_UART_REGISTER_CALLBACKS == 1`),
the gsm instance can receive and store URCs sent by the GSM module to be handled later by calling `HandleURCs()`. Note that DMA for UART RX must be enabled to use this feature.

## Headers

- GSM.hpp: Contains the generic `GSM` base class. This class cannot be instantiated directly.
- GL865.hpp
- SIM800x.hpp
- M66.hpp

---

##### [Go Back](../README.md)
