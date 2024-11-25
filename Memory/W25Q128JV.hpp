#pragma once

#include "../Common.hpp"
#include "../IO.hpp"



namespace STM32T
{
	/**
	* @brief The name is very specific because there are subtle differences between the different W25Q variants. This was written for W25Q128JVSQ but should work on other variants.
	*/
	class W25Q128JV
	{
	public:
		using addr_t = uint32_t;
		
	private:
		enum CMD : uint8_t
		{
			READ_JEDEC_ID = 0x9F,
			READ_UID = 0x4B,
			READ_DATA = 0x03,
			READ_STATUS1 = 0x05,
			READ_STATUS2 = 0x35,
			READ_STATUS3 = 0x15,
			
			WRITE_ENABLE = 0x06,
			PAGE_PROGRAM = 0x02,
			WRITE_STATUS1 = 0x01,
			WRITE_STATUS2 = 0x31,
			WRITE_STATUS3 = 0x11,
			
			ERASE_SECTOR = 0x20,
			ERASE_BLOCK32 = 0x52,
			ERASE_BLOCK64 = 0xD8,
			ERASE_CHIP = 0xC7,
		};
		
		
		
		static constexpr addr_t MAX_ADDRESS = 0x00FF'FFFF, NO_ADDRESS = 0xFFFF'FFFF;
		
		
		
		SPI_HandleTypeDef* const p_hspi;
		STM32T::IO m_CS;
		
		/**
		* @brief Send a single-byte instruction with no data.
		*/
		bool SingleByte(const CMD cmd)
		{
			m_CS.Set();
			const HAL_StatusTypeDef stat = HAL_SPI_Transmit(p_hspi, (uint8_t *)&cmd, 1, HAL_MAX_DELAY);
			m_CS.Reset();
			
			return stat == HAL_OK;
		}
		
		/**
		* @brief Send an instruction and read the returned data (might include dummy bytes before actual data).
		* @param buf - Can be null only if len is 0
		* @param len - Number of actual data bytes to receive
		* @param args - Arguments to send before reading data (e.g. address). Could also be used for reading dummy bytes.
		*/
		bool Read(const CMD cmd, uint8_t * const buf, const uint16_t len, const strv args = strv())
		{
			ScopeIO _cs(m_CS);
			
			if (HAL_SPI_Transmit(p_hspi, (uint8_t *)&cmd, 1, HAL_MAX_DELAY) != HAL_OK)
				return false;
			
			if (not args.empty() > 0 and HAL_SPI_Transmit(p_hspi, (uint8_t *)args.data(), args.size(), HAL_MAX_DELAY) != HAL_OK)
				return false;
			
			if (len > 0 and HAL_SPI_Receive(p_hspi, buf, len, HAL_MAX_DELAY) != HAL_OK)
				return false;
			
			return true;
		}
		
		/**
		* @brief Performs a write operation with an optional 24-bit address and optional data.
		*/
		bool Write(const CMD cmd, const addr_t addr, const uint8_t * const buf, const uint16_t len)
		{
			const uint8_t _addr[3] = { (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr };
			
			ScopeIO _cs(m_CS);
			
			if (HAL_SPI_Transmit(p_hspi, (uint8_t *)&cmd, 1, HAL_MAX_DELAY) != HAL_OK)
				return false;
			
			if (addr <= MAX_ADDRESS and HAL_SPI_Transmit(p_hspi, _addr, sizeof(_addr), HAL_MAX_DELAY) != HAL_OK)
				return false;
			
			if (len > 0 and HAL_SPI_Transmit(p_hspi, buf, len, HAL_MAX_DELAY) != HAL_OK)
				return false;
			
			return true;
		}
		
		/**
		* @retval False if there was an error or timeout
		*/
		bool BusyWait(const uint32_t timeout)
		{
			ScopeIO _cs(m_CS);
			
			const uint32_t start = HAL_GetTick();
			
			const CMD cmd = READ_STATUS1;
			if (HAL_SPI_Transmit(p_hspi, (uint8_t *)&cmd, 1, HAL_MAX_DELAY) != HAL_OK)
				return false;
			
			while (HAL_GetTick() - start <= timeout)
			{
				uint8_t stat;
				if (HAL_SPI_Receive(p_hspi, &stat, 1, HAL_MAX_DELAY) == HAL_OK and not(stat & 0b1))	// busy flag gone
					return true;
			}
			
			return false;
		}
		
	public:
		static constexpr size_t BLOCK_SIZE = 64_Ki, SECTOR_SIZE = 4_Ki, PAGE_SIZE = 256;
		
		enum class ET : uint8_t { SECTOR = ERASE_SECTOR, BLOCK32 = ERASE_BLOCK32, BLOCK64 = ERASE_BLOCK64, CHIP = ERASE_CHIP };
		
		/**
		* @param CS - This is usually active low and must be configured as such. The class won't handle the polarity of the pin.
		*/
		W25Q128JV(SPI_HandleTypeDef* hspi, STM32T::IO CS) : p_hspi(hspi), m_CS(CS) {}
		
		/**
		* @retval JEDEC ID of the device or 0 in case of failure.
		*/
		uint32_t ReadJEDECID()
		{
			uint8_t buf[3] = {0};
			Read(READ_JEDEC_ID, buf, 3);
			return (buf[0] << 16) | (buf[1] << 8) | buf[2];
		}
		
		/**
		* @retval Unique ID of the device or 0 in case of failure.
		*/
		uint64_t ReadUID()
		{
			uint64_t id = 0;
			Read(READ_UID, (uint8_t *)&id, sizeof(id), "\0\0\0\0"sv);
			return STM32T::le2be(id);
		}
		
		/**
		* @param addr - 24-bit address for data.
		* @retval True if len bytes were successfully read.
		*/
		bool ReadData(const addr_t addr, uint8_t * const data, const uint16_t len)
		{
			const uint8_t _addr[3] = { (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr };
			return addr <= MAX_ADDRESS and Read(READ_DATA, data, len, strv((char *)_addr, sizeof(_addr)));
		}
		
		template <class T>
		bool Read(const addr_t addr, T& t)
		{
			return ReadData(addr, (uint8_t *)&t, sizeof(T));
		}
		
		bool ReadStatus(uint32_t& status)
		{
			uint8_t stat[4] = {0};
			
			if (Read(READ_STATUS1, stat, 1) and Read(READ_STATUS2, stat + 1, 1) and Read(READ_STATUS3, stat + 2, 1))
			{
				status = *(uint32_t*)stat;
				return true;
			}
			
			return false;
		}
		
		/**
		* @param len - If zero, considered 256. Int narrowing will take care of making sure it's not more than 256 (W25Q page size).
		*/
		bool WriteData(const addr_t addr, const uint8_t * const data, const uint8_t len)
		{
			return SingleByte(WRITE_ENABLE) and Write(PAGE_PROGRAM, addr, data, len ? len : 256) and BusyWait(3);
		}
		
		template <class T>
		bool Write(const addr_t addr, const T& t)
		{
			return WriteData(addr, (uint8_t *)&t, sizeof(T));
		}
		
		bool WriteStatus(const uint32_t status)
		{
			const uint8_t stat[3] = { (uint8_t)(status >> 16), (uint8_t)(status >> 8), (uint8_t)status };
			return SingleByte(WRITE_ENABLE) and Write(WRITE_STATUS1, NO_ADDRESS, (uint8_t *)&status, 2) and Write(WRITE_STATUS3, NO_ADDRESS, (uint8_t *)(&status) + 2, 1) and BusyWait(15);
		}
		
		bool Erase(const addr_t addr, const ET erase_type)
		{
			static constexpr uint32_t DELAY[16] = { 400, 0, 1600, 0, 0, 0, 0, 200'000, 2000, 0 };	// lookup table for max delay
			return SingleByte(WRITE_ENABLE) and Write((CMD)erase_type, addr, nullptr, 0) and BusyWait(DELAY[(uint8_t)erase_type & 0x0F]);
		}
	};
}
