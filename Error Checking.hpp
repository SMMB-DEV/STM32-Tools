#pragma once

#include "./Core/Utils.hpp"
#include "./Core/strv.hpp"



namespace STM32T
{
	inline namespace EC
	{
		/**
		* @tparam TABLE - The CRC table. Must have 256 values.
		* @tparam T - The integer type used for calculating CRC. Determines the size of the CRC.
		*	It is the same type as the table elements and normally should not be specified.
		* @copyright Based on https://www.sunshine2k.de/articles/coding/crc/understanding_crc.html#ch44
		*/
		template <const auto (&TABLE)[256], typename T = std::remove_const_t<std::remove_reference_t<decltype(TABLE[0])>>>
		constexpr T TableCRC(const uint8_t *data, size_t len, T crc_init = 0, const bool inputReflected = false)
		{
			static_assert(std::is_same_v<T, std::remove_const_t<std::remove_reference_t<decltype(TABLE[0])>>>);
			static_assert(is_int_v<T>);
			
			while (len--)
			{
				const uint8_t curByte = (inputReflected ? reverse(*data) : *data);
				++data;
				
				const uint8_t pos = (crc_init >> (sizeof(T) - 1) * 8) ^ curByte;
				
				if constexpr (sizeof(T) > 1)
					crc_init = (crc_init << 8) ^ TABLE[pos];
				else
					crc_init = TABLE[pos];
			}

			return crc_init;
		}
		
		/**
		* @tparam T - The integer type used for calculating CRC. Determines the size of the CRC.
		* @param table - Must have 256 values.
		* @copyright Based on https://www.sunshine2k.de/articles/coding/crc/understanding_crc.html#ch44
		*/
		template <typename T>
		constexpr T TableCRC(const uint8_t *data, size_t len, const T table[256], T crc_init = 0, const bool inputReflected = false)
		{
			static_assert(is_int_v<T>);
			
			while (len--)
			{
				const uint8_t curByte = (inputReflected ? reverse(*data) : *data);
				++data;
				
				const uint8_t pos = (crc_init >> (sizeof(T) - 1) * 8) ^ curByte;
				
				if constexpr (sizeof(T) > 1)
					crc_init = (crc_init << 8) ^ table[pos];
				else
					crc_init = table[pos];
			}

			return crc_init;
		}
		
		template <typename T>
		[[deprecated("Use TableCRC() instead. Will be removed in v0.5.0.")]]
		inline T CRCx(const uint8_t *data, size_t len, const T table[256], T crc_init = 0, const bool inputReflected = false)
		{
			return TableCRC<T>(data, len, table, crc_init, inputReflected);
		}
		
		/**
		* @copyright Based on https://www.sunshine2k.de/articles/coding/crc/understanding_crc.html#ch43
		*/
		template <typename T, uint8_t WIDTH = sizeof(T) * 8>
		constexpr T SimpleCRC(const uint8_t *data, size_t len, T poly, T crc_init = 0, const bool inputReflected = false)
		{
			static_assert(is_int_v<T>);
			static_assert(WIDTH <= sizeof(T) * 8 && WIDTH > (sizeof(T) - 1) * 8);
			
			constexpr uint8_t
				bitsToShift = (WIDTH >= 8) ? WIDTH - 8 : 0,	// example CRC-11: 3
				bitOffsetTo8 = (WIDTH < 8) ? 8 - WIDTH : 0;
			
			constexpr T
				bitmask = (WIDTH >= 8) ? 1 << (WIDTH - 1) : 0x80,		// example CRC-11: 0x0400
				finalmask = (WIDTH >= 8) ? (1 << WIDTH) - 1 : 0xFF;		// example CRC-11: 0x07FF
			
			poly <<= bitOffsetTo8;
			crc_init <<= bitOffsetTo8;
			
			while (len--)
			{
				const uint8_t curByte = inputReflected ? reverse(*data) : *data;
				++data;
				
				crc_init ^= curByte << bitsToShift;		// move byte into MSB of CRC
				
				for (int j = 0; j < 8; ++j)
				{
					if (crc_init & bitmask)
						crc_init = (crc_init << 1) ^ poly;
					else
						crc_init <<= 1;
				}
				
				crc_init &= finalmask;
			}
			
			crc_init >>= bitOffsetTo8;
			
			return crc_init;
		}
	}
	
	class HC
	{
		/**
		* @brief Converts a one-based byte index to a zero-based Hamming index.
		*/
		static size_t hamming_index(uint32_t byte_index)
		{
			return byte_index * 8 + ParityBits(byte_index);
		}
		
		/**
		* @brief Converts a one-based Hamming index to a one-based data index.
		*/
		static uint32_t data_index(uint32_t hamming_index)
		{
			uint8_t extra = 0;
			while (hamming_index > (1u << extra))
				extra++;
			
			return hamming_index - extra;
		}
		
		/**
		* @brief Returns a bitmask indicating which bits are covered by the nth parity bit at the specified byte index. All indices are zero-based.
		*/
		static uint8_t mask(const uint32_t byte_index, const uint8_t parity_index)
		{
			size_t index = hamming_index(byte_index);
			
			uint8_t mask = 0;
			for (uint8_t bit = 0b1; bit; bit <<= 1)
			{
				++index;	// convert to one-based index for comparisons
				while (is_power_2(index))
					++index;
				
				if (index & (1u << parity_index))
					mask |= bit;
			}
			
			return mask;
		}
		
		/**
		* @brief Calculates the XOR of all of the items in the array.
		*/
		template <typename T>
		static T fold(const T *data, uint32_t len)
		{
			static_assert(std::is_integral_v<T>);
			
			T _xor = 0;
			
			if constexpr (sizeof(T) < sizeof(uint32_t))
			{
				uint32_t len2 = sizeof(uint32_t) - (size_t)data % sizeof(uint32_t);		// Might be 4 which is ok.
				if (len2 <= len)
				{
					len -= len2;
					while (len2)
					{
						_xor ^= *data++;
						len2 -= sizeof(T);
					}
				}
				
				// Aligned to 32 bits
				constexpr size_t FACTOR = sizeof(uint32_t) / sizeof(T);
				
				len2 = len / FACTOR;
				len -= len2 * FACTOR;
				
				uint32_t _xor2 = 0;
				while (len2--)
				{
					_xor2 ^= *(uint32_t*)data;
					data += FACTOR;
				}
				
				// shrink _xor2 to make sure it fits inside _xor
				_xor2 ^= _xor2 >> 16;
				
				if (sizeof(T) == sizeof(uint16_t))
				{
					_xor ^= _xor2 & 0xFFFF;
				}
				else	// 8 bits
				{
					_xor2 ^= _xor2 >> 8;
					_xor ^= _xor2 & 0xFF;
				}
			}
			
			while (len--)
				_xor ^= *data++;
			
			return _xor;
		}
		
	public:
		/**
		* @retval Number of parity bits required for encoding bytes.
		*/
		static constexpr uint8_t ParityBits(const uint32_t bytes)
		{
			const uint32_t data_bits = bytes * 8;
			if (data_bits == 0)
				return 0;
			
			uint8_t bits = 1;
			
			for (; bits < 32; ++bits)
				if (data_bits <= (1 << bits) - bits - 1)
					break;
			
			return bits;
		}
		
		/**
		* @retval { hamming code parity bits, number of parity bits }
		*/
		static constexpr std::pair<uint32_t, uint8_t> CalculateNormal(const strv data, const bool odd = false)
		{
			const uint8_t parity_count = ParityBits(data.length());
			
			uint32_t bits = 0;
			for (uint8_t p = 0; p < parity_count; ++p)
			{
				uint8_t _xor = 0;
				for (size_t i = 0; i < data.length(); ++i)
					_xor ^= data[i] & mask(i, p);
				
				bits |= !(parity(_xor) == odd) << p;
			}
			
			return { bits, parity_count };
		}
		
		/**
		* @retval { extended hamming code parity bits, number of parity bits (excluding the extended bit) }
		*/
		static constexpr std::pair<uint32_t, uint8_t> CalculateExtended(const strv data, const bool odd = false)
		{
			auto [bits, count] = CalculateNormal(data, odd);
			
			// extended parity bit
			uint32_t _xor = bits ^ fold(data.data(), data.length());
			bits |= !(parity(_xor) == odd) << count;
			
			return { bits, count };
		}
		
		static bool CorrectExtended(uint8_t *const data, const uint32_t len, const uint32_t parity_bits, bool odd = false)
		{
			const auto [calc, count] = CalculateNormal(strv((char *)data, len), odd);
			
			const uint32_t pow2 = 1 << count;
			uint32_t common = fold(data, len) ^ (parity_bits & ~pow2);
			common = !(parity(common) == odd) << count;
			
			const bool common_ok = common == (parity_bits & pow2), parity_ok = calc == (parity_bits & ~pow2);
			
			if (parity_ok)
			{
				if (common_ok)	// everything ok
					return true;
				else			// what is this state? error in common parity bit?
					return true;
			}
			else
			{
				if (common_ok)	// uncorrectable
					return false;
			}
			
			
			uint16_t diff = (calc ^ parity_bits) & ~pow2;
			if (bit_count(diff) == 1)	// the error is in the parity bits
				return true;
			
			uint32_t error_index = data_index(diff) - 1;	// the parity bits automatically calculate the one-based Hamming index
			data[error_index / 8] ^= 1 << (error_index % 8);

			return true;
		}
	};
}
