// *** TODO: ADD LICENSE ***

#include "KS0108B.hpp"

#include <algorithm>
#include <cstdarg>
#include <cstdio>		// vsnprintf



namespace STM32T
{
	static constexpr uint8_t bit_filter_table[8] = { 0b0000'0001, 0b0000'0011, 0b0000'0111, 0b0000'1111, 0b0001'1111, 0b0011'1111, 0b0111'1111, 0b1111'1111 };
	
	static constexpr uint8_t MAX_CHARS = 130, FONT_WIDTH = 5;

	static constexpr uint8_t Font[MAX_CHARS][FONT_WIDTH] =
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
		0x04, 0x02, 0x7F, 0x02, 0x04,	/*up*/
		0x10, 0x20, 0x7F, 0x20, 0x10	/*down*/
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







	bool KS0108B::SetPage(const uint8_t page, const bool force)
	{
		if (!force && m_page == page % m_pageCount)
			return false;
		
		m_page = page % m_pageCount;
		
		f_setCS(1 << m_page);
		
		return true;
	}

	void KS0108B::SetLine(const uint8_t line, const bool force)
	{
		if (!force && m_line == line % MAX_LINES)
			return;
		
		m_line = line % MAX_LINES;
		f_command(m_line | 0b1011'1000, 0, 0);
	}

	void KS0108B::SetCursor(const uint8_t cursor, const bool force)
	{
		if (!force && m_cursor == cursor % MAX_CURSOR)
			return;
		
		m_cursor = cursor % MAX_CURSOR;
		f_command(m_cursor | 0b0100'0000, 0, 0);
	}

	void KS0108B::Display(const bool display) const
	{
		f_command(0b0011'1110 | display, 0, 0);
	}

	void KS0108B::Goto(const uint8_t cursor, const uint8_t line, const bool force)
	{
		if (SetPage(cursor / MAX_CURSOR))
		{
			SetCursor(cursor);
			SetLine(line);
			return;
		}
		
		SetCursor(cursor, force);
		SetLine(line, force);
	}

	void KS0108B::CheckCursor(const uint8_t lines)
	{
		if (++m_cursor >= MAX_CURSOR)
		{
			SetPage(m_page + 1);
			SetCursor(m_cursor);
			SetLine(m_line + (m_page == 0) * lines);
		}
	}

	// todo: Add WriteArr().
	void KS0108B::Write(const uint8_t byte, const bool check, const uint8_t lines)
	{
		m_screenMap[MapIndex()] = byte;
		f_command(byte, 0, 1);
		
		if (check)
			CheckCursor(lines);
	}

	void KS0108B::Write_H(const uint8_t byte, const uint8_t shift, const bool check, const uint8_t lines)
	{
		const uint16_t index = MapIndex();
		m_screenMap[index] = (m_screenMap[index] & (0xFF >> (8 - shift))) | (byte << shift);
		f_command(m_screenMap[index], 0, 1);
		
		if (check)
			CheckCursor(lines);
	}

	void KS0108B::Write_L(const uint8_t byte, const uint8_t shift, const bool check, const uint8_t lines)
	{
		const uint16_t index = MapIndex();
		m_screenMap[index] = (m_screenMap[index] & (0xFF << shift)) | (byte >> (8 - shift));
		f_command(m_screenMap[index], 0, 1);
		
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







	KS0108B::KS0108B(void (*command)(uint8_t data, bool rw, bool rs), void (*set_cs)(uint8_t), const uint8_t page_count) :
			f_command(command), f_setCS(set_cs), m_pageCount(std::min((size_t)page_count, 8u)), m_screenLen(m_pageCount * MAX_CURSOR),
			m_screenMap(new uint8_t[MAX_LINES * m_screenLen]) {}
	
	KS0108B::~KS0108B() { delete[] m_screenMap; }

	/*void KS0108B::SetFa(void)
	{
		if (!init)
		{
			return;
		}
		FontFa=new Char[36];
		////////////////////   1   ////////////////////
		static const uint8_t
		dh_comma[4]={0,0,0,0},                 dl_comma[4]={0,6,5,0},
		dh_semicolon[4]={0,192,160,0},         dl_semicolon[4]={0,6,6,0},
		dh_q[5]={0,128,128,128,0},             dl_q[5]={3,4,88,0,1},
		dh_a[5]={12,4,244,4,6},                dl_a[5]={0,0,15,0,0},
		dh_dal[4]={0,128,0,0},                 dl_dal[4]={0,16,9,6},
		dh_zal[4]={0,128,32,0},                dl_zal[4]={0,16,9,6},
		dh_reh[4]={0,0,0,0},                   dl_reh[4]={32,16,8,6},
		dh_zeh[4]={0,0,0,64},                  dl_zeh[4]={32,16,8,6},
		dh_ta[8]={0,0,240,0,0,128,128,0},      dl_ta[8]={4,4,7,6,5,4,4,3},
		dh_za[8]={0,0,240,0,0,160,128,0},      dl_za[8]={4,4,7,6,5,4,4,3},
		dh_vav[4]={0,0,0,0},                   dl_vav[4]={32,39,53,31};
		
		FontFa[0].SetStateCount(1);    FontFa[0].Set(dh_comma,dl_comma,initial,4);
		FontFa[1].SetStateCount(1);    FontFa[1].Set(dh_semicolon,dl_semicolon,initial,4);
		FontFa[2].SetStateCount(1);    FontFa[2].Set(dh_q,dl_q,initial,5);
		FontFa[3].SetStateCount(1);    FontFa[3].Set(dh_a,dl_a,initial,5);
		FontFa[13].SetStateCount(1);   FontFa[13].Set(dh_dal,dl_dal,initial,4);
		FontFa[14].SetStateCount(1);   FontFa[14].Set(dh_zal,dl_zal,initial,4);
		FontFa[15].SetStateCount(1);   FontFa[15].Set(dh_reh,dl_reh,initial,4);
		FontFa[16].SetStateCount(1);   FontFa[16].Set(dh_zeh,dl_zeh,initial,4);
		FontFa[22].SetStateCount(1);   FontFa[22].Set(dh_ta,dl_ta,initial,8);
		FontFa[23].SetStateCount(1);   FontFa[23].Set(dh_za,dl_za,initial,8);
		FontFa[33].SetStateCount(1);   FontFa[33].Set(dh_vav,dl_vav,initial,4);
		
		////////////////////   2   ////////////////////
		static const uint8_t
		dh_alef_initial[2]={0,240},                       dl_alef_initial[2]={0,7},
		dh_alef_final[2]={0,240},                         dl_alef_final[2]={0,3},
		dh_beh_initial[4]={0,0,0,0},                      dl_beh_initial[4]={4,4,20,3},
		dh_beh_final[7]={0,0,0,0,0,0,0},                  dl_beh_final[7]={3,4,4,20,4,4,3},
		dh_peh_initial[4]={0,0,0,0},                      dl_peh_initial[4]={4,20,36,19},
		dh_peh_final[7]={0,0,0,0,0,0,0},                  dl_peh_final[7]={3,4,20,36,20,4,3},
		dh_teh_initial[4]={0,64,0,64},                    dl_teh_initial[4]={4,4,4,3},
		dh_teh_final[7]={0,0,64,0,64,0,0},                dl_teh_final[7]={3,4,4,4,4,4,3},
		dh_seh_initial[4]={0,64,32,64},                   dl_seh_initial[4]={4,4,4,3},
		dh_seh_final[7]={0,0,64,32,64,0,0},               dl_seh_final[7]={3,4,4,4,4,4,3},
		dh_jim_initial[6]={0,0,128,128,0,0},              dl_jim_initial[6]={4,5,4,20,5,2},
		dh_jim_final[6]={0,0,128,128,0,0},                dl_jim_final[6]={120,133,132,148,133,2},
		dh_cheh_initial[6]={0,0,128,128,0,0},             dl_cheh_initial[6]={4,5,20,36,21,2},
		dh_cheh_final[6]={0,0,128,128,0,0},               dl_cheh_final[6]={120,133,148,164,149,2},
		dh_heh_initial[6]={0,0,128,128,0,0},              dl_heh_initial[6]={4,5,4,4,5,2},
		dh_heh_final[6]={0,0,128,128,0,0},                dl_heh_final[6]={120,133,132,132,133,2},
		dh_kheh_initial[6]={0,0,128,160,0,0},             dl_kheh_initial[6]={4,5,4,4,5,2},
		dh_kheh_final[6]={0,0,128,160,0,0},               dl_kheh_final[6]={120,133,132,132,133,2},
		dh_zheh_initial[5]={0,0,128,64,128},              dl_zheh_initial[5]={32,16,8,6,0},
		dh_zheh_final[5]={0,0,128,64,128},                dl_zheh_final[5]={32,16,8,6,4},
		dh_sad_initial[8]={0,0,0,0,0,128,128,0},          dl_sad_initial[8]={4,7,4,6,5,4,4,3},
		dh_sad_final[11]={0,0,0,0,0,0,0,0,128,128,0},     dl_sad_final[11]={14,16,16,16,16,15,6,5,4,4,3},
		dh_zad_initial[8]={0,0,0,0,0,128,160,0},          dl_zad_initial[8]={4,7,4,6,5,4,4,3},
		dh_zad_final[11]={0,0,0,0,0,0,0,0,128,160,0},     dl_zad_final[11]={14,16,16,16,16,15,6,5,4,4,3},
		dh_feh_initial[6]={0,0,0,0,64,0},                 dl_feh_initial[6]={4,4,4,7,5,7},
		dh_feh_final[8]={0,0,0,0,0,0,64,0},               dl_feh_final[8]={3,4,4,4,4,7,5,7},
		dh_ghaf_initial[6]={0,0,0,64,0,64},               dl_ghaf_initial[6]={4,4,4,7,5,7},
		dh_ghaf_final[7]={0,0,0,0,64,0,64},               dl_ghaf_final[7]={30,32,32,32,39,37,31},
		dh_lam_initial[4]={0,0,0,240},                    dl_lam_initial[4]={4,4,4,3},
		dh_lam_final[5]={0,0,0,0,240},                    dl_lam_final[5]={6,8,8,8,7},
		dh_mim_initial[5]={0,0,0,0,0},                    dl_mim_initial[5]={4,4,7,7,7},
		dh_mim_final[5]={0,0,0,0,0},                      dl_mim_final[5]={252,4,7,7,7},
		dh_noon_initial[4]={0,0,64,0},                    dl_noon_initial[4]={4,4,4,3},
		dh_noon_final[6]={0,0,0,0,0,0},                   dl_noon_final[6]={14,16,16,17,16,15},
		dh_yeh_initial[4]={0,0,0,0},                      dl_yeh_initial[4]={4,20,4,19},
		dh_yeh_final[6]={0,0,0,0,0,0},                    dl_yeh_final[6]={112,128,128,156,148,116};
		
		FontFa[5].SetStateCount(2);
		FontFa[6].SetStateCount(2);
		FontFa[7].SetStateCount(2);
		FontFa[8].SetStateCount(2);
		FontFa[9].SetStateCount(2);
		FontFa[10].SetStateCount(2);
		FontFa[11].SetStateCount(2);
		FontFa[12].SetStateCount(2);
		FontFa[20].SetStateCount(2);
		FontFa[21].SetStateCount(2);
		FontFa[26].SetStateCount(2);
		FontFa[27].SetStateCount(2);
		FontFa[30].SetStateCount(2);
		FontFa[31].SetStateCount(2);
		FontFa[32].SetStateCount(2);
		FontFa[35].SetStateCount(2);
		
		FontFa[5].Set(dh_beh_initial,dl_beh_initial,initial,4);
		FontFa[5].Set(dh_beh_final,dl_beh_final,final,7);
		FontFa[6].Set(dh_peh_initial,dl_peh_initial,initial,4);
		FontFa[6].Set(dh_peh_final,dl_peh_final,final,7);
		FontFa[7].Set(dh_teh_initial,dl_teh_initial,initial,4);
		FontFa[7].Set(dh_teh_final,dl_teh_final,final,7);
		FontFa[8].Set(dh_seh_initial,dl_seh_initial,initial,4);
		FontFa[8].Set(dh_seh_final,dl_seh_final,final,7);
		FontFa[9].Set(dh_jim_initial,dl_jim_initial,initial,6);
		FontFa[9].Set(dh_jim_final,dl_jim_final,final,6);
		FontFa[10].Set(dh_cheh_initial,dl_cheh_initial,initial,6);
		FontFa[10].Set(dh_cheh_final,dl_cheh_final,final,6);
		FontFa[11].Set(dh_heh_initial,dl_heh_initial,initial,6);
		FontFa[11].Set(dh_heh_final,dl_heh_final,final,6);
		FontFa[12].Set(dh_kheh_initial,dl_kheh_initial,initial,6);
		FontFa[12].Set(dh_kheh_final,dl_kheh_final,final,6);
		FontFa[20].Set(dh_sad_initial,dl_sad_initial,initial,8);
		FontFa[20].Set(dh_sad_final,dl_sad_final,final,11);
		FontFa[21].Set(dh_zad_initial,dl_zad_initial,initial,8);
		FontFa[21].Set(dh_zad_final,dl_zad_final,final,11);
		FontFa[26].Set(dh_feh_initial,dl_feh_initial,initial,6);
		FontFa[26].Set(dh_feh_final,dl_feh_final,final,8);
		FontFa[27].Set(dh_ghaf_initial,dl_ghaf_initial,initial,6);
		FontFa[27].Set(dh_ghaf_final,dl_ghaf_final,final,7);
		FontFa[30].Set(dh_lam_initial,dl_lam_initial,initial,4);
		FontFa[30].Set(dh_lam_final,dl_lam_final,final,5);
		FontFa[31].Set(dh_mim_initial,dl_mim_initial,initial,5);
		FontFa[31].Set(dh_mim_final,dl_mim_final,final,5);
		FontFa[32].Set(dh_noon_initial,dl_noon_initial,initial,4);
		FontFa[32].Set(dh_noon_final,dl_noon_final,final,6);
		FontFa[35].Set(dh_yeh_initial,dl_yeh_initial,initial,4);
		FontFa[35].Set(dh_yeh_final,dl_yeh_final,final,6);
		
		////////////////////   3   ////////////////////
		FontFa[4].SetStateCount(3);
		FontFa[17].SetStateCount(3);
		
		FontFa[4].Set(dh_alef_initial,dl_alef_initial,initial,2);
		FontFa[4].Set(dh_alef_final,dl_alef_final,final,2);
		FontFa[4].Set(dh_alef_final,dl_alef_final,medial,2);
		FontFa[17].Set(dh_zheh_initial,dl_zheh_initial,initial,5);
		FontFa[17].Set(dh_zheh_final,dl_zheh_final,final,5);
		FontFa[17].Set(dh_zheh_final,dl_zheh_final,medial,5);
		
		////////////////////   4   ////////////////////
		static const uint8_t
		dh_ain_initial[5]={0,0,0,128,128},                  dl_ain_initial[5]={4,4,7,4,4},
		dh_ain_final[6]={0,128,128,128,128,0},              dl_ain_final[6]={120,133,134,134,5,4},
		dh_ain_medial[5]={0,128,128,128,128},               dl_ain_medial[5]={4,5,6,6,5},
		dh_ain_isolated[6]={0,0,0,0,128,128},               dl_ain_isolated[6]={120,132,132,135,4,4},
		dh_ghain_initial[5]={0,0,0,160,128},                dl_ghain_initial[5]={4,4,7,4,4},
		dh_ghain_final[6]={0,128,128,160,128,0},            dl_ghain_final[6]={120,133,134,134,5,4},
		dh_ghain_medial[5]={0,128,160,128,128},             dl_ghain_medial[5]={4,5,6,6,5},
		dh_ghain_isolated[6]={0,0,0,0,160,128},             dl_ghain_isolated[6]={120,132,132,135,4,4},
		dh_kaf_initial[7]={0,0,0,224,16,8,4},               dl_kaf_initial[7]={4,4,4,3,0,0,0},
		dh_kaf_final[10]={0,0,0,0,0,0,224,16,8,4},          dl_kaf_final[10]={3,4,4,4,4,4,3,4,4,4},
		dh_kaf_medial[7]={0,0,0,224,16,8,4},                dl_kaf_medial[7]={4,4,4,3,4,4,4},
		dh_kaf_isolated[10]={0,0,0,0,0,0,224,16,8,4},       dl_kaf_isolated[10]={3,4,4,4,4,4,3,0,0,0},
		dh_gaf_initial[7]={0,0,0,232,20,10,5},              dl_gaf_initial[7]={4,4,4,3,0,0,0},
		dh_gaf_final[10]={0,0,0,0,0,0,232,20,10,5},         dl_gaf_final[10]={3,4,4,4,4,4,3,4,4,4},
		dh_gaf_medial[7]={0,0,0,232,20,10,5},               dl_gaf_medial[7]={4,4,4,3,4,4,4},
		dh_gaf_isolated[10]={0,0,0,0,0,0,232,20,10,5},      dl_gaf_isolated[10]={3,4,4,4,4,4,3,0,0,0},
		dh_he_initial[8]={0,0,64,192,128,128,128,0},        dl_he_initial[8]={4,4,7,4,6,5,4,7},
		dh_he_final[5]={0,0,128,128,0},                     dl_he_final[5]={0,3,0,0,3},
		dh_he_medial[5]={0,0,0,128,128},                    dl_he_medial[5]={4,4,31,36,63},
		dh_he_isolated[6]={0,0,128,0,0,0},                  dl_he_isolated[6]={0,6,9,9,6,0},
		
		dh_sin_initial[7]={0,0,0,0,0,0,0},                  dl_sin_initial[7]={4,4,7,4,7,4,3},
		dh_sin_final[10]={0,0,0,0,0,0,0,0,0,0},             dl_sin_final[10]={56,64,64,64,64,63,4,7,4,3},
		dl_sin_medial[7]={4,4,7,4,7,4,7},                   dl_sin_isolated[10]={56,64,64,64,64,63,4,7,4,7},
		
		dh_shin_initial[7]={0,0,0,64,32,64,0},              dl_shin_initial[7]={4,4,7,4,7,4,3},	
		dh_shin_final[10]={0,0,0,0,0,0,64,32,64,0},         dl_shin_final[10]={56,64,64,64,64,63,4,7,4,3},
		dl_shin_medial[7]={4,4,7,4,7,4,7},	                dl_shin_isolated[10]={56,64,64,64,64,63,4,7,4,7};
		
		FontFa[24].SetStateCount(4);
		FontFa[25].SetStateCount(4);
		FontFa[28].SetStateCount(4);
		FontFa[29].SetStateCount(4);
		FontFa[34].SetStateCount(4);
		
		FontFa[24].Set(dh_ain_initial,dl_ain_initial,initial,5);
		FontFa[24].Set(dh_ain_final,dl_ain_final,final,6);
		FontFa[24].Set(dh_ain_medial,dl_ain_medial,medial,5);
		FontFa[24].Set(dh_ain_isolated,dl_ain_isolated,isolated,6);
		FontFa[25].Set(dh_ghain_initial,dl_ghain_initial,initial,5);
		FontFa[25].Set(dh_ghain_final,dl_ghain_final,final,6);
		FontFa[25].Set(dh_ghain_medial,dl_ghain_medial,medial,5);
		FontFa[25].Set(dh_ghain_isolated,dl_ghain_isolated,isolated,6);
		FontFa[28].Set(dh_kaf_initial,dl_kaf_initial,initial,7);
		FontFa[28].Set(dh_kaf_final,dl_kaf_final,final,10);
		FontFa[28].Set(dh_kaf_medial,dl_kaf_medial,medial,7);
		FontFa[28].Set(dh_kaf_isolated,dl_kaf_isolated,isolated,10);
		FontFa[29].Set(dh_gaf_initial,dl_gaf_initial,initial,7);
		FontFa[29].Set(dh_gaf_final,dl_gaf_final,final,10);
		FontFa[29].Set(dh_gaf_medial,dl_gaf_medial,medial,7);
		FontFa[29].Set(dh_gaf_isolated,dl_gaf_isolated,isolated,10);
		FontFa[34].Set(dh_he_initial,dl_he_initial,initial,8);
		FontFa[34].Set(dh_he_final,dl_he_final,final,5);
		FontFa[34].Set(dh_he_medial,dl_he_medial,medial,5);
		FontFa[34].Set(dh_he_isolated,dl_he_isolated,isolated,6);
		
		FontFa[18].SetStateCount(4);
		FontFa[19].SetStateCount(4);
		
		FontFa[18].Set(dh_sin_initial,dl_sin_initial,initial,7);
		FontFa[18].Set(dh_sin_final,dl_sin_final,final,10);
		FontFa[18].Set(dh_sin_final,dl_sin_isolated,isolated,10);
		FontFa[18].Set(dh_sin_initial,dl_sin_medial,medial,7);
		
		FontFa[19].Set(dh_shin_initial,dl_shin_initial,initial,7);
		FontFa[19].Set(dh_shin_final,dl_shin_final,final,10);
		FontFa[19].Set(dh_shin_final,dl_shin_isolated,isolated,10);
		FontFa[19].Set(dh_shin_initial,dl_shin_medial,medial,7);
		
		faset=1;
	}*/


	void KS0108B::Test()
	{
		ClearScreen(true);
		HAL_Delay(1000);
		ClearScreen();
		HAL_Delay(1000);
		
		Gotoxy(100, 4);
		WriteByte(0xFF);
		WriteByte(0xAA);
		WriteByte(0x55);
		WriteByte(0x00);
		
		Line(60, 40, 70, 25);
		Rectangle(60, 45, 90, 55);
		FillRectangle(80, 25, 90, 40);
		Circle(110, 30, 8);
		FillCircle(110, 52, 10);
		
		Gotoxy(0, 4);
		PutStr("Normal");
		NextLine(2);
		PutStr("Big", true);
		
		while (1)
		{
			HAL_Delay(1000);
			NegateRectangle(0, 4, 6 * 6, 4 + 7);
		}
	}

	void KS0108B::Init()
	{
		f_setCS(0xFF);
		
		Display(1);
		ClearScreen();
	}

	void KS0108B::ClearScreen(const bool fill)
	{
		f_setCS(0xFF);
		
		const uint8_t _write = 0xFF * fill;
		
		SetCursor(0);
		for (uint8_t i = 0; i < MAX_LINES; i++)
		{
			SetLine(i);
			
			for (uint8_t j = 0; j < MAX_CURSOR; j++)
				f_command(_write, 0, 1);
		}
		
		SetPage(0, true);	//ensure page change
		SetCursor(0);
		SetLine(0);
		m_row = 0;
		
		memset(m_screenMap, _write, MAX_LINES * m_screenLen);
	}

	KS0108B& KS0108B::Gotoxy(const uint8_t x, const uint8_t y)
	{
		m_row = y % PIXELS_PER_LINE;
		Goto(x, y / PIXELS_PER_LINE);
		return *this;
	}

	KS0108B& KS0108B::Movexy(const int16_t x, const int8_t y)
	{
		static constexpr uint8_t MAX_PIXELS = PIXELS_PER_LINE * MAX_LINES;
		
		int32_t _x = (m_page * MAX_CURSOR + m_cursor + x) % m_screenLen, _y = (m_line * PIXELS_PER_LINE + m_row + y) % MAX_PIXELS;
		
		if (_x < 0)
			_x += (-_x / m_screenLen + 1) * m_screenLen;
		
		if (_y < 0)
			_y = (-_y / MAX_PIXELS + 1) * MAX_PIXELS;
		
		Gotoxy(_x, _y);
		return *this;
	}

	KS0108B& KS0108B::Gotoxl(const uint8_t x, const uint8_t line)
	{
		m_row = 0;
		Goto(x, line);
		return *this;
	}

	void KS0108B::WriteByte(const uint8_t byte, const uint8_t repeat)
	{
		if (m_row == 0)
			for (uint8_t i = 0; i < repeat; i++)
				Write(byte);
		else
		{
			const uint8_t line = m_line, cursor = LongCursor();
			
			// todo: Do all the L & H writes in one page, then move to the next page.
			
			//Bottom part
			SetLine(line + 1);
			for (uint8_t i = 0; i < repeat; i++)
				Write_L(byte, m_row, i < repeat - 1);	// Don't check for the last byte
			
			//Top part
			//SetLine(line);
			//SetCursor(m_cursor, true);
			Goto(cursor, line, true);
			for (uint8_t i = 0; i < repeat; i++)
				Write_H(byte, m_row);
		}
	}

	/*uint8_t KS0108B::Read(void)
	{
		if (init)
		{
			uint8_t data=0;
			GPIO_InitTypeDef GPIO_InitStruct;
			GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
			GPIO_InitStruct.Pull = GPIO_NOPULL;
			GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
			for (uint8_t i=0;i<8;i++)
			{
				GPIO_InitStruct.Pin = Pin[i];
				HAL_GPIO_Init(Port[i], &GPIO_InitStruct);
			}
			
			Port[10]->BSRR=Pin[10];
			Port[9]->BSRR=Pin[9];
			
			HAL_GPIO_WritePin(Port[8], Pin[8], GPIO_PIN_RESET);
			GLCD_Delay_us(80);
			HAL_GPIO_WritePin(Port[8], Pin[8], GPIO_PIN_SET);
			GLCD_Delay_us(80);
			data=(uint8_t)HAL_GPIO_ReadPin(Port[7],Pin[7]);
			for (int8_t i=6;i>=0;i--)
			{
				data<<=1;
				data|=(uint8_t)HAL_GPIO_ReadPin(Port[i],Pin[i]);
			}
			GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			for (uint8_t i=0;i<8;i++)
			{
				GPIO_InitStruct.Pin = Pin[i];
				HAL_GPIO_Init(Port[i], &GPIO_InitStruct);
			}
			
			CheckCursor();
			return data;
		}
		return 0;
	}*/

	void KS0108B::NextLine(const uint8_t lines)
	{
		SetPage(0);
		SetCursor(0);
		SetLine(m_line + lines);
		
		//SetLine(m_line+1+Fa);
	}

	void KS0108B::Pixel(uint8_t x, uint8_t y, bool fill)
	{
		if (x >= m_screenLen || y >= SCREEN_WIDTH)
			return;
		
		uint8_t row = y % PIXELS_PER_LINE;
		y /= PIXELS_PER_LINE;
		
		Goto(x, y);
		
		Write(fill ? (m_screenMap[MapIndex(y, x)] | (1 << row)) : (m_screenMap[MapIndex(y, x)] & (~(1 << row))));
	}

	void KS0108B::PutChar(const uint8_t ch, bool big, bool interpret_specials)
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
					NextLine();
					if (big)
						NextLine();
					
					return;
				}
				
				case '\b':
				{
					const uint16_t s = Fa ? m_line * m_screenLen + m_cursor + 6 : m_line * m_screenLen + m_page * MAX_CURSOR + m_cursor - (FONT_WIDTH + 1) * (1 + big);
					const uint8_t cursor = s % m_screenLen, line = s / m_screenLen;
					
					Goto(cursor, line);
					PutChar(' ', big);
					Goto(cursor, line);
					
					return;
				}
				
				case '\t':
				{
					PutChar(' ', big);
					PutChar(' ', big);
					
					return;
				}
				
				case 127:	//DEL
				{
					uint8_t cursor = m_page * MAX_CURSOR + m_cursor, line = m_line;
					PutChar(' ', big);
					Goto(cursor, line);
					
					return;
				}
			}
		}
		
		uint8_t writeSpace = 1 + big;
		
		if (m_page == m_pageCount - 1)
		{
			uint8_t maxCursor = MAX_CURSOR - (FONT_WIDTH) * (1 + big);
			if (m_cursor > maxCursor)
			{
				NextLine();
				if (big)
					NextLine();
			}
			else
				writeSpace = std::min(1 + big, maxCursor - m_cursor);
		}
		
		const uint8_t line = m_line, cursor = m_page * MAX_CURSOR + m_cursor;
		
		if (big)
		{
			uint16_t bigCh[FONT_WIDTH] = { 0 };
			for (uint8_t i = 0; i < FONT_WIDTH; i++)
			{
				for (uint8_t j = 0; j < 8; j++)
					bigCh[i] |= (Font[ch][i] & (1 << j)) << j;
			
				bigCh[i] |= bigCh[i] << 1;
			}
			
			
			if (m_row == 0)
			{
				//Low
				SetLine(line + 1);
				for (uint8_t i = 0; i < FONT_WIDTH; i++)
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
				for (uint8_t i = 0; i < FONT_WIDTH; i++)
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
				for (uint8_t i = 0; i < FONT_WIDTH; i++)
				{
					Write_L(bigCh[i] >> PIXELS_PER_LINE, m_row);
					Write_L(bigCh[i] >> PIXELS_PER_LINE, m_row);
				}
				
				if (writeSpace >= 1)
					Write_L(0, m_row);
				
				if (writeSpace == 2)
					Write_L(0, m_row, false);
				
				//Mid
				Goto(cursor, line + 1, true);
				for (uint8_t i = 0; i < FONT_WIDTH; i++)
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
				for (uint8_t i = 0; i < FONT_WIDTH; i++)
				{
					Write_H(bigCh[i], m_row);
					Write_H(bigCh[i], m_row, true, 2);
				}
				
				for (uint8_t i = 0; i < writeSpace; i++)
					Write_H(0, m_row, true, 2);
			}
		}
		else
		{
			if (m_row == 0)
			{
				for (uint8_t i = 0; i < FONT_WIDTH; i++)
					Write(Font[ch][i]);
				
				if (writeSpace)
					Write(0);
			}
			else
			{
				SetLine(line + 1);
				
				for (uint8_t i = 0; i < FONT_WIDTH; i++)
					Write_L(Font[ch][i], m_row);
				
				if (writeSpace)
					Write_L(0, m_row, false);
				
				Goto(cursor, line, true);
				for (uint8_t i = 0; i < FONT_WIDTH; i++)
					Write_H(Font[ch][i], m_row);
				
				if (writeSpace)
					Write_H(0, m_row);
			}
		}
	}

	void KS0108B::PutStr(const char* str, bool big)
	{
		while (*str)
			PutChar(*str++, big);
	}

	void KS0108B::PutStrn(const char* str, uint16_t n, bool big)
	{
		for (uint16_t i = 0; i < n; i++)
			PutChar(str[i], big);
	}
	
	void KS0108B::PutStrf(const char* fmt, ...)
	{
		char buf[64];
		
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);
		
		PutStr(buf);
	}
	
	void KS0108B::Bitmap(const uint8_t * bmp, uint16_t x, uint8_t y)
	{
		// todo: Specify a format for bmp and handle y % PIXELS_PER_LINE != 0.
		
		uint16_t bmp_x = *(uint16_t*)bmp, bmp_y = *((uint16_t*)(bmp) + 1);
		
		if (!bmp_x || !bmp_y || bmp_y % PIXELS_PER_LINE)	// || x + bmp_x > m_screenLen || x >= m_screenLen || y + bmp_y > SCREEN_WIDTH || y >= SCREEN_WIDTH)
			return;
		
		bmp += 4;
		
		m_row = y % PIXELS_PER_LINE;
		const uint8_t line1 = y / PIXELS_PER_LINE, line2 = (y + bmp_y) / PIXELS_PER_LINE;
		
		Goto(x, line1);
		uint16_t index = MapIndex(line1, x);
		
		for (uint8_t i = 0; i < bmp_x; i++, index++)
			Write((bmp[i] << m_row) | ((0xFF >> (PIXELS_PER_LINE - m_row)) & m_screenMap[index]));
		
		for (uint8_t line = line1 + 1; line < line2; line++)
		{
			Goto(x, line);
			
			for (uint8_t i = 0; i < bmp_x; i++)
				Write((bmp[i] >> (PIXELS_PER_LINE - m_row)) | (bmp[i + bmp_x] << m_row));
			
			bmp += bmp_x;
		}
		
		if (m_row)
		{
			Goto(x, line2);
			index = MapIndex(line2, x);
			
			for (uint8_t i = 0; i < bmp_x; i++, index++)
				Write((bmp[i] >> (PIXELS_PER_LINE - m_row)) | ((0xFF << m_row) & m_screenMap[index]));
		}
	}

	/*void KS0108B::PutNum(int32_t num)
	{
		uint8_t n[11];
		itoa(n,num);
		PutStr(n);
	}

	void KS0108B::PutNum(float num)
	{
		uint8_t n[20];
		itoa(n,num);
		PutStr(n);
	}*/

	/*void KS0108B::PutCharFa(uint8_t ch, State s)
	{
		if (faset)
		{
			Fa=1;
			uint8_t w=FontFa[ch].getwidth(s), i;
			const uint8_t *dh=FontFa[ch].getdh(s), *dl=FontFa[ch].getdl(s);
			if (m_page==0 && m_cursor+1<w)
			{
				Goto(m_screenLen-1,m_line+2);
			}
			uint8_t l=m_line, c=64*m_page+m_cursor+1-w;
			if (m_row==0)
			{
				const uint8_t *d=dh;
				for (uint8_t j=0;j<2;j++)
				{
					Goto(c,l+j);
					for (i=0;i<w;i++)
					{
						Write(d[i] | m_screenMap[m_line][c+i]);
					}
					d=dl;
				}
			}
			else
			{
				Goto(c,l);
				for (i=0;i<w;i++)
				{
					Write((dh[i]<<m_row) | m_screenMap[m_line][c+i]);
				}
				l++;
				Goto(c,l);
				for (i=0;i<w;i++)
				{
					Write((dh[i]>>(8-m_row)) | (dl[i]<<m_row));
				}
				l++;
				Goto(c,l);
				for (i=0;i<w;i++)
				{
					Write((dl[i]>>(8-m_row)) | m_screenMap[m_line][c+i]);
				}
				l-=2;
			}
			if (c==0)
			{
				Goto(m_screenLen-1,l+2);
			}
			else
			{
				Goto(--c,l);
			}
			Fa=0;
		}
	}

	void KS0108B::PutStrFa(uint8_t *str)
	{
		if (faset)
		{
			uint8_t c,cp=40;
			uint16_t len=STRLEN(str),i=0,end=0;
			State s=initial;
			while (i<len)
			{
				if (i>=end)
				{
					end=i;
					while (str[end]>0x80)
					{
						end++;
					}
				}
				if (str[i]>=0xD8 && str[i]<=0xDB)
				{
					i++;
				}
				if (str[i]<128)
				{
					s=initial;
					cp=40;
					if (m_cursor+m_page*64+1<6)
					{
						Goto(m_screenLen-1,m_line+2);
					}
					Movexy(-6,5);
					PutChar(str[i]);
					Movexy(-6,-5);
				}
				else
				{
					if (str[i]>=0xAA && str[i]<=0xBA && str[i]!=0xAF)
					{
						c=str[i]-163+(1*(str[i]>=0xB3))+(1*(str[i]>=0xAD));
						goto put;
					}
					switch (str[i])
					{
						case 0x81: c=26;    break;
						case 0x82: c=27;    break;
						case 0xA9: c=28;    break;
						case 0x84: c=30;    break;
						case 0x85: c=31;    break;
						
						case 0xA2: c=3;     break;
						case 0xA7: c=4;     break;
						case 0xA8: c=5;     break;
						case 0x87: c=34;    break;
						case 0x88: c=33;    break;
						case 0xBE: c=6;     break;
						case 0x98: c=17;    break;
						case 0x9B: c=1;     break;
						case 0x9F: c=2;     break;
						case 0x8A: c=35;    break;
						case 0x8C:
						{
							switch (str[i-1])
							{
								case 0xD8: c=0;    break;
								case 0xDB: c=35;   break;
							}
							break;
						}
						case 0xAF:
						{
							switch (str[i-1])
							{
								case 0xDA: c=29;   break;
								case 0xD8: c=13;   break;
							}
							break;
						}
						case 0x86:
						{
							switch (str[i-1])
							{
								case 0xDA: c=10;   break;
								case 0xD9: c=32;   break;
							}
							break;
						}
					}
		put:
					if (cp==40)
					{
						s=initial;
					}
					else
					{
						s=medial;
					}
					if (cp<=4 || cp==33 || (cp>=13 && cp<=17))
					{
						s=initial;
					}
					if (end-i==1)
					{
						s=final;
						if (cp<=4 || cp==33 || (cp>=13 && cp<=17))
						{
							s=isolated;
						}
					}
					PutCharFa(c,s);
					cp=c;
				}
				i++;
			}
		}
	}

	void KS0108B::PutStrFa(const char *str)
	{
		PutStrFa((uint8_t*)str);
	}*/

	void KS0108B::Line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool draw)
	{
		if (x1 >= m_screenLen || x2 >= m_screenLen || y1 >= SCREEN_WIDTH || y2 >= SCREEN_WIDTH)
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
			
			//float slope = ((float)dy + sgn(dy)) / (dx + sgn(dx));
			
			if (x1 > x2)
			{
				std::swap(x1, x2);
				std::swap(y1, y2);
			}
			
			uint8_t x = x1, prev_x = x1, y = y1;
			
			while (x < x2)
			{
				//while (y - (int)(slope * (x - x1)) - y1 == 0)
				while (y - (int)((dy + sgn(dy)) * (x - x1) / (dx + sgn(dx))) - y1 == 0)
					++x;
				
				Line_x(prev_x, y, x - prev_x - 1, draw);
				prev_x = x;
				//y += sgn(slope);
				y += sgn(y2 - y1);
			}
			
			if (x == x2)
				Pixel(x2, y2, draw);
		}
		else
		{
			//Vertical lines (x -> y , y -> x)
			//x - x1 = m(y - y1) ===> x - m(y - y1) - x1 = 0
			
			//float slope = ((float)dx + sgn(dx)) / (dy + sgn(dy));
			
			if (y1 > y2)
			{
				std::swap(x1, x2);
				std::swap(y1, y2);
			}
			
			uint8_t x = x1, prev_y = y1, y = y1;
			
			while (y < y2)
			{
				//while (x - (int)(slope * (y - y1)) - x1 == 0)
				while (x - (int)((dx + sgn(dx)) * (y - y1) / (dy + sgn(dy))) - x1 == 0)
					++y;
				
				Line_y(x, prev_y, y - prev_y - 1, draw);
				prev_y = y;
				//x += sgn(slope);
				x += sgn(x2 - x1);
			}
			
			if (y == y2)
				Pixel(x2, y2, draw);
		}
		
		/*uint8_t maxDelta;
		bool mirror;
		float m;
		
		if (abs(dx) <= abs(dy))
		{
			maxDelta = abs(dy);
			mirror = true;
			m = ((float)dx + sgn(dx)) * sgn(dy) / (dy + sgn(dy));	//adds one to dy and dx to account for the end point
		}
		else
		{
			maxDelta = abs(dx);
			mirror = false;
			m = ((float)dy + sgn(dy)) * sgn(dx) / (dx + sgn(dx));
		}
		
		Pixel(x1, y1, draw);
		
		int8_t pchange = 0;
		for (uint8_t i = 1; i <= maxDelta; i++)
		{
			float change = m * i;
			if (mirror)
			{
				//y1++;
				y1 += sgn(dy);
				x1 += (int8_t)(change - pchange);
			}
			else
			{
				//x1++;
				x1 += sgn(dx);
				y1 += (int8_t)(change - pchange);
			}
			
			Pixel(x1, y1, draw);
			pchange = change;
		}*/
	}


	void KS0108B::Rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool draw)
	{
		if (x1 >= m_screenLen || x2 >= m_screenLen || y1 >= SCREEN_WIDTH || y2 >= SCREEN_WIDTH)
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
		
		/*uint8_t row1 = y1 % 8, row2 = y2 % 8, write = 255 * d, y, jmax, corner;
		y1 /= 8;
		y2 /= 8;
		
		for (y = y1 + 1; y < y2; y++)
		{
			Goto(x1, y);
			Write(write);
			Goto(x2, y);
			Write(write);
		}
		y = y1;
		if (y1 == y2)
		{
			corner = 255 >> (7 - row2 + row1) << row1;
			write = (1 << row1) | (1 << row2);
			jmax = 1;
		}
		else
		{
			corner = 255 << row1;
			write = 1 << row1;
			jmax = 2;
		}
		
		Goto(x1, y1);
		
		for (uint8_t j = 0; j < jmax; j++)
		{
			if (d)
			{
				Write(corner | m_screenMap[MapIndex(y, x1)]);
				
				for (uint8_t x = x1 + 1; x < x2; x++)
					Write(write | m_screenMap[MapIndex(y, x)]);
				
				Write(corner | m_screenMap[MapIndex(y, x2)]);
			}
			else
			{
				write = ~write;
				corner = ~corner;
				
				Write(corner & m_screenMap[MapIndex(y, x1)]);
				
				for (uint8_t x = x1 + 1; x < x2; x++)
					Write(write & m_screenMap[MapIndex(y, x)]);
				
				Write(corner & m_screenMap[MapIndex(y, x2)]);
			}
			
			if (j == jmax - 1)
				break;
			
			write = 128 >> (7 - row2);
			corner = 255 >> (7 - row2);
			Goto(x1, y2);
			y = y2;
		}*/
	}


	void KS0108B::FillRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool fill)
	{
		if (x1 >= m_screenLen || x2 >= m_screenLen || y1 >= SCREEN_WIDTH || y2 >= SCREEN_WIDTH)
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
		if (x1 >= m_screenLen || x2 >= m_screenLen || y1 >= SCREEN_WIDTH || y2 >= SCREEN_WIDTH)
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
		
		
		
		
		/*uint8_t row1=y1%8,row2=y2%8,_xor,ymax,y;
		y1/=8;
		y2/=8;
		y=y1+1;
		ymax=y2;
		if (row1==0)
			y--;
		if (row2==7)
			ymax++;
		for (;y<ymax;y++)
		{
			Goto(x1,y);
			for (uint8_t x=x1;x<=x2;x++)
				Write(~m_screenMap[MapIndex(y, x)]);
		}
		if (y1!=y2)
		{
			Goto(x1,y1);
			if (row1!=0)
			{
				_xor=255<<row1;
				for (uint8_t x=x1;x<=x2;x++)
					Write(m_screenMap[MapIndex(y1, x)] ^ _xor);
			}
			Goto(x1,y2);
			if (row2!=7)
			{
				_xor=255>>(7-row2);
				for (uint8_t x=x1;x<=x2;x++)
					Write(m_screenMap[MapIndex(y2, x)]^_xor);
			}
		}
		else
		{
			if (row1!=0 || row2!=7)
			{
				Goto(x1,y1);
				_xor=255>>(7-row2+row1)<<row1;
				for (uint8_t x=x1;x<=x2;x++)
					Write(m_screenMap[MapIndex(y1, x)]^_xor);
			}
		}*/
	}

	void KS0108B::Circle(uint8_t x, uint8_t y, uint8_t r, bool draw)
	{
		if (x >= m_screenLen || y >= SCREEN_WIDTH || r > x || r > y || r >= m_screenLen - x || r >= SCREEN_WIDTH - y)
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
		if (x >= m_screenLen || y >= SCREEN_WIDTH || r > x || r > y || r >= m_screenLen - x || r >= SCREEN_WIDTH - y)
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
		if (x >= m_screenLen || y >= SCREEN_WIDTH || r > x || r > y || r >= m_screenLen - x || r >= SCREEN_WIDTH - y)
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
		
		//Goto(x - len[y - (_y - 1)] / 2, (_y - 1) / 8);
		
		while (_y > y - r)
		{
			//for (uint8_t i = 0; i < len[y - _y] - len[y - (_y - 1)]; ++i, ++index)
			//{
			//	Write_H(0xFF * fill, _y % 8);
			//}
			
			--_y;
		}
		
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

