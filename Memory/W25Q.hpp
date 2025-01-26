#pragma once

#include "../Common.hpp"
#include "../IO.hpp"



namespace STM32T
{
	/**
	* @note There are subtle differences between the different W25Q variants. This was written for W25Q128JVSQ but should work on other variants.
	*/
	class W25Q
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
			ERASE_HALF_BLOCK = 0x52,
			ERASE_BLOCK = 0xD8,
			ERASE_CHIP = 0xC7,
		};
		
		
		
		static constexpr addr_t NO_ADDRESS = 0xFFFF'FFFF;
		
		
		
		SPI_HandleTypeDef* const p_hspi;
		STM32T::IO m_CS;
		const addr_t c_MaxAddress;
		
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
			
			if (addr <= c_MaxAddress and HAL_SPI_Transmit(p_hspi, _addr, sizeof(_addr), HAL_MAX_DELAY) != HAL_OK)
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
		
		enum class ET : uint8_t { SECTOR = ERASE_SECTOR, HALF_BLOCK = ERASE_HALF_BLOCK, BLOCK = ERASE_BLOCK, CHIP = ERASE_CHIP };
		
		static addr_t Sector(const uint16_t sector_num) { return sector_num * SECTOR_SIZE; }
		
		/**
		* @param CS - This is usually active low and must be configured as such. The class won't handle the polarity of the pin.
		* @param size - The size of memory in Mbits.
		*/
		W25Q(SPI_HandleTypeDef* hspi, STM32T::IO CS, uint16_t size) : p_hspi(hspi), m_CS(CS), c_MaxAddress(size * 1_Ki * 1_Ki / 8 - 1) {}
		
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
			return addr <= c_MaxAddress and Read(READ_DATA, data, len, strv((char *)_addr, sizeof(_addr)));
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
		
		template <size_t BUF_SIZE = PAGE_SIZE>
		bool VerifyData(addr_t addr, const uint8_t * data, uint16_t len)
		{
			uint8_t buf[BUF_SIZE];
			
			while (len)
			{
				const uint16_t read_size = std::min((uint16_t)sizeof(buf), len);
				if (!ReadData(addr, buf, read_size) or memcmp(buf, data, read_size) != 0)
					return false;
				
				addr += read_size;
				data += read_size;
				len -= read_size;
			}
			
			return true;
		}
		
		/**
		* @param data - Should be at most 256 bytes (W25Q page size).
		* @note If addr is not aligned to PAGE_SIZE, it will wrap around to the start of the page.
		*/
		bool WritePage(const addr_t addr, const uint8_t * const data, const uint16_t len = 256)
		{
			return SingleByte(WRITE_ENABLE) and Write(PAGE_PROGRAM, addr, data, len) and BusyWait(3);
		}
		
		bool WriteData(const addr_t addr, const uint8_t * const data, const uint16_t len)
		{
			uint16_t written = 0;
			const addr_t offset = addr % PAGE_SIZE;
			
			if (offset != 0)
			{
				const uint16_t write_size = std::min((uint16_t)(PAGE_SIZE - offset), len);
				if (!WritePage(addr + written, data + written, write_size))
					return false;
				
				written += write_size;
			}
			
			uint16_t rem = len - written;
			if (!rem)
				return true;
			
			for (; rem > PAGE_SIZE; rem = len - written)
			{
				if (!WritePage(addr + written, data + written))
					return false;
				
				written += PAGE_SIZE;
			}
			
			return WritePage(addr + written, data + written, rem);
		}
		
		bool WriteVerifyData(const addr_t addr, const uint8_t * const data, const uint16_t len)
		{
			uint16_t written = 0;
			const addr_t offset = addr % PAGE_SIZE;
			
			if (offset != 0)
			{
				const uint16_t write_size = std::min((uint16_t)(PAGE_SIZE - offset), len);
				if (!WritePage(addr + written, data + written, write_size) or !VerifyData(addr + written, data + written, write_size))
					return false;
				
				written += write_size;
			}
			
			uint16_t rem = len - written;
			if (!rem)
				return true;
			
			for (; rem > PAGE_SIZE; rem = len - written)
			{
				if (!WritePage(addr + written, data + written) or !VerifyData(addr + written, data + written, PAGE_SIZE))
					return false;
				
				written += PAGE_SIZE;
			}
			
			return WritePage(addr + written, data + written, rem) and VerifyData(addr + written, data + written, rem);
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
		
		bool ModifySector(const uint16_t sector_num, const uint16_t offset, const uint8_t * const data, const uint16_t len)
		{
			const addr_t addr = Sector(sector_num);
			
			if (offset + len > SECTOR_SIZE)
				return false;
			
			if (VerifyData(addr + offset, data, len))
				return true;
			
			std::unique_ptr<uint8_t[]> buf(new uint8_t[SECTOR_SIZE]);
			if (!ReadData(addr, buf.get(), SECTOR_SIZE))
				return false;
			
			memcpy(buf.get() + offset, data, len);
			// fixme: If Erase() fails, the data is lost.
			return Erase(addr, ET::SECTOR) and WriteData(addr, buf.get(), SECTOR_SIZE);
		}
	};
}
