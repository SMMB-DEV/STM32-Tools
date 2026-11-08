#include <Tools/Log.hpp>

static STM32T::Log::Logger s_log(STM32T::Log::Level::Info, "MyModule"sv);		// Default output: stdout

int main()
{
	s_log.i("Hello, %s!", "World");			// Possible output: [       563][Info ][MyModule]: Hello, World!
}

/**
* @example Log_Dynamic_Example.cpp
* A dynamic logger is simply an object of type @ref STM32T::Log::Logger.
* The attributes of this logger (level, name, etc.) can be modified during runtime.
*/