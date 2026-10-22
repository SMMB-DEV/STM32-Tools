##### [Go Back](./README.md)

---

# Logging

Include "Log.hpp" to use the logging module. This module introduces the `Log`, `Stdout`, and `Stderr` namespaces.

You can use the `Logger` class to create a "static" or "dynamic" logger.
Each logger has a level, a name, a timestamp formatter, and one or more outputs (sinks). They also support printf-style formatting (+ `"b"`/`"B"` for binary formatting).

The output format of loggers is currently fixed:

```log
[Timestamp][Level][Module name (if present)]: Log message
```

## Dynamic logger

A dynamic logger is simply an object of type `Logger`. The attributes of this logger (level, name, etc.) can be modified during runtime.

```C++
#include <Tools/Log.hpp>

static STM32T::Log::Logger s_log(STM32T::Log::Level::Info, "MyModule"sv);		// Default output: stdout

int main()
{
	s_log.i("Hello, %s!", "World");			// Possible output: [       563][Info ][MyModule]: Hello, World!
}
```

## Static logger

A static logger is just a logger that is declared as `constexpr`.
This has the benefit of actually removing unused log code and compile time level checking resulting in a smaller code footprint but with the restriction of fixed logger attributes.
You can also use the `LOG_X` functions in `Log::DefLog` for a possibly smaller footprint.

```C++
#include <Tools/Log.hpp>

using namespace STM32T::Log::DefLog;

static constexpr STM32T::Log::Logger s_log(STM32T::Log::Level::Info, "MyModule"sv);		// Default output: stdout

int main()
{
	s_log.i("Hello, %s!", "World");			// Possible output: [       563][Info ][MyModule]: Hello, World!
	LOG_I<s_log>("Hello, %s!", "World");	// Possible output: [       564][Info ][MyModule]: Hello, World!
}
```

## Default logger

There is always a default static logger called `g_defaultLogger` which is disabled.
If you don't need multiple logging modules or simply want a main logging module, you can use the default logger.

You can use the `STM32T_DEFAULT_LOG_XXX` macros to change the attributes of the default logger
(including setting the level to something other than `None` which enables the logger).

```C++
#define STM32T_DEFAULT_LOG_LEVEL	Level::Info		// Default logger with Info level and stdout output. Must be defined before including "Log.hpp".
#include <Tools/Log.hpp>

using namespace STM32T::Log::DefLog;

int main()
{
	LOG_I("Hello, %s!", "World");		// Uses the default logger
}
```

## Default outputs

The outputs must be of type `STM32T::Log::output_t` aka `void (*)(strv data, bool last_chunk)`. The default outputs defined in STM32T::Log are:

+ default_output_stdout
+ default_output_vcp (For now only if USB_FS is enabled)
+ default_output_gpio (Emulates UART using GPIO. Use `SetGPIOConfig(...)` to define the pin.)
+ default_output_itm
+ default_output_uart, default_output_uart_dma (Use `SetUARTHandle(...)` to set the UART used.)

By default all loggers use `default_output_stdout` as their only sink.

You can of course use your own custom output (e.g. to log to an SD card).

## Redirecting stdout and stderr

You can redirect your logs to many different sinks including `stdout`. `stdout` and `stderr`
can also be independently redirected to multiple sinks (except themselves of course).
Use the `STM32T_SYS_WRITE` macro as well as the `AddHandler(...)` and `RemoveHandler(...)` functions in `STM32T::Stdout` and `STM32T::Stderr`.

Other libraries and frameworks might simply use `printf(...)` for logging. Or for example,
the `<cassert>` library writes an error message to `stderr` when an assert fails.
This way you can redirect logs and error messages from other libraries to the same (non-`stdout`) sinks as your normal logs.
Otherwise they will be handled separately by your default `stdout` handler.

---

##### [Go Back](./README.md)
