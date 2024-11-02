#pragma once

#include "../IO.hpp"
#include "../Timing.hpp"
#include "../Common.hpp"

#include <optional>



namespace STM32T
{
	class DS18B20
	{
		// All times are in microseconds.
		static constexpr uint32_t
			RESET_LOW = 480, RESET_HIGH = 480,
			PRESENCE_MAX_DELAY = 60, PRESENCE_MIN = 60, PRESENCE_MAX = 240,
			TIME_SLOT = 60,
			WRITE_1_LOW = 1,
			READ_LOW = 1, READ_DELAY = 9,
			RECOVERY = 1;
		
		// Based on the datasheet
		static_assert(RESET_LOW >= 480 && RESET_HIGH >= 480);
		static_assert(TIME_SLOT >= 60 && TIME_SLOT <= 120);
		static_assert(WRITE_1_LOW >= 1 && WRITE_1_LOW <= 15);
		static_assert(READ_LOW >= 1);
		static_assert(READ_DELAY + READ_LOW <= 15);
		static_assert(RECOVERY >= 1);
		
		
		enum class CMD : uint8_t
		{
			// ROM Commands
			SEARCH_ROM = 0xF0,
			READ_ROM = 0x33,
			MATCH_ROM = 0x55,
			SKIP_ROM = 0xCC,
			ALARM_SEARCH = 0xEC,
			
			// Function Commands
			CONVERT_T = 0x44,
			WRITE_SCRATCHPAD = 0x4E,
			READ_SCRATCHPAD = 0xBE,
			COPY_SCRATCHPAD = 0x48,
			RECALL_EEPROM = 0xB8,
			READ_PS = 0xB4
		};
		
		// Reflected
		static constexpr uint8_t TABLE[256] =
		{
			0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83, 0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41,
			0x9D, 0xC3, 0x21, 0x7F, 0xFC, 0xA2, 0x40, 0x1E, 0x5F, 0x01, 0xE3, 0xBD, 0x3E, 0x60, 0x82, 0xDC,
			0x23, 0x7D, 0x9F, 0xC1, 0x42, 0x1C, 0xFE, 0xA0, 0xE1, 0xBF, 0x5D, 0x03, 0x80, 0xDE, 0x3C, 0x62,
			0xBE, 0xE0, 0x02, 0x5C, 0xDF, 0x81, 0x63, 0x3D, 0x7C, 0x22, 0xC0, 0x9E, 0x1D, 0x43, 0xA1, 0xFF,
			0x46, 0x18, 0xFA, 0xA4, 0x27, 0x79, 0x9B, 0xC5, 0x84, 0xDA, 0x38, 0x66, 0xE5, 0xBB, 0x59, 0x07,
			0xDB, 0x85, 0x67, 0x39, 0xBA, 0xE4, 0x06, 0x58, 0x19, 0x47, 0xA5, 0xFB, 0x78, 0x26, 0xC4, 0x9A,
			0x65, 0x3B, 0xD9, 0x87, 0x04, 0x5A, 0xB8, 0xE6, 0xA7, 0xF9, 0x1B, 0x45, 0xC6, 0x98, 0x7A, 0x24,
			0xF8, 0xA6, 0x44, 0x1A, 0x99, 0xC7, 0x25, 0x7B, 0x3A, 0x64, 0x86, 0xD8, 0x5B, 0x05, 0xE7, 0xB9,
			0x8C, 0xD2, 0x30, 0x6E, 0xED, 0xB3, 0x51, 0x0F, 0x4E, 0x10, 0xF2, 0xAC, 0x2F, 0x71, 0x93, 0xCD,
			0x11, 0x4F, 0xAD, 0xF3, 0x70, 0x2E, 0xCC, 0x92, 0xD3, 0x8D, 0x6F, 0x31, 0xB2, 0xEC, 0x0E, 0x50,
			0xAF, 0xF1, 0x13, 0x4D, 0xCE, 0x90, 0x72, 0x2C, 0x6D, 0x33, 0xD1, 0x8F, 0x0C, 0x52, 0xB0, 0xEE,
			0x32, 0x6C, 0x8E, 0xD0, 0x53, 0x0D, 0xEF, 0xB1, 0xF0, 0xAE, 0x4C, 0x12, 0x91, 0xCF, 0x2D, 0x73,
			0xCA, 0x94, 0x76, 0x28, 0xAB, 0xF5, 0x17, 0x49, 0x08, 0x56, 0xB4, 0xEA, 0x69, 0x37, 0xD5, 0x8B,
			0x57, 0x09, 0xEB, 0xB5, 0x36, 0x68, 0x8A, 0xD4, 0x95, 0xCB, 0x29, 0x77, 0xF4, 0xAA, 0x48, 0x16,
			0xE9, 0xB7, 0x55, 0x0B, 0x88, 0xD6, 0x34, 0x6A, 0x2B, 0x75, 0x97, 0xC9, 0x4A, 0x14, 0xF6, 0xA8,
			0x74, 0x2A, 0xC8, 0x96, 0x15, 0x4B, 0xA9, 0xF7, 0xB6, 0xE8, 0x0A, 0x54, 0xD7, 0x89, 0x6B, 0x35,
		};
		
		static constexpr uint32_t TIMEOUTS[4] = { 94, 188, 375, 750 };
		
		
		
		IO m_dq;
		uint8_t m_bits = 12;
		
		bool Init()
		{
			m_dq.Timed((uint16_t)RESET_LOW, DWT_Delay, false);
			const uint32_t start = DWT_GetTick();
			
			if (!m_dq.Wait(false, PRESENCE_MAX_DELAY, DWT_GetTick) || !m_dq.CheckPulse(false, PRESENCE_MAX, PRESENCE_MIN, DWT_GetTick))
				return false;
			
			while (DWT_GetTick() - start < RESET_HIGH);
			
			return true;
		}
		
		void SendBit(bool bit)
		{
			m_dq.Reset();
			DWT_Delay(WRITE_1_LOW);
			m_dq.Set(bit);
			DWT_Delay(TIME_SLOT - WRITE_1_LOW);
			m_dq.Set();
			DWT_Delay(RECOVERY);
		}
		
		bool ReadBit()
		{
			m_dq.Reset();
			DWT_Delay(READ_LOW);
			m_dq.Set();
			DWT_Delay(READ_DELAY);
			const bool bit = m_dq.Read();
			DWT_Delay(TIME_SLOT - READ_DELAY - READ_LOW + RECOVERY);
			
			return bit;
		}
		
		void WaitFor1(const uint32_t timeout)
		{
			const uint32_t start = HAL_GetTick();
			while (!ReadBit())
				if (HAL_GetTick() - start > timeout)
					return;
		}
		
		void SendByte(uint8_t byte)
		{
			for (uint8_t j = 0; j < 8; j++, byte >>= 1)
				SendBit(byte & 0b1);
		}
		
		uint8_t ReadByte()
		{
			uint8_t byte = 0;
			for (uint8_t i = 0; i < 8; i++)
				byte |= ReadBit() << i;
			
			return byte;
		}
		
		__attribute__((always_inline)) void SendCmd(const CMD cmd)
		{
			SendByte((uint8_t)cmd);
		}
		
		void SendBytes(const uint8_t * const buf, const uint8_t len)
		{
			for (uint8_t i = 0; i < len; i++)
			{
				for (uint8_t j = 0; j < 8; j++)
					SendByte(buf[i]);
			}
		}
		
		void ReadBytes(uint8_t * const buf, const uint8_t len)
		{
			for (uint8_t i = 0; i < len; i++)
				buf[i] = ReadByte();
		}
		
		/**
		* @param buf - Must at least be 9 bytes.
		*/
		bool CheckScratchpadCRCSingle(uint8_t * const buf)
		{
			if (!Init())
				return false;
			
			SendCmd(CMD::SKIP_ROM);
			SendCmd(CMD::READ_SCRATCHPAD);
			ReadBytes(buf, 9);
			
			return CRC8<TABLE>(buf, 9) == 0;
		}
		
	public:
		/**
		* @param dq - Must be open-drain and pulled up.
		*/
		DS18B20(IO dq) : m_dq(dq) {}
		
		std::optional<uint64_t> ReadSingleID()
		{
			if (!Init())
				return std::nullopt;
			
			SendCmd(CMD::READ_ROM);
			
			share_arr<uint64_t> id;
			ReadBytes(id.arr, sizeof(id));
			
			if (CRC8<TABLE>(id.arr, sizeof(id)) != 0)
				return std::nullopt;
			
			return id.val;
		}
		
		bool ConfigSingle(const uint8_t bits, const float low = 0, const float high = 0, const bool check_crc = true, const bool save_to_eeprom = false)
		{
			if (!Init())
				return false;
			
			SendCmd(CMD::SKIP_ROM);
			
			m_bits = std::clamp(bits, 9_u8, 12_u8);
			
			SendCmd(CMD::WRITE_SCRATCHPAD);
			SendByte((int8_t)(high * 16));
			SendByte((int8_t)(low * 16));
			SendByte((m_bits - 9) << 5);
			
			uint8_t buf[9];
			if (check_crc && !CheckScratchpadCRCSingle(buf))
				return false;
			
			if (save_to_eeprom)
			{
				if (!Init())
					return false;
				
				SendCmd(CMD::SKIP_ROM);
				SendCmd(CMD::COPY_SCRATCHPAD);
				WaitFor1(10);
			}
			
			return true;
		}
		
		/**
		* @retval The temperature multiplied by 16.
		*/
		std::optional<int16_t> ReadSingle(const bool check_crc = true)
		{
			uint8_t buf[9];
			if (check_crc)
			{
				if (!CheckScratchpadCRCSingle(buf))
					return std::nullopt;
			}
			else
			{
				if (!Init())
					return std::nullopt;
				
				SendCmd(CMD::SKIP_ROM);
				SendCmd(CMD::READ_SCRATCHPAD);
				ReadBytes(buf, 2);
			}
			
			return *(int16_t *)buf & (0xFFFF << (12 - m_bits));
		}
		
		/**
		* @retval The temperature multiplied by 16.
		*/
		std::optional<int16_t> ConvertAndReadSingle(const bool check_crc = true)
		{
			if (!Init())
				return std::nullopt;
			
			SendCmd(CMD::SKIP_ROM);
			SendCmd(CMD::CONVERT_T);
			WaitFor1(TIMEOUTS[m_bits - 9]);
			
			return ReadSingle(check_crc);
		}
	};
}
