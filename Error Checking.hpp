#pragma once

#include "./Common.hpp"
//#include "./Log.hpp"



namespace STM32T::CRCx
{
	/**
	* @param TABLE - Must have 256 values.
	* @note Base on https://www.sunshine2k.de/articles/coding/crc/understanding_crc.html#ch44
	*/
	template <const uint8_t * TABLE>
	inline uint8_t CRC8(const uint8_t * const data, const size_t len, uint8_t crc_init = 0)
	{
		for (size_t i = 0; i < len; i++)
			crc_init = TABLE[data[i] ^ crc_init];
		
		return crc_init;
	}
}

namespace STM32T::HC
{
	// todo: Optimize for 32-bit calculations.
	// todo: Move helper functions to a .cpp file so that they're not accessible.
	
	static constexpr inline uint32_t POW_2[24] =
	{
		1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7,
		1 << 8, 1 << 9, 1 << 10, 1 << 11, 1 << 12, 1 << 13, 1 << 14, 1 << 15,
		1 << 16, 1 << 17, 1 << 18, 1 << 19, 1 << 20, 1 << 21, 1 << 22, 1 << 23
	};
	
	/**
	* @retval Number of parity bits required for encoding bytes or 0 in case of error.
	*/
	inline uint8_t parity_bits(uint16_t bytes)
	{
		static constexpr uint32_t DATA_BITS[24] =
		{
			0, 0, 1, 4, 11, 26, 57, 120, 247, 502, 1013, 2036, 4083, 8178, 16369, 32752,
			65519, 131'054, 262'125, 524'268, 1'048'555, 2'097'130, 4'194'281, 8'388'584
		};
		
		const uint32_t bits = bytes * 8;
		
		const auto it = std::find_if(DATA_BITS, std::end(DATA_BITS), [bits](auto x) -> bool { return bits <= x; });
		return it == std::end(DATA_BITS) ? 0 : it - DATA_BITS;
	}
	
	/**
	* @brief Converts a one-based byte index to a zero-based Hamming index.
	*/
	inline uint32_t hamming_index(uint16_t byte_index)
	{
		return byte_index * 8 + parity_bits(byte_index);
	}
	
	/**
	* @brief Converts a one-based Hamming index to a one-based data index.
	*/
	inline uint32_t data_index(uint32_t hamming_index)
	{
		uint8_t extra = 0;
		while (hamming_index > POW_2[extra])
			extra++;
		
		return hamming_index - extra;
	}
	
	/**
	* @brief Returns a bitmask indicating which bits are covered by the nth parity bit at the specified byte index. All indices are zero-based.
	*/
	inline uint8_t mask(const uint16_t byte_index, const uint8_t parity_index)
	{
		uint32_t index = hamming_index(byte_index);
		
		uint8_t mask = 0;
		for (uint8_t bit = 0b1; bit; bit <<= 1)
		{
			index++;	// convert to one-based index for comparisons
			while (is_power_2(index))
				index++;
			
			if ((index & POW_2[parity_index]))
				mask |= bit;
		}
		
		return mask;
	}
	
	/**
	* @brief Calculates the XOR of all of the items in the array.
	*/
	template <typename T>
	T fold(const T * data, uint16_t len)
	{
		static_assert(std::is_integral_v<T>);
		
		T _xor = 0;
		
		if constexpr (sizeof(T) < sizeof(uint32_t))
		{
			uint16_t len2 = sizeof(uint32_t) - (size_t)data % sizeof(uint32_t);		// Might be 4 which is ok.
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
	
	/**
	* @retval { hamming code parity bits, number of parity bits }
	*/
	inline std::pair<uint16_t, uint8_t> calculate_normal(const strv data, bool odd = false)
	{
		const uint8_t parity_count = parity_bits(data.length());
		
		uint16_t bits = 0;
		for (uint8_t p = 0; p < parity_count; p++)
		{
			uint8_t _xor = 0;
			for (uint8_t i = 0; i < data.length(); i++)
				_xor ^= data[i] & mask(i, p);
			
			bits |= !(parity(_xor) == odd) << p;
		}
		
		return { bits, parity_count };
	}
	
	/**
	* @retval { extended hamming code parity bits, number of parity bits (excluding the extended bit) }
	*/
	inline std::pair<uint16_t, uint8_t> calculate_extended(const strv data, bool odd = false)
	{
		auto [bits, count] = calculate_normal(data, odd);
		
		// extended parity bit
		uint16_t _xor = bits ^ fold(data.data(), data.length());
		bits |= !(parity(_xor) == odd) << count;
		
		return { bits, count };
	}
	
	inline bool correct_extended(uint8_t * const data, const uint16_t len, const uint16_t parity, bool odd = false)
	{
		auto [calc, count] = calculate_normal(strv((char *)data, len), odd);
		
		uint16_t common = fold(data, len) ^ (parity & ~POW_2[count]);
		common = !(STM32T::parity(common) == odd) << count;
		
		const bool common_ok = common == (parity & POW_2[count]), parity_ok = calc == (parity & ~POW_2[count]);
		
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
		
		
		uint16_t diff = (calc ^ parity) & ~POW_2[count];
		if (bit_count(diff) == 1)	// the error is in the parity bits
			return true;
		
		uint32_t error_index = data_index(diff) - 1;	// the parity bits automatically calculate the one-based Hamming index
		data[error_index / 8] ^= 1 << (error_index % 8);

		return true;
	}
}
