#include <Tools/Log.hpp>

using namespace STM32T::Log;

static constexpr Logger s_log(Level::Info, "MyModule"sv);		// Default output: stdout

int main()
{
	s_log.i("Hello, %s!", "World");			// Possible output: [       563][Info ][MyModule]: Hello, World!
	LOG_I<s_log>("Hello, %s!", "World");	// Possible output: [       564][Info ][MyModule]: Hello, World!
}

/**
* @example Log_Static_Example.cpp
* A static logger is just a logger that is declared as `constexpr`.
* This has the benefit of actually removing unused log code and compile time level checking
* resulting in a smaller code footprint but with the restriction of fixed logger attributes.
* You can also use the `LOG_X` functions in @ref STM32T::Log::DefLog for a possibly smaller footprint.
*/