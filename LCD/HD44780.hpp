#pragma once

#include "../Timing.hpp"
#include "./ILCD.hpp"



namespace STM32T
{
	class HD44780 : public ILCD
	{
		static constexpr uint16_t DEFAULT_TIMEOUT_US = 50;
		
		uint8_t (* const cf_rw)(bool rw, bool rs, uint8_t data);
		uint8_t m_addr = 0;
		const uint8_t c_colCount;
		const bool c_twoLines, c_4Bit;	// todo: Store the number of lines as well as c_twoLines.
		
		bool Write(const bool rs, const uint8_t data, const uint16_t timeout_us = DEFAULT_TIMEOUT_US)
		{
			cf_rw(0, rs, data);
			
			if (c_4Bit)
				cf_rw(0, rs, data << 4);
			
			const bool ok = BusyWait(timeout_us);
			
			if (rs && ok)
				m_addr = (m_addr + 1) & 0x7F;
			
			return ok;
		}
		
		uint8_t Read(const bool rs)
		{
			uint8_t data = cf_rw(1, rs, 0);
			
			if (c_4Bit)
				data |= cf_rw(1, rs, 0) >> 4;
			
			if (rs)
			{
				if (BusyWait(DEFAULT_TIMEOUT_US))
					m_addr = (m_addr - 1) & 0x7F;
			}
			
			return data;
		}
		
		bool BusyWait(uint16_t timeout_us)
		{
			uint32_t start = Time::GetCycle();
			while (Read(0) & 0b1000'0000)		// Busy Flag
			{
				if (Time::Elapsed_us(start, timeout_us))
					return false;
			}
			
			return true;
		}
		
		void SetAddress(uint8_t addr, bool force = false)
		{
			addr &= 0b0111'1111;
			if (force || m_addr != addr)
			{
				if (Write(0, 0b1000'0000 | addr))
					m_addr = addr;
			}
		}
		
		void SetCGAddress(uint8_t addr)
		{
			Write(0, 0b0100'0000 | addr);
		}
		
	public:
		/**
		* @param rw - Function to set DB7:0 (or DB7:4), RW and RS pins according to input arguments and return the data on the bus (DB7:0 or DB7:4) if necessary.
		*				The E pin must also be cycled according to the datasheet.
		* @param line_count - Determines the number of lines on the display (usually 2). Valid values: 1, 2, 4
		* @param _4_bit - Determines wether the display should work in 4-bit mode or 8-bit mode (usually 4-bit mode).
		*/
		HD44780(uint8_t (* const rw)(bool rw, bool rs, uint8_t data), const uint8_t line_count, const uint8_t col_count, bool _4_bit = true)
			: cf_rw(rw), c_twoLines(line_count != 1), c_colCount(col_count), c_4Bit(_4_bit) {}
		
		~HD44780() {}
		
		HD44780& Init()
		{
			Time::Init();
			Time::Delay_ms(15);			// For VCC; probably not necessary.
			cf_rw(0, 0, 0b0011'0000);
			Time::Delay_us(4100);
			cf_rw(0, 0, 0b0011'0000);
			Time::Delay_us(100);
			cf_rw(0, 0, 0b0011'0000);
			Time::Delay_us(100);		// Instead of BusyWait() because interface is stil in 8-bit mode.
			
			cf_rw(0, 0, 0b0010'0000 | (!c_4Bit << 4) | (c_twoLines << 3));	// 5x8 font by default
			Time::Delay_us(100);
			
			if (c_4Bit)
				Write(0, 0b0010'0000 | (c_twoLines << 3));
			
			Display(true, false, false);
			Clear();
			Write(0, 0b0000'0110);		// Entry mode set: Increment, No display shift
			
			return *this;
		}
		
		HD44780& Clear()
		{
			Write(0, 0b0000'0001, 1640);	// Display clear
			m_addr = 0;
			return *this;
		}
		
		void Display(const bool on, const bool cursor = false, const bool blink = false)
		{
			Write(0, 0b0000'1000 | (on << 2) | (cursor << 1) | blink);
		}
		
		HD44780& NextLine(const line_t lines = 1) override
		{
			// todo: implement this correctly (4 * 20)
			if (c_twoLines && lines % 2)
				SetAddress(m_addr >= 0x40 ? 0 : 0x40);
			
			return *this;
		}
		
		HD44780& XL(uint8_t x, uint8_t l)
		{
			if (c_twoLines)
				SetAddress(l * 0x40 + x % 0x40);
			else
				SetAddress(x);
			
			return *this;
		}
		
		HD44780& PutChar(const char ch, bool interpret_specials = true, bool auto_next_line = true) override
		{
			if (interpret_specials)
			{
				switch (ch)
				{
					case '\n':
					case '\r':
						return NextLine();
					
					/*case '\b':
					{
						const uint16_t s = m_line * m_screenLen + m_page * MAX_CURSOR + m_cursor - (FONT_WIDTH + 1);
						const uint8_t cursor = s % m_screenLen, line = s / m_screenLen;
						
						Goto(cursor, line);
						PutChar(' ');
						Goto(cursor, line);
						
						return;
					}*/
					
					case '\t':
					{
						PutChar(' ');
						PutChar(' ');
						
						return *this;
					}
					
					/*case 127:	//DEL
					{
						uint8_t cursor = m_page * MAX_CURSOR + m_cursor, line = m_line;
						PutChar(' ');
						Goto(cursor, line);
						
						return;
					}*/
				}
			}
			
			if (auto_next_line && c_twoLines && (m_addr % 0x40) >= c_colCount)
				NextLine();
			
			Write(1, ch);
			
			return *this;
		}
		
		/**
		* @param data - Must have at least 8 elements.
		*/
		HD44780& CreateChar(uint8_t addr, const uint8_t *data)
		{
			const uint8_t old_addr = m_addr;
			SetCGAddress((addr << 3) & 0b0011'1111);
			
			for (uint8_t i = 0; i < 8; i++)
				Write(1, data[i]);
			
			SetAddress(old_addr, true);
			
			return *this;
		}
		
		void Test() override
		{
			Clear();
			PutStr("123456789O123456""1234");
			HAL_Delay(2000);
			
			Clear();
			XL(15, 0);
			PutChar('A');
			XL(14, 0);
			PutChar('B');
			HAL_Delay(2000);
			
			CreateChar(1, (uint8_t []){1, 2, 4, 8, 16, 1, 2, 4});
			PutChar(1);
			HAL_Delay(2000);
			
			Clear();
			for (uint16_t i = 0; i < 256; i += 16)
			{
				HAL_Delay(1000);
				XL(0, 0);
				
				for (uint8_t j = 0; j < 16; j++)
					PutChar(i + j, false);
				
				PutInt(i);
			}
		}
	};
}
