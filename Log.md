# Logging

To use the logging module you need to include "Log.h". Then you can use the `Logger` class to create a "static" or "dynamic" logger;

Each logger has a level, a name and one or more outputs (sinks). They also support printf-style formatting.

## Dynamic logger

A dynamic logger is simply an object of type `Logger`. The attributes of this logger (level, name, etc.) can be modified during runtime.

```C++
#include <Tools/Log.hpp>

static STM32T::Log::Logger s_log(STM32T::Log::Level::Info, "MyModule"sv);		// Default output: stdout

int main()
{
	s_log.i("Hello, %s!\n", "World");
}
```

## Static logger

A static logger is just a normal logger that is declared as `constexpr`.
This has the benefit of actually removing unused log code and compile time level checking resulting in a smaller code footprint but with the restriction of fixed logger attributes.
You can also use the LOG_X functions for a slightly smaller footprint.

```C++
#include <Tools/Log.hpp>

static constexpr STM32T::Log::Logger s_log(STM32T::Log::Level::Info, "MyModule"sv);		// Default output: stdout

int main()
{
	s_log.i("Hello, %s!\n", "World");
	LOG_I<s_log>("Hello, %s!\n", "World");
}
```

## Default logger

There is always a default static logger called `g_defaultLogger` which is disabled.
If you don't need multiple logging modules or simply want a main logging module, you can use the default logger.

Using one of the STM32T_DEFAULT_LOG_XXX macros enables the default logger. You can access this logger through the STM32T::Log namespace or by simply using the LOG_X functions.

```C++
#define STM32T_DEFAULT_LOG_INFO		// Default logger with Info level and stdout output. Must be defined before including "Log.hpp".
#include <Tools/Log.hpp>

int main()
{
	LOG_I("Hello, %s!\n", "World");		// Uses the default logger
}
```

If you need to modify the default logger (e.g. setting multiple outputs), you can define the STM32T_DEFAULT_LOG macro instead.

```C++
#define STM32T_DEFAULT_LOG	(Level::Info, "Main"sv, std::array<output_t, 2>{default_output_stdout, [](const char *buf, size_t len) \
{ \
	/* Custom logging code */ \
}})		// Parenthesis are necessary. Must be defined before including "Log.hpp".
#include <Tools/Log.hpp>

int main()
{
	LOG_I("Hello, %s!\n", "World");		// Uses the default logger
}
```

The reason that this macro must be defined in such a specific way is that it is used directly inside the STM32T::Log namespace to initialize the default logger.

```C++
namespace STM32::Log
{
	inline constexpr Logger g_defaultLogger STM32T_DEFAULT_LOG;
}
```

## Default outputs

The outputs must be of type `STM32T::Log::output_t` aka `void (*)(const char *, size_t)`. By default all loggers use `stdout` as their only sink. The default loggers defined in STM32T::Log are:

+ default_output_stdout
+ default_output_vcp (for now only if USB_FS is enabled)

You can of course use your own custom output (e.g. to log to an SD card).
