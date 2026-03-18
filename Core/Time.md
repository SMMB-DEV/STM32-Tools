# Time.hpp

This file contains timing utilities and introduces the `STM32T::Time` namespace.

The `STM32T_TIME_CLK` macro must be defined before including this header and it must be a `constexpr` value.
You can define this in STM32CubeMX or Keil Options dialog to be equal to HCLK.

If your microcontroller has a DWT unit, it will be enabled in `Time::Init()` and used for timing. Otherwise, the SysTick timer is used.

The module also has functions for working with the conventional tick provided by HAL_GetTick().

### Features:

- Conversion between cycles and ms, μs, and ns.
- Delay functions
- Checking if a certain amount of time has passed since a specific cycle or tick.
- Add or subtract an amount of time to a pair of `RTC_DateTypeDef` and `RTC_TimeTypeDef`
- A `Filter` class to remove spurious changes in a boolean stream (e.g. the state of a pin)
