#include <Tools/Display/HD44780.hpp>
#include <Tools/Core/Time.hpp>

static STM32T::IOs<4> g_dataPins =
std::array{
    STM32T::IO{LCD_D4_GPIO_Port, LCD_D4_Pin},
    STM32T::IO{LCD_D5_GPIO_Port, LCD_D5_Pin},
    STM32T::IO{LCD_D6_GPIO_Port, LCD_D6_Pin},
    STM32T::IO{LCD_D7_GPIO_Port, LCD_D7_Pin}
};

static STM32T::IO
	en_pin(LCD_E_GPIO_Port, LCD_E_Pin),
	rw_pin(LCD_RW_GPIO_Port, LCD_RW_Pin),
	rs_pin(LCD_RS_GPIO_Port, LCD_RS_Pin);

static HD44780 lcd(16, 2, [](const bool rw, const bool rs, uint8_t data)
{
	using namespace STM32T::Time;
	
    static cycle_t last_low_cyc = 0;
	
	rw_pin.Set(rw);
	rs_pin.Set(rs);
	
	Delay_ns(std::max(40u, std::min(250u, 250u - (uint32_t)CyclesTo_ns(GetCycle() - last_low_cyc))));	// AS, cycE - EH
	en_pin.Set();
	
	if (!rw)
	{
		// write
		g_dataPins.Set(data >> 4);
		Delay_ns(250u);			// DSW, EH
	}
	else
	{
		// read
		Delay_ns(250);			// DDR, EH
		data = g_dataPins.Read() << 4;
	}
	
	en_pin.Reset();
	last_low_cyc = GetCycle();
	
	Delay_ns(10);				// AH, H
	
	if (!rw)
		g_dataPins.Set(0xFF);	// Release all pins
	
	return data;
});


int main()
{
    using namespace STM32T;
    
    g_dataPins.SetMode(GPIO_MODE_OUTPUT_OD, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH);	// Pins need to be OD to read and write simultaneously
    
    lcd.Init().PutStr("Hello, World!");
}

/**
* @example{lineno} HD44780_Example.cpp
* Example usage of HD44780 class
*/