
### Example of command function:
You need to provide the KS0108B object with a `command` function pointer to manipulate the data pins as well as the RS, RW, EN pins according to the timing specifications of KS0108B (page 8 of [the datasheet](https://www.sparkfun.com/datasheets/LCD/ks0108b.pdf)). Here is an examples of such function:

```C++
#include <Tools/GLCD/KS0108B.hpp>
#include <Tools/Timing.hpp>

void my_glcd_cmd(uint8_t data, const bool rw, const bool rs)
{
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, rs);
    HAL_GPIO_WritePin(LCD_RW_GPIO_Port, LCD_RW_Pin, rw);
    STM32T::DWT_Delay_ns(1750);			// (Increased) Address Set-Up Time
    HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, GPIO_PIN_SET);
    
    
    HAL_GPIO_WritePin(LCD_D0_GPIO_Port, LCD_D0_Pin, data & 0b0000'0001);
    HAL_GPIO_WritePin(LCD_D1_GPIO_Port, LCD_D1_Pin, data & 0b0000'0010);
    HAL_GPIO_WritePin(LCD_D2_GPIO_Port, LCD_D2_Pin, data & 0b0000'0100);
    HAL_GPIO_WritePin(LCD_D3_GPIO_Port, LCD_D3_Pin, data & 0b0000'1000);
    HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, data & 0b0001'0000);
    HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, data & 0b0010'0000);
    HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_65_Pin, data & 0b0100'0000);
    HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, data & 0b1000'0000);
    STM32T::DWT_Delay_ns(250);			// (Increased) Data Set-Up Time
    HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, GPIO_PIN_RESET);
    
    STM32T::DWT_Delay_ns(3000);			// (Increased) Address & Data Hold Time
}


int main()
{
    using namespace STM32T;

    KS0108B glcd(my_glcd_cmd, ...);
    glcd.PutStr("Hello, World!");
}

```