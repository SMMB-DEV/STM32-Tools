#include "./KS0108B.hpp"
#include "../Core/Time.hpp"

#include <algorithm>



namespace STM32T
{
	static constexpr uint8_t bit_filter_table[8] = { 0b0000'0001, 0b0000'0011, 0b0000'0111, 0b0000'1111, 0b0001'1111, 0b0011'1111, 0b0111'1111, 0b1111'1111 };

	static constexpr uint8_t Font[KS0108B::MAX_CHARS][KS0108B::FONT_LENGTH] =
	{
		0, 0, 0, 0, 0,					/*0*/
		0, 0, 0, 0, 0,					/*1*/
		0, 0, 0, 0, 0,					/*2*/
		0, 0, 0, 0, 0,					/*3*/
		0, 0, 0, 0, 0,					/*4*/
		0, 0, 0, 0, 0,					/*5*/
		0, 0, 0, 0, 0,					/*6*/
		0, 0, 0, 0, 0,					/*7*/
		0, 0, 0, 0, 0,					/*8*/
		0, 0, 0, 0, 0,					/*9*/
		0, 0, 0, 0, 0,					/*10*/
		0, 0, 0, 0, 0,					/*11*/
		0, 0, 0, 0, 0,					/*12*/
		0, 0, 0, 0, 0,					/*13*/
		0, 0, 0, 0, 0,					/*14*/
		0, 0, 0, 0, 0,					/*15*/
		0, 0, 0, 0, 0,					/*16*/
		0, 0, 0, 0, 0,					/*17*/
		0, 0, 0, 0, 0,					/*18*/
		0, 0, 0, 0, 0,					/*19*/
		0, 0, 0, 0, 0,					/*20*/
		0, 0, 0, 0, 0,					/*21*/
		0, 0, 0, 0, 0,					/*22*/
		0, 0, 0, 0, 0,					/*23*/
		0, 0, 0, 0, 0,					/*24*/
		0, 0, 0, 0, 0,					/*25*/
		0, 0, 0, 0, 0,					/*26*/
		0, 0, 0, 0, 0,					/*27*/
		0, 0, 0, 0, 0,					/*28*/
		0, 0, 0, 0, 0,					/*29*/
		0, 0, 0, 0, 0,					/*30*/
		0, 0, 0, 0, 0,					/*31*/
		0, 0, 0, 0, 0,					/*32*/
		0x00, 0x00, 0x5F, 0x00, 0x00,	/*!*/
		0x00, 0x07, 0x00, 0x07, 0x00,	/*"*/
		0x14, 0x7F, 0x14, 0x7F, 0x14,	/*#*/
		0x24, 0x2A, 0x7F, 0x2A, 0x12,	/*$*/
		0x23, 0x13, 0x08, 0x64, 0x62,	/*%*/
		0x36, 0x49, 0x55, 0x22, 0x50,	/*&*/
		0x00, 0x05, 0x03, 0x00, 0x00,	/*'*/
		0x00, 0x1C, 0x22, 0x41, 0x00,	/*(*/
		0x00, 0x41, 0x22, 0x1C, 0x00,	/*)*/
		0x08, 0x2A, 0x1C, 0x2A, 0x08,	/***/
		0x08, 0x08, 0x3E, 0x08, 0x08,	/*+*/
		0x00, 0x50, 0x30, 0x00, 0x00,	/*,*/
		0x08, 0x08, 0x08, 0x08, 0x08,	/*-*/
		0x00, 0x30, 0x30, 0x00, 0x00,	/*.*/
		0x20, 0x10, 0x08, 0x04, 0x02,	/*/*/
		0x3E, 0x51, 0x49, 0x45, 0x3E,	/*0*/
		0x00, 0x42, 0x7F, 0x40, 0x00,	/*1*/
		0x42, 0x61, 0x51, 0x49, 0x46,	/*2*/
		0x21, 0x41, 0x45, 0x4B, 0x31,	/*3*/
		0x18, 0x14, 0x12, 0x7F, 0x10,	/*4*/
		0x27, 0x45, 0x45, 0x45, 0x39,	/*5*/
		0x3C, 0x4A, 0x49, 0x49, 0x30,	/*6*/
		0x01, 0x71, 0x09, 0x05, 0x03,	/*7*/
		0x36, 0x49, 0x49, 0x49, 0x36,	/*8*/
		0x06, 0x49, 0x49, 0x29, 0x1E,	/*9*/
		0x00, 0x36, 0x36, 0x00, 0x00,	/*:*/
		0x00, 0x56, 0x36, 0x00, 0x00,	/*;*/
		0x00, 0x08, 0x14, 0x22, 0x41,	/*<*/
		0x14, 0x14, 0x14, 0x14, 0x14,	/*=*/
		0x41, 0x22, 0x14, 0x08, 0x00,	/*>*/
		0x02, 0x01, 0x51, 0x09, 0x06,	/*?*/
		0x32, 0x49, 0x79, 0x41, 0x3E,	/*@*/
		0x7E, 0x11, 0x11, 0x11, 0x7E,	/*A*/
		0x7F, 0x49, 0x49, 0x49, 0x36,	/*B*/
		0x3E, 0x41, 0x41, 0x41, 0x22,	/*C*/
		0x7F, 0x41, 0x41, 0x22, 0x1C,	/*D*/
		0x7F, 0x49, 0x49, 0x49, 0x41,	/*E*/
		0x7F, 0x09, 0x09, 0x01, 0x01,	/*F*/
		0x3E, 0x41, 0x41, 0x51, 0x32,	/*G*/
		0x7F, 0x08, 0x08, 0x08, 0x7F,	/*H*/
		0x00, 0x41, 0x7F, 0x41, 0x00,	/*I*/
		0x20, 0x40, 0x41, 0x3F, 0x01,	/*J*/
		0x7F, 0x08, 0x14, 0x22, 0x41,	/*K*/
		0x7F, 0x40, 0x40, 0x40, 0x40,	/*L*/
		0x7F, 0x02, 0x04, 0x02, 0x7F,	/*M*/
		0x7F, 0x04, 0x08, 0x10, 0x7F,	/*N*/
		0x3E, 0x41, 0x41, 0x41, 0x3E,	/*O*/
		0x7F, 0x09, 0x09, 0x09, 0x06,	/*P*/
		0x3E, 0x41, 0x51, 0x21, 0x5E,	/*Q*/
		0x7F, 0x09, 0x19, 0x29, 0x46,	/*R*/
		0x46, 0x49, 0x49, 0x49, 0x31,	/*S*/
		0x01, 0x01, 0x7F, 0x01, 0x01,	/*T*/
		0x3F, 0x40, 0x40, 0x40, 0x3F,	/*U*/
		0x1F, 0x20, 0x40, 0x20, 0x1F,	/*V*/
		0x7F, 0x20, 0x18, 0x20, 0x7F,	/*W*/
		0x63, 0x14, 0x08, 0x14, 0x63,	/*X*/
		0x03, 0x04, 0x78, 0x04, 0x03,	/*Y*/
		0x61, 0x51, 0x49, 0x45, 0x43,	/*Z*/
		0x00, 0x00, 0x7F, 0x41, 0x41,	/*[*/
		0x02, 0x04, 0x08, 0x10, 0x20,	/*\*/
		0x41, 0x41, 0x7F, 0x00, 0x00,	/*]*/
		0x04, 0x02, 0x01, 0x02, 0x04,	/*^*/
		0x40, 0x40, 0x40, 0x40, 0x40,	/*_*/
		0x00, 0x01, 0x02, 0x04, 0x00,	/*`*/
		0x20, 0x54, 0x54, 0x54, 0x78,	/*a*/
		0x7F, 0x48, 0x44, 0x44, 0x38,	/*b*/
		0x38, 0x44, 0x44, 0x44, 0x20,	/*c*/
		0x38, 0x44, 0x44, 0x48, 0x7F,	/*d*/
		0x38, 0x54, 0x54, 0x54, 0x18,	/*e*/
		0x08, 0x7E, 0x09, 0x01, 0x02,	/*f*/
		//0x00, 0x4C, 0x52, 0x52, 0x3E,	/*g*/
		0x08, 0x14, 0x54, 0x54, 0x3C,	/*g*/
		0x7F, 0x08, 0x04, 0x04, 0x78,	/*h*/
		0x00, 0x44, 0x7D, 0x40, 0x00,	/*i*/
		0x20, 0x40, 0x44, 0x3D, 0x00,	/*j*/
		0x00, 0x7F, 0x10, 0x28, 0x44,	/*k*/
		0x00, 0x41, 0x7F, 0x40, 0x00,	/*l*/
		0x7C, 0x04, 0x18, 0x04, 0x78,	/*m*/
		0x7C, 0x08, 0x04, 0x04, 0x78,	/*n*/
		0x38, 0x44, 0x44, 0x44, 0x38,	/*o*/
		0x7C, 0x14, 0x14, 0x14, 0x08,	/*p*/
		0x08, 0x14, 0x14, 0x18, 0x7C,	/*q*/
		0x7C, 0x08, 0x04, 0x04, 0x08,	/*r*/
		0x48, 0x54, 0x54, 0x54, 0x20,	/*s*/
		0x04, 0x3F, 0x44, 0x40, 0x20,	/*t*/
		0x3C, 0x40, 0x40, 0x20, 0x7C,	/*u*/
		0x1C, 0x20, 0x40, 0x20, 0x1C,	/*v*/
		0x3C, 0x40, 0x30, 0x40, 0x3C,	/*w*/
		0x44, 0x28, 0x10, 0x28, 0x44,	/*x*/
		0x0C, 0x50, 0x50, 0x50, 0x3C,	/*y*/
		0x44, 0x64, 0x54, 0x4C, 0x44,	/*z*/
		0x00, 0x08, 0x36, 0x41, 0x00,	/*{*/
		0x00, 0x00, 0x7F, 0x00, 0x00,	/*|*/
		0x00, 0x41, 0x36, 0x08, 0x00,	/*}*/
		0, 0, 0, 0, 0,					/*~*/
		0, 0, 0, 0, 0,					/*DEL*/
		//0x04, 0x02, 0x7F, 0x02, 0x04,	/*up*/
		//0x10, 0x20, 0x7F, 0x20, 0x10	/*down*/
	};



	template <class T>
	void sort(T& a, T& b)
	{
		if (a <= b)
			return;
		
		std::swap(a, b);
	}

	template <class T>
	T abs(T a)
	{
		if (a < 0)
			return a*-1;
		
		return a;
	}

	template <typename T>
	T sgn(T val)
	{
		return (T(0) < val) - (val < T(0));
	}





	Char::Char()
	{
		statecount=1;
		width=0;
		datah=0;
		datal=0;
		scset=0;
		chset=0;
	}

	Char::~Char()
	{
		if (scset)
		{
			delete[] datah;
			delete[] datal;
			delete[] width;
			delete[] chset;
		}
	}

	void Char::SetStateCount(uint8_t s)
	{
		if (s>0 && s<=4 && !scset)
		{
			statecount=s;
			datah=new const uint8_t*[s];
			datal=new const uint8_t*[s];
			width=new uint8_t[s];
			chset=new bool[s];
			
			for (uint8_t i=0;i<s;i++)
			{
				chset[i]=0;
			}
			scset=1;
		}
	}

	void Char::Set(const uint8_t *d1, const uint8_t *d2, State s, uint8_t wid)
	{
		if(scset && s<statecount && wid!=0)
		{
			if (!chset[s])
			{
				datah[s]=d1;
				datal[s]=d2;
				width[s]=wid;
				chset[s]=1;
			}
		}
	}

	uint8_t Char::getwidth(State s)
	{
		if (scset)
		{
			if (chset[s%statecount])
			{
				return width[s%statecount];
			}
		}
		return 0;
	}

	const uint8_t* Char::getdh(State s)
	{
		if (scset)
		{
			if (chset[s%statecount])
			{
				return datah[s%statecount];
			}
		}
		return 0;
	}

	const uint8_t* Char::getdl(State s)
	{
		if (scset)
		{
			if (chset[s%statecount])
			{
				return datal[s%statecount];
			}
		}
		return 0;
	}







	bool KS0108B::BusyWait(const uint16_t timeout_us) const
	{
		const uint32_t start = Time::GetCycle();
		while (f_rw(1, 0, 0) & 0b1000'0000)		// Busy Flag
		{
			if (Time::Elapsed_us(start, timeout_us))
				return false;
		}
		
		return true;
	}
	
	bool KS0108B::Instruction(const uint8_t ins) const
	{
		f_rw(0, 0, ins);
		return BusyWait();
	}
	
	bool KS0108B::DataWrite(const uint8_t data) const
	{
		f_rw(0, 1, data);
		return BusyWait();
	}
	
	bool KS0108B::SetPage(uint8_t page, const bool force)
	{
		page %= m_pageCount;
		if (!force && m_page == page)
			return false;
		
		m_page = page;
		f_setCS(1 << m_page);
		
		return true;
	}

	bool KS0108B::SetLine(uint8_t line, const bool force)
	{
		line %= MAX_LINES;
		if (!force && m_line == line)
			return false;
		
		m_line = line;
		Instruction(m_line | 0b1011'1000);
		
		return true;
	}

	bool KS0108B::SetCursor(uint8_t cursor, const bool force)
	{
		cursor %= MAX_CURSOR;
		if (!force && m_cursor == cursor)
			return false;
		
		m_cursor = cursor;
		Instruction(m_cursor | 0b0100'0000);
		
		return true;
	}

	void KS0108B::Display(const bool display) const
	{
		Instruction(0b0011'1110 | display);
	}

	void KS0108B::Goto(const uint8_t cursor, const uint8_t line, const bool force)
	{
		if (SetPage(cursor / MAX_CURSOR, force))
		{
			SetCursor(cursor, true);
			SetLine(line, true);
			return;
		}
		
		SetCursor(cursor, force);
		SetLine(line, force);
	}

	void KS0108B::CheckCursor(const uint8_t lines)
	{
		if (++m_cursor >= MAX_CURSOR)
		{
			SetPage(m_page + 1, true);
			SetCursor(m_cursor, true);
			SetLine(m_line + (m_page == 0) * lines, true);
		}
	}
	
	void KS0108B::Write(uint8_t byte, const bool check, const uint8_t lines)
	{
		if (negate)
			byte = ~byte;
		
		m_screenMap[MapIndex()] = byte;
		DataWrite(byte);
		
		if (check)
			CheckCursor(lines);
	}

	void KS0108B::WriteTop(uint8_t byte, const uint8_t shift, const bool check, const uint8_t lines)
	{
		if (negate)
			byte = ~byte;
		
		const uint16_t index = MapIndex();
		m_screenMap[index] = (m_screenMap[index] & (0xFF >> (PIXELS_PER_LINE - shift))) | (byte << shift);
		DataWrite(m_screenMap[index]);
		
		if (check)
			CheckCursor(lines);
	}

	void KS0108B::WriteBottom(uint8_t byte, const uint8_t shift, const bool check, const uint8_t lines)
	{
		if (negate)
			byte = ~byte;
		
		const uint16_t index = MapIndex();
		m_screenMap[index] = (m_screenMap[index] & (0xFF << shift)) | (byte >> (PIXELS_PER_LINE - shift));
		DataWrite(m_screenMap[index]);
		
		if (check)
			CheckCursor(lines);
	}



	void KS0108B::Line_y(const uint8_t x, const uint8_t y, const uint8_t len, const bool draw)
	{
		//assuming x and y are checked before calling this function
		const uint8_t lineBegin = y / PIXELS_PER_LINE, lineEnd = (y + len) / PIXELS_PER_LINE, rowBegin = y % PIXELS_PER_LINE, rowEnd = (y + len) % PIXELS_PER_LINE;
		
		Goto(x, lineBegin);
		
		if (lineBegin == lineEnd)
		{
			if (draw)
				Write((bit_filter_table[rowEnd - rowBegin] << rowBegin) | m_screenMap[MapIndex(lineBegin, x)]);
			else
				Write(~(bit_filter_table[rowEnd - rowBegin] << rowBegin) & m_screenMap[MapIndex(lineBegin, x)]);
		}
		else
		{
			Write(draw ? ((0xFF << rowBegin) | m_screenMap[MapIndex(lineBegin, x)]) : ((0xFF >> (PIXELS_PER_LINE - rowBegin)) & m_screenMap[MapIndex(lineBegin, x)]), false);
			
			const uint8_t _write = draw * 0xFF;
			for (uint8_t line = lineBegin + 1; line < lineEnd; line++)
			{
				SetLine(line);
				SetCursor(x, true);
				Write(_write, false);
			}
			
			SetLine(lineEnd);
			SetCursor(x, true);
			Write(draw ? (bit_filter_table[rowEnd] | m_screenMap[MapIndex(lineEnd, x)]) : (~bit_filter_table[rowEnd] & m_screenMap[MapIndex(lineEnd, x)]), false);
			
			CheckCursor();
		}
	}

	void KS0108B::Line_x(const uint8_t x, const uint8_t y, const uint8_t len, const bool draw)
	{
		//assuming x and y are checked before calling this function
		const uint8_t &begin = x, end = x + len, line = y / PIXELS_PER_LINE;
		uint8_t _write = 1 << (y % PIXELS_PER_LINE);
		
		Goto(x, line);
		
		if (draw)
		{
			for (uint8_t i = begin; i <= end; i++)
				Write(_write | m_screenMap[MapIndex(line, i)]);
		}
		else
		{
			_write = ~_write;
			
			for (uint8_t i = begin; i <= end; i++)
				Write(_write & m_screenMap[MapIndex(line, i)]);
		}
	}







	KS0108B::KS0108B(uint8_t (* const rw)(bool rw, bool rs, uint8_t data), void (*set_cs)(uint8_t), const uint8_t page_count) :
			f_rw(rw), f_setCS(set_cs), m_pageCount(std::min((size_t)page_count, 8u)), screenLen(m_pageCount * MAX_CURSOR),
			m_screenMap(new uint8_t[MAX_LINES * screenLen]) {}
	
	KS0108B::~KS0108B() { delete[] m_screenMap; }


	void KS0108B::Test()
	{
		Clear(true);
		HAL_Delay(1000);
		Clear();
		HAL_Delay(1000);
		
		XY(100, 4);
		WriteByte(0xFF);
		WriteByte(0xAA);
		WriteByte(0x55);
		WriteByte(0x00);
		
		Line(60, 40, 70, 25);
		Rectangle(60, 45, 90, 55);
		FillRectangle(80, 25, 90, 40);
		Circle(110, 30, 8);
		FillCircle(110, 52, 10);
		
		FillRectangle(60, 10, 90, 20);
		Clear(65, 15, 85, 15);
		
		XY(0, 4);
		PutStr("Normal");
		NextLine(2);
		PutStrBig("Big");
		
		while (1)
		{
			HAL_Delay(1000);
			NegateRectangle(0, 4, 6 * 6, 4 + 7);
		}
		
		// todo: add tests for bitmap functions
	}

	void KS0108B::Init()
	{
		Time::Init();
		
		f_setCS(0xFF);
		
		Display(1);
		Clear();
	}

	KS0108B& KS0108B::Clear(const bool fill)
	{
		f_setCS(0xFF);
		
		const uint8_t _write = 0xFF * fill;
		
		SetCursor(0);
		for (uint8_t i = 0; i < MAX_LINES; i++)
		{
			SetLine(i);
			
			for (uint8_t j = 0; j < MAX_CURSOR; j++)
				DataWrite(_write);
		}
		
		memset(m_screenMap, _write, MAX_LINES * screenLen);
		
		SetPage(0, true);
		SetCursor(0);
		SetLine(0);
		m_row = 0;
		
		return *this;
	}
	
	KS0108B& KS0108B::Clear(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, const bool fill)
	{
		FillRectangle(x1, y1, x2, y2, fill);	// todo: optimize
		return *this;
	}

	KS0108B& KS0108B::XY(const uint8_t x, const uint8_t y)
	{
		m_row = y % PIXELS_PER_LINE;
		Goto(x, y / PIXELS_PER_LINE);
		return *this;
	}
	
	KS0108B& KS0108B::XL(const uint8_t x, const uint8_t line)
	{
		m_row = 0;
		Goto(x, line);
		return *this;
	}
	
	KS0108B& KS0108B::Move(const int16_t x, const int8_t y)
	{
		static constexpr uint8_t MAX_PIXELS = PIXELS_PER_LINE * MAX_LINES;
		
		int32_t _x = (m_page * MAX_CURSOR + m_cursor + x) % screenLen, _y = (m_line * PIXELS_PER_LINE + m_row + y) % MAX_PIXELS;
		
		if (_x < 0)
			_x += (-_x / screenLen + 1) * screenLen;
		
		if (_y < 0)
			_y = (-_y / MAX_PIXELS + 1) * MAX_PIXELS;
		
		XY(_x, _y);
		return *this;
	}

	KS0108B& KS0108B::WriteByte(const uint8_t byte, const uint8_t repeat)
	{
		if (m_row == 0)
			for (uint8_t i = 0; i < repeat; i++)
				Write(byte);
		else
		{
			const uint8_t line = m_line, cursor = LongCursor();
			
			// todo: Do all the T & B writes in one page, then move to the next page.
			
			//Bottom part
			SetLine(line + 1);
			for (uint8_t i = 0; i < repeat; i++)
				WriteBottom(byte, m_row, i < repeat - 1);	// Don't check for the last byte
			
			//Top part
			Goto(cursor, line, true);
			for (uint8_t i = 0; i < repeat; i++)
				WriteTop(byte, m_row);
		}
		
		return *this;
	}
	
	KS0108B& KS0108B::WriteByteArr(const uint8_t *bytes, size_t count)
	{
		if (m_row == 0)
			for (size_t i = 0; i < count; i++)
				Write(bytes[i]);
		else
		{
			const uint8_t line = m_line, cursor = LongCursor();
			
			// todo: Do all the T & B writes in one page, then move to the next page.
			
			//Bottom part
			SetLine(line + 1);
			for (size_t i = 0; i < count; i++)
				WriteBottom(bytes[i], m_row, i < count - 1);	// Don't check for the last byte
			
			//Top part
			Goto(cursor, line, true);
			for (size_t i = 0; i < count; i++)
				WriteTop(bytes[i], m_row);
		}
		
		return *this;
	}

	KS0108B& KS0108B::NextLine(const line_t lines)
	{
		bool force = SetPage(0);
		SetCursor(0, force);
		SetLine(m_line + lines);
		
		return *this;
	}

	void KS0108B::Pixel(uint8_t x, uint8_t y, bool fill)
	{
		if (x >= screenLen || y >= SCREEN_WIDTH)
			return;
		
		uint8_t row = y % PIXELS_PER_LINE;
		y /= PIXELS_PER_LINE;
		
		Goto(x, y);
		
		Write(fill ? (m_screenMap[MapIndex(y, x)] | (1 << row)) : (m_screenMap[MapIndex(y, x)] & (~(1 << row))));
	}

	KS0108B& KS0108B::PutChar(const char ch, bool interpret_specials, bool auto_next_line)
	{
		//todo: space between characters
		
		if (ch >= MAX_CHARS)
			return *this;
		
		if (interpret_specials)
		{
			switch (ch)
			{
				case '\n':
				case '\r':
					return NextLine();
				
				case '\b':
				{
					const uint16_t s = Fa ? m_line * screenLen + m_cursor + 6 : m_line * screenLen + m_page * MAX_CURSOR + m_cursor - (FONT_LENGTH + 1);
					const uint8_t cursor = s % screenLen, line = s / screenLen;
					
					Goto(cursor, line);
					PutChar(' ');
					Goto(cursor, line);
					
					return *this;
				}
				
				case '\t':
				{
					PutChar(' ');
					PutChar(' ');
					
					return *this;
				}
				
				case 127:	//DEL
				{
					uint8_t cursor = m_page * MAX_CURSOR + m_cursor, line = m_line;
					PutChar(' ');
					Goto(cursor, line);
					
					return *this;
				}
			}
		}
		
		uint8_t writeSpace = 1;
		
		if (m_page == m_pageCount - 1)
		{
			uint8_t maxCursor = MAX_CURSOR - FONT_LENGTH;
			if (m_cursor > maxCursor)
				NextLine();
			else
				writeSpace = std::min(1, maxCursor - m_cursor);
		}
		
		const uint8_t line = m_line, cursor = m_page * MAX_CURSOR + m_cursor;
		
		if (m_row == 0)
		{
			for (uint8_t i = 0; i < FONT_LENGTH; i++)
				Write(Font[ch][i]);
			
			if (writeSpace)
				Write(0);
		}
		else
		{
			SetLine(line + 1);
			
			for (uint8_t i = 0; i < FONT_LENGTH; i++)
				WriteBottom(Font[ch][i], m_row);
			
			if (writeSpace)
				WriteBottom(0, m_row, false);
			
			Goto(cursor, line, true);
			for (uint8_t i = 0; i < FONT_LENGTH; i++)
				WriteTop(Font[ch][i], m_row);
			
			if (writeSpace)
				WriteTop(0, m_row);
		}
		
		return *this;
	}
	
	void KS0108B::PutCharBig(const uint8_t ch, bool interpret_specials)
	{
		//todo: space between characters
		
		if (ch >= MAX_CHARS)
			return;
		
		if (interpret_specials)
		{
			switch (ch)
			{
				case '\n':
				case '\r':
				{
					NextLine(2);
					
					return;
				}
				
				case '\b':
				{
					const uint16_t s = Fa ? m_line * screenLen + m_cursor + 6 : m_line * screenLen + m_page * MAX_CURSOR + m_cursor - (FONT_LENGTH + 1) * 2;
					const uint8_t cursor = s % screenLen, line = s / screenLen;
					
					Goto(cursor, line);
					PutCharBig(' ');
					Goto(cursor, line);
					
					return;
				}
				
				case '\t':
				{
					PutCharBig(' ');
					PutCharBig(' ');
					
					return;
				}
				
				case 127:	//DEL
				{
					uint8_t cursor = m_page * MAX_CURSOR + m_cursor, line = m_line;
					PutCharBig(' ');
					Goto(cursor, line);
					
					return;
				}
			}
		}
		
		uint8_t writeSpace = 2;
		
		if (m_page == m_pageCount - 1)
		{
			uint8_t maxCursor = MAX_CURSOR - (FONT_LENGTH) * 2;
			if (m_cursor > maxCursor)
				NextLine(2);
			else
				writeSpace = std::min(2, maxCursor - m_cursor);
		}
		
		const uint8_t line = m_line, cursor = m_page * MAX_CURSOR + m_cursor;
		
		uint16_t bigCh[FONT_LENGTH] = { 0 };
		for (uint8_t i = 0; i < FONT_LENGTH; i++)
		{
			for (uint8_t j = 0; j < 8; j++)
				bigCh[i] |= (Font[ch][i] & (1 << j)) << j;
		
			bigCh[i] |= bigCh[i] << 1;
		}
		
		
		if (m_row == 0)
		{
			//Low
			SetLine(line + 1);
			for (uint8_t i = 0; i < FONT_LENGTH; i++)
			{
				Write(bigCh[i] >> PIXELS_PER_LINE);
				Write(bigCh[i] >> PIXELS_PER_LINE);
			}
			
			if (writeSpace >= 1)
				Write(0);
			
			if (writeSpace == 2)
				Write(0, false);
			
			//High
			Goto(cursor, line, true);
			for (uint8_t i = 0; i < FONT_LENGTH; i++)
			{
				Write(bigCh[i]);
				Write(bigCh[i], true, 2);
			}
			
			for (uint8_t i = 0; i < writeSpace; i++)
				Write(0, true, 2);
		}
		else
		{
			//Low
			SetLine(line + 2);
			for (uint8_t i = 0; i < FONT_LENGTH; i++)
			{
				WriteBottom(bigCh[i] >> PIXELS_PER_LINE, m_row);
				WriteBottom(bigCh[i] >> PIXELS_PER_LINE, m_row);
			}
			
			if (writeSpace >= 1)
				WriteBottom(0, m_row);
			
			if (writeSpace == 2)
				WriteBottom(0, m_row, false);
			
			//Mid
			Goto(cursor, line + 1, true);
			for (uint8_t i = 0; i < FONT_LENGTH; i++)
			{
				Write(bigCh[i] >> (PIXELS_PER_LINE - m_row));
				Write(bigCh[i] >> (PIXELS_PER_LINE - m_row));
			}
			
			if (writeSpace >= 1)
				Write(0);
			
			if (writeSpace == 2)
				Write(0, false);
			
			//High
			Goto(cursor, line, true);
			for (uint8_t i = 0; i < FONT_LENGTH; i++)
			{
				WriteTop(bigCh[i], m_row);
				WriteTop(bigCh[i], m_row, true, 2);
			}
			
			for (uint8_t i = 0; i < writeSpace; i++)
				WriteTop(0, m_row, true, 2);
		}
	}

	void KS0108B::PutStrBig(const char* str)
	{
		while (*str)
			PutCharBig(*str++);
	}

	void KS0108B::PutStrnBig(const char* str, uint16_t n)
	{
		for (uint16_t i = 0; i < n; i++)
			PutCharBig(str[i]);
	}
	
	void KS0108B::Bitmap(const uint8_t *bmp, uint16_t x, uint8_t y)
	{
		const uint16_t bmp_x = pack_le<uint16_t>(bmp), bmp_y = pack_le<uint16_t>(bmp + 2);
		
		if (!bmp_x || !bmp_y)
			return;
		
		bmp += sizeof(bmp_x) + sizeof(bmp_y);
		
		m_row = Row(y);
		const uint8_t new_row = Row(y + bmp_y), line1 = Line(y), line2 = Line(y + bmp_y);
		
		Goto(x, line1);
		
		if (line1 == line2)
		{
			uint16_t index = MapIndex();
			const uint8_t background_mask = (0xFF << new_row) | (0xFF >> (PIXELS_PER_LINE - m_row));
			
			for (uint8_t i = 0; i < bmp_x; i++, index++)
				Write(((bmp[i] << m_row) & ~background_mask) | (background_mask & m_screenMap[index]));
		}
		else
		{
			for (uint8_t i = 0; i < bmp_x; i++)
				WriteTop(bmp[i], m_row);
			
			for (uint8_t line = line1 + 1; line < line2; line++)
			{
				Goto(x, line);
				
				for (uint8_t i = 0; i < bmp_x; i++)
					Write((bmp[i] >> (PIXELS_PER_LINE - m_row)) | (bmp[i + bmp_x] << m_row));
				
				bmp += bmp_x;
			}
			
			if (new_row)
			{
				Goto(x, line2);
				uint16_t index = MapIndex();
				
				const uint8_t background_mask = 0xFF << new_row;
				
				if (new_row <= bmp_y % PIXELS_PER_LINE)
				{
					if (line2 - line1 < 2)
						bmp += bmp_x;
					
					for (uint8_t i = 0; i < bmp_x; i++, index++)
						Write(((bmp[i] >> (m_row ? PIXELS_PER_LINE - m_row : 0)) & ~background_mask) | (background_mask & m_screenMap[index]));
				}
				else
				{
					for (uint8_t i = 0; i < bmp_x; i++, index++)
						Write((((bmp[i] >> (PIXELS_PER_LINE - m_row)) | (bmp[i + bmp_x] << m_row)) & ~background_mask) | (background_mask & m_screenMap[index]));
				}
			}
		}
		
		m_row = new_row;
	}
	
	void KS0108B::ClearBitmap(const uint8_t *bmp, uint16_t x, uint8_t y, bool fill)
	{
		const uint16_t bmp_x = pack_le<uint16_t>(bmp), bmp_y = pack_le<uint16_t>(bmp + 2);
		if (!bmp_x || !bmp_y)
			return;
		
		FillRectangle(x, y, x + bmp_x - 1, y + bmp_y - 1, fill);
	}
	
	void KS0108B::ReplaceBitmap(const uint8_t *old_bmp, uint16_t old_x, uint8_t old_y, const uint8_t *bmp, uint16_t x, uint8_t y, bool fill)
	{
		const uint16_t bmp_x = pack_le<uint16_t>(bmp), bmp_y = pack_le<uint16_t>(bmp + 2);
		const uint16_t old_bmp_x = pack_le<uint16_t>(old_bmp), old_bmp_y = pack_le<uint16_t>(old_bmp + 2);
		
		if (old_y < y)
			Clear(old_x, old_y, old_x + old_bmp_x - 1, y - 1, fill);
		
		if (old_y + old_bmp_y > y + bmp_y)
			Clear(old_x, y + bmp_y, old_x + old_bmp_x - 1, old_y + old_bmp_y - 1, fill);
		
		if (old_x < x)
			Clear(old_x, std::max(old_y, y), x - 1, std::min(old_y + old_bmp_y, y + bmp_y) - 1, fill);
		
		if (old_x + old_bmp_x > x + bmp_x)
			Clear(x + bmp_x, std::max(old_y, y), old_x + old_bmp_x - 1, std::min(old_y + old_bmp_y, y + bmp_y) - 1, fill);
		
		Bitmap(bmp, x, y);
	}

	void KS0108B::Line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool draw)
	{
		if (x1 >= screenLen || x2 >= screenLen || y1 >= SCREEN_WIDTH || y2 >= SCREEN_WIDTH)
			return;
		
		int16_t dy = y2 - y1, dx = x2 - x1;
		
		if (y1 == y2)
		{
			Line_x(std::min(x1, x2), y1, abs(dx), draw);
			return;
		}
		
		if (x1 == x2)
		{
			Line_y(x1, std::min(y1, y2), abs(dy), draw);
			return;
		}
		
		if (abs(dx) >= abs(dy))
		{
			//Horizontal lines
			//y - y1 = m(x - x1) ===> y - m(x - x1) - y1 = 0
			
			if (x1 > x2)
			{
				std::swap(x1, x2);
				std::swap(y1, y2);
			}
			
			uint8_t x = x1, prev_x = x1, y = y1;
			
			while (x < x2)
			{
				while (y - (int)((dy + sgn(dy)) * (x - x1) / (dx + sgn(dx))) - y1 == 0)
					++x;
				
				Line_x(prev_x, y, x - prev_x - 1, draw);
				prev_x = x;
				y += sgn(y2 - y1);
			}
			
			if (x == x2)
				Pixel(x2, y2, draw);
		}
		else
		{
			//Vertical lines (x -> y , y -> x)
			//x - x1 = m(y - y1) ===> x - m(y - y1) - x1 = 0
			
			if (y1 > y2)
			{
				std::swap(x1, x2);
				std::swap(y1, y2);
			}
			
			uint8_t x = x1, prev_y = y1, y = y1;
			
			while (y < y2)
			{
				while (x - (int)((dx + sgn(dx)) * (y - y1) / (dy + sgn(dy))) - x1 == 0)
					++y;
				
				Line_y(x, prev_y, y - prev_y - 1, draw);
				prev_y = y;
				x += sgn(x2 - x1);
			}
			
			if (y == y2)
				Pixel(x2, y2, draw);
		}
	}


	void KS0108B::Rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool draw)
	{
		if (x1 >= screenLen || x2 >= screenLen || y1 >= SCREEN_WIDTH || y2 >= SCREEN_WIDTH)
			return;
		
		sort(x1, x2);
		sort(y1, y2);
		
		Line_y(x1, y1, y2 - y1, draw);
		
		if (x2 - x1 >= 2)
		{
			Line_x(x1 + 1, y2, x2 - x1 - 2, draw);
			Line_x(x1 + 1, y1, x2 - x1 - 2, draw);
		}
		
		if (y2 > y1)
			Line_y(x2, y1, y2 - y1, draw);
	}


	void KS0108B::FillRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool fill)
	{
		if (x1 >= screenLen || x2 >= screenLen || y1 >= SCREEN_WIDTH || y2 >= SCREEN_WIDTH)
			return;
		
		sort(x1, x2);
		sort(y1, y2);
		
		const uint8_t row1 = y1 % PIXELS_PER_LINE, row2 = y2 % PIXELS_PER_LINE, line1 = y1 / PIXELS_PER_LINE, line2 = y2 / PIXELS_PER_LINE;
		
		Goto(x1, line1);
		uint16_t index = MapIndex(line1, x1);
		
		if (line1 == line2)
		{
			for (uint8_t x = x1; x <= x2; ++x, ++index)
				Write(fill ? ((bit_filter_table[row2 - row1] << row1) | m_screenMap[index]) : (~(bit_filter_table[row2 - row1] << row1) & m_screenMap[index]));
		}
		else
		{
			for (uint8_t x = x1; x <= x2; ++x, ++index)
				Write(fill ? ((0xFF << row1) | m_screenMap[index]) : ((0xFF >> (PIXELS_PER_LINE - row1)) & m_screenMap[index]));
			
			const uint8_t _write = fill * 0xFF;
			for (uint8_t line = line1 + 1; line < line2; line++)
			{
				Goto(x1, line);
				
				for (uint8_t x = x1; x <= x2; ++x, ++index)
					Write(_write);
			}
			
			Goto(x1, line2);
			index = MapIndex(line2, x1);
			
			for (uint8_t x = x1; x <= x2; ++x, ++index)
				Write(fill ? (bit_filter_table[row2] | m_screenMap[index]) : (~bit_filter_table[row2] & m_screenMap[index]));
		}
	}

	void KS0108B::NegateRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
	{
		if (x1 >= screenLen || x2 >= screenLen || y1 >= SCREEN_WIDTH || y2 >= SCREEN_WIDTH)
			return;
		
		sort(x1, x2);
		sort(y1, y2);
		
		const uint8_t row1 = y1 % PIXELS_PER_LINE, row2 = y2 % PIXELS_PER_LINE, line1 = y1 / PIXELS_PER_LINE, line2 = y2 / PIXELS_PER_LINE;
		
		Goto(x1, line1);
		uint16_t index = MapIndex(line1, x1);
		
		if (line1 == line2)
		{
			for (uint8_t x = x1; x <= x2; ++x, ++index)
				Write(((bit_filter_table[row2 - row1] << row1) & ~m_screenMap[index]) | (~(bit_filter_table[row2 - row1] << row1) & m_screenMap[index]));
		}
		else
		{
			static constexpr uint8_t bit_filter_row1[8] = { 0b1111'1111, 0b1111'1110, 0b1111'1100, 0b1111'1000, 0b1111'0000, 0b1110'0000, 0b1100'0000, 0b1000'0000 };
			
			for (uint8_t x = x1; x <= x2; ++x, ++index)
				Write((bit_filter_row1[row1] & ~m_screenMap[index]) | (~bit_filter_row1[row1] & m_screenMap[index]));
			
			for (uint8_t line = line1 + 1; line < line2; line++)
			{
				Goto(x1, line);
				index = MapIndex(line, x1);
				
				for (uint8_t x = x1; x <= x2; ++x, ++index)
					Write(~m_screenMap[index]);
			}
			
			Goto(x1, line2);
			index = MapIndex(line2, x1);
			
			for (uint8_t x = x1; x <= x2; ++x, ++index)
				Write((bit_filter_table[row2] & ~m_screenMap[index]) | (~bit_filter_table[row2] & m_screenMap[index]));
		}
	}

	void KS0108B::Circle(uint8_t x, uint8_t y, uint8_t r, bool draw)
	{
		if (x >= screenLen || y >= SCREEN_WIDTH || r > x || r > y || r >= screenLen - x || r >= SCREEN_WIDTH - y)
			return;
		
		if (r == 0)
		{
			Pixel(x, y, draw);
			return;
		}
		
		const uint8_t xl = r * 732 / 1024;		//cos(45) ~ 0.715 (MAGIC NUMBER); Actual value is closer to 0.707
		const uint16_t r2 = r * r;
		
		uint16_t yi = r;
		
		Pixel(x + r, y, draw);
		Pixel(x - r, y, draw);
		Pixel(x, y + r, draw);
		Pixel(x, y - r, draw);
		
		for (uint16_t xi = 1; xi <= xl; ++xi)
		{
			uint16_t y2 = r2 - xi * xi;
			if (yi * yi - y2 >= y2 - (yi - 1) * (yi - 1))	//real y is more than half a pixel lower
				yi--;
			
			Pixel(x + xi, y + yi, draw);
			Pixel(x - xi, y + yi, draw);
			Pixel(x + xi, y - yi, draw);
			Pixel(x - xi, y - yi, draw);
			Pixel(x + yi, y + xi, draw);
			Pixel(x + yi, y - xi, draw);
			Pixel(x - yi, y + xi, draw);
			Pixel(x - yi, y - xi, draw);
		}
	}

	void KS0108B::FillCircle(uint8_t x, uint8_t y, uint8_t r, bool fill)
	{
		if (x >= screenLen || y >= SCREEN_WIDTH || r > x || r > y || r >= screenLen - x || r >= SCREEN_WIDTH - y)
			return;
		
		if (r == 0)
		{
			Pixel(x, y, fill);
			return;
		}
		
		const uint8_t xl = r * 732 / 1024;  //cos(45) ~ 0.715
		const uint16_t r2 = r * r;
		
		static constexpr uint8_t MAX_XL = (SCREEN_WIDTH / 2) * 732 / 1024, MAX_R = SCREEN_WIDTH / 2;
		
		uint16_t yi = r;
		
		Pixel(x + r, y, fill);
		Pixel(x - r, y, fill);
		Line_y(x, y - r, r * 2, fill);
		
		for (uint8_t xi = 1; xi <= xl; ++xi)
		{
			uint16_t y2 = r2 - xi * xi;
			if (yi * yi - y2 >= y2 - (yi - 1) * (yi - 1))
				yi--;
			
			Line_y(x + xi, y - yi, yi * 2, fill);
			Line_y(x - xi, y - yi, yi * 2, fill);
			Line_y(x + yi, y - xi, xi * 2, fill);
			Line_y(x - yi, y - xi, xi * 2, fill);
		}
	}

	void KS0108B::FillCircleNew(uint8_t x, uint8_t y, uint8_t r, bool fill)
	{
		if (x >= screenLen || y >= SCREEN_WIDTH || r > x || r > y || r >= screenLen - x || r >= SCREEN_WIDTH - y)
			return;
		
		if (r == 0)
		{
			Pixel(x, y, fill);
			return;
		}
		
		const uint8_t xl = r * 732 / 1024;  //cos(45) ~ 0.715
		const uint16_t r2 = r * r;
		
		static constexpr uint8_t MAX_XL = (SCREEN_WIDTH / 2) * 732 / 1024, MAX_R = SCREEN_WIDTH / 2;
		
		uint16_t yi = r;
		uint8_t points[MAX_XL + 1], len[MAX_R + 1];
		
		points[0] = yi;
		len[0] = yi * 2;
		
		for (uint8_t xi = 1; xi <= xl; ++xi)
		{
			uint16_t y2 = r2 - xi * xi;
			if (yi * yi - y2 >= y2 - (yi - 1) * (yi - 1))
			{
				len[yi] = (xi - 1) * 2;
				
				yi--;
			}
			
			points[xi] = yi;
			len[xi] = yi * 2;
		}
		
		len[yi] = xl * 2;
		
		
		
		uint8_t _y = y - r;
		
		_y += 8 - _y % 8;
		
		while (_y > y - r)
			--_y;
		
		_y = (y - r);
		_y += 8 - _y % 8;
		
		while (_y < y - 8)
		{
			Goto(x - len[y - _y] / 2, _y / 8);
			for (uint8_t i = 0; i <= len[y - _y]; ++i)
				Write(0xFF * fill);
			
			_y += 8;
		}
		
		
		if (y - _y <= r && _y + 8 - y <= r)
		{
			uint8_t minLen = std::min(len[y - _y], len[_y + 8 - y]);
			Goto(x - minLen / 2, _y / 8);
			for (uint8_t i = 0; i <= minLen; ++i)
				Write(0xFF * fill);
		}
		
		
		
		_y = y + r;
		
		while (_y % 8)
			--_y;
		
		while (_y > y + 8)
		{
			Goto(x - len[_y - y] / 2, _y / 8 - 1);
			for (uint8_t i = 0; i <= len[_y - y]; ++i)
				Write(0xFF * fill);
			
			_y -= 8;
		}
	}
}
