# STM32 Tools (v0.1.0)

STM32T is a framework of useful utilities for working with STM32 microcontrollers. It is written in C++17 with only Keil (MDK-ARM) projects in mind.
It might not be usable with STM32CubeIDE or other IDEs. It relies on some HAL functions and typedefs.

## Modules

- [Core](./Core/README.md): Core utilities that aren't dependent on other modules or header files in the framework
- [Display](./Display/README.md): Headers for using different types of display
- [GSM](./GSM/README.md): Headers for using GSM modules
- [1-Wire](./1-Wire/README.md): For working with 1-wire sensors
- Memory: For working with memory ICs
- RFID: For working with RFID ICs and modules

## Other Headers

- [Log.hpp](./Log.md)
- [Versioning.hpp](./Versioning.md)
- Error Checking.hpp
- IO.hpp
- Runnable.hpp
