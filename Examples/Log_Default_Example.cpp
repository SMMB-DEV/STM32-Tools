#define STM32T_DEFAULT_LOG_LEVEL	Level::Info		// Default logger with Info level and stdout output. Must be defined before including "Log.hpp".
#include <Tools/Log.hpp>

using namespace STM32T::Log::DefLog;

int main()
{
	LOG_I("Hello, %s!", "World");		// Uses the default logger
}

/**
* @example Log_Default_Example.cpp
* Default logger usage example:
*
* @attention It is strongly recommended to define the @ref default_log_macros "STM32T_DEFAULT_LOG_XXX" macros in the project's settings
* so that they are defined in every translation unit. Defining them in just one translation unit (like this example) could lead to different
* translation units having different default logger configurations without any errors or warnings.
*/