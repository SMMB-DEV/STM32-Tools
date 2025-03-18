# Using LCD objects

You need to provide the KS0108B/HD44780 object with a read/write function pointer to manipulate the data pins as well as the RS, RW, and EN pins according to their timing specifications. Here are two examples of such function:

### KS0108B

Apart from the R/W function, this object requires a function to set the CS pins of the LCD.

```C++
#include <Tools/LCD/KS0108B.hpp>
#include <Tools/Timing.hpp>

static STM32T::IOs<8> g_dataPins =
std::array<STM32T::IO, 8>{
    STM32T::IO{LCD_D0_GPIO_Port, LCD_D0_Pin},
    STM32T::IO{LCD_D1_GPIO_Port, LCD_D1_Pin},
    STM32T::IO{LCD_D2_GPIO_Port, LCD_D2_Pin},
    STM32T::IO{LCD_D3_GPIO_Port, LCD_D3_Pin},
    STM32T::IO{LCD_D4_GPIO_Port, LCD_D4_Pin},
    STM32T::IO{LCD_D5_GPIO_Port, LCD_D5_Pin},
    STM32T::IO{LCD_D6_GPIO_Port, LCD_D6_Pin},
    STM32T::IO{LCD_D7_GPIO_Port, LCD_D7_Pin}
};

uint8_t my_glcd_rw(const bool rw, const bool rs, uint8_t data)
{
    static uint32_t last_low_cyc = 0;
	
	static STM32T::IO en_pin(LCD_EN_GPIO_Port, LCD_EN_Pin), rw_pin(LCD_RW_GPIO_Port, LCD_RW_Pin), rs_pin(LCD_RS_GPIO_Port, LCD_RS_Pin);
	
	using namespace STM32T::Time;
	
	rw_pin.Set(rw);
	rs_pin.Set(rs);
	
	Delay_ns(std::max(140u, std::min(500u, 500u - CyclesTo_ns(GetCycle() - last_low_cyc))));	// ASU, WL
	en_pin.Set();
	
	if (!rw)
	{
		// write
		g_dataPins.Set(data);
		Delay_ns(500u);		// DSU, WH
	}
	else
	{
		// read
		Delay_ns(500);		// D, WH
		data = g_dataPins.Read();
	}
	
	en_pin.Reset();
	last_low_cyc = GetCycle();
	
	Delay_ns(10);		// DHW, AH
	
	if (!rw)
		g_dataPins.Set(0xFF);	// Release all pins
	
	return data;
}


int main()
{
    using namespace STM32T;
    
    for (uint8_t i = 0; i < g_dataPins.PinCount(); i++)
		g_dataPins[i].SetMode(GPIO_MODE_OUTPUT_OD, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH);
    
    KS0108B glcd(my_glcd_rw, [](uint8_t cs)     // Using lambda instead of function pointer
    {
        // Set CS pins (no timing required)
    }, 2);
    
    glcd.Init().PutStr("Hello, World!");
}

```

### HD44780

```C++
#include <Tools/LCD/HD44780.hpp>
#include <Tools/Timing.hpp>

static STM32T::IOs<4> g_dataPins =
std::array<STM32T::IO, 4>{
    STM32T::IO{LCD_D4_GPIO_Port, LCD_D4_Pin},
    STM32T::IO{LCD_D5_GPIO_Port, LCD_D5_Pin},
    STM32T::IO{LCD_D6_GPIO_Port, LCD_D6_Pin},
    STM32T::IO{LCD_D7_GPIO_Port, LCD_D7_Pin}
};

static HD44780 lcd([](const bool rw, const bool rs, uint8_t data)
{
    static uint32_t last_low_cyc = 0;
	
	static STM32T::IO en_pin(LCD_E_GPIO_Port, LCD_E_Pin), rw_pin(LCD_RW_GPIO_Port, LCD_RW_Pin), rs_pin(LCD_RS_GPIO_Port, LCD_RS_Pin);
	
	using namespace STM32T::Time;
	
	rw_pin.Set(rw);
	rs_pin.Set(rs);
	
	Delay_ns(std::max(40u, std::min(250u, 250u - CyclesTo_ns(GetCycle() - last_low_cyc))));	// AS, cycE - EH
	en_pin.Set();
	
	if (!rw)
	{
		// write
		g_dataPins.Set(data >> 4);
		Delay_ns(250u);		// DSW, EH
	}
	else
	{
		// read
		Delay_ns(250);		// DDR, EH
		data = g_dataPins.Read() << 4;
	}
	
	en_pin.Reset();
	last_low_cyc = GetCycle();
	
	Delay_ns(10);		// AH, H
	
	if (!rw)
		g_dataPins.Set(0xFF);	// Release all pins
	
	return data;
}, 2, 16, true);


int main()
{
    using namespace STM32T;
    
    for (uint8_t i = 0; i < g_dataPins.PinCount(); i++)
		g_dataPins[i].SetMode(GPIO_MODE_OUTPUT_OD, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH);
    
    lcd.Init().PutStr("Hello, World!");
}

```
