/// @file

#pragma once

#include "../../IO.hpp"
#include "../../Core/strv.hpp"
#include "../../Core/span.hpp"
#include "../../Core/Time.hpp"
#include "../../EDAC.hpp"

#include <optional>



namespace STM32T::RFID
{
	static constexpr uint8_t
		b1 = 0b0000'0001, b2 = 0b0000'0010, b3 = 0b0000'0100, b4 = 0b0000'1000, b5 = 0b0001'0000, b6 = 0b0010'0000, b7 = 0b0100'0000, b8 = 0b1000'0000,
		b4321 = b4 | b3 | b2 | b1, b8765 = b8 | b7 | b6 | b5;
	
	class PICC;
	
	/**
	* @copyright This is mostly taken from the <a href="https://github.com/miguelbalboa/rfid">Arduino RFID library</a>
	*		which uses the <a href="https://unlicense.org/">"Unlicense"</a>.
	*		I'm not sure what that means when I'm using the MIT license.
	*/
	class MFRC522
	{
		// todo: adjust all SPI timeouts
		friend class PICC;
		
		SPI_HandleTypeDef *p_spi;
		IO m_cs;
		
		enum class Reg : uint8_t
		{
			// Page 0: Command and status
			Reserved = 0x00, Command, ComlEn, DivlEn, ComIrq, DivIrq, Error, Status1, Status2, FIFOData, FIFOLevel, WaterLevel, Control, BitFraming, Coll,
			
			// Page 1: Command
			Mode = 0x11, TxMode, RxMode, TxControl, TxASK, TxSel, RxSel, RxThreshold, Demod, MfTx = 0x1C, MfRx, SerialSpeed = 0x1F,
			
			// Page 2: Configuration
			CRCResultH = 0x21, CRCResultL, ModWidth = 0x24, RFCfg = 0x26, GsN, CWGsP, ModGs, TMode, TPrescaler, TReloadH, TReloadL, TCounterValH, TCounterValL,
			
			// Page 3: Test register
			TestSel1 = 0x31, TestSel2, TestPinEn, TestPinValue, TestBus, AutoTest, Version, AnalogTest, TestDAC1, TestDAC2, TestADC
		};
		
		enum class Cmd : uint8_t
		{
			Idle = 0b0000, Mem, GenRandomID, CalcCRC, Transmit, NoCmdChange = 0b0111, Receive, Transceive = 0b1100, MFAuthent = 0b1110, SoftReset
		};
		
		enum CmdPICC : uint8_t
		{
			// The commands used by the PCD to manage communication with several PICCs (ISO 14443-3, Type A, section 6.4)
			REQA			= 0x26,		// REQuest command, Type A. Invites PICCs in state IDLE to go to READY and prepare for anticollision or selection. 7 bit frame.
			WUPA			= 0x52,		// Wake-UP command, Type A. Invites PICCs in state IDLE and HALT to go to READY(*) and prepare for anticollision or selection. 7 bit frame.
			CT				= 0x88,		// Cascade Tag. Not really a command, but used during anti collision.
			SEL_CL1			= 0x93,		// Anti collision/Select, Cascade Level 1
			SEL_CL2			= 0x95,		// Anti collision/Select, Cascade Level 2
			SEL_CL3			= 0x97,		// Anti collision/Select, Cascade Level 3
			HLTA			= 0x50,		// HaLT command, Type A. Instructs an ACTIVE PICC to go to state HALT.
			RATS			= 0xE0,     // Request command for Answer To Reset.
			// The commands used for MIFARE Classic (from http://www.mouser.com/ds/2/302/MF1S503x-89574.pdf, Section 9)
			// Use PCD_MFAuthent to authenticate access to a sector, then use these commands to read/write/modify the blocks on the sector.
			// The read/write commands can also be used for MIFARE Ultralight.
			MF_AUTH_KEY_A	= 0x60,		// Perform authentication with Key A
			MF_AUTH_KEY_B	= 0x61,		// Perform authentication with Key B
			MF_READ			= 0x30,		// Reads one 16 byte block from the authenticated sector of the PICC. Also used for MIFARE Ultralight.
			MF_WRITE		= 0xA0,		// Writes one 16 byte block to the authenticated sector of the PICC. Called "COMPATIBILITY WRITE" for MIFARE Ultralight.
			MF_DECREMENT	= 0xC0,		// Decrements the contents of a block and stores the result in the internal data register.
			MF_INCREMENT	= 0xC1,		// Increments the contents of a block and stores the result in the internal data register.
			MF_RESTORE		= 0xC2,		// Reads the contents of a block into the internal data register.
			MF_TRANSFER		= 0xB0,		// Writes the contents of the internal data register to a block.
			// The commands used for MIFARE Ultralight (from http://www.nxp.com/documents/data_sheet/MF0ICU1.pdf, Section 8.6)
			// The PICC_CMD_MF_READ and PICC_CMD_MF_WRITE can also be used for MIFARE Ultralight.
			UL_WRITE		= 0xA2		// Writes one 4 byte page to the PICC.
		};
		
		enum class Status : uint8_t
		{
			OK				,	// Success
			ERROR			,	// Error in communication
			COLLISION		,	// Collission detected
			TIMEOUT			,	// Timeout in communication.
			NO_ROOM			,	// A buffer is not big enough.
			INTERNAL_ERROR	,	// Internal error in the code. Should not happen ;-)
			INVALID			,	// Invalid argument.
			CRC_WRONG		,	// The CRC_A does not match
			MIFARE_NACK		= 0xff	// A MIFARE PICC responded with NAK.
		};
		
		struct ATS
		{
			static constexpr size_t FS[16] = { 16, 24, 32, 40, 48, 64, 96, 128, 256, 257, 257, 257, 257, 257, 257, 257 };
			
			static constexpr uint8_t DEFAULT_FWI = 4;
			
			using duration_t = std::chrono::duration<uint32_t, std::ratio<1, 13'560'000>>;
			
			uint8_t TL;
			std::optional<uint8_t> T0, TA, TB, TC;
			std::vector<uint8_t> T1k;
			
			// Frame Size
			constexpr uint8_t FSCI() const { return T0.value_or(0) & b4321; }
			constexpr size_t FSC() const { return FS[FSCI()]; }
			
			// DS, DR
			constexpr uint8_t DS() const { return (TA.value_or(0) >> 4) & 0b0111; }
			constexpr uint8_t DR() const { return TA.value_or(0) & 0b0111; }
			
			// Frame Waiting (PCD req -> PICC resp)
			constexpr uint8_t FWI() const { return TB ? *TB >> 4 : DEFAULT_FWI; }
			constexpr duration_t FWT() const { return duration_t(256 * 16 * pow(2, FWI())); }
			
			// Start-up Frame Guard Time (PICC ATS -> PCD req)
			constexpr uint8_t SFGI() const { return TB.value_or(0) & 0b1111; }
			constexpr duration_t SFGT() const
			{
				auto sfgi = SFGI();
				return duration_t{sfgi ? 256 * 16 * pow(2, sfgi) : 9 * 128 + 84};	// todo: check if this is correct.
			}
			
			template <Log::Level level = Log::Level::Debug>
			void Log() const
			{
				using namespace STM32T::Log::DefLog;
				using namespace std::chrono;
				
				LOG<level>("ATS:\n\tTL: %hhu", TL);
				
				if (T0)
					LOG_N<level>("\n\tT0: %#.8hhb (FSC: %zu)", *T0, FSC());
				else
					LOG_N<level>("\n\tT0: N/A");
				
				if (TA)
					LOG_N<level>("\n\tTA: %#.8hhb", *TA);
				else
					LOG_N<level>("\t\nTA: N/A");
				
				if (TB)
					LOG_N<level>("\n\tTB: %#.8hhb (FWT: %.1f ms, SFGT: %.1f ms)",
					*TB, duration_cast<duration<float, std::milli>>(FWT()).count(), duration_cast<duration<float, std::milli>>(SFGT()).count());
				else
					LOG_N<level>("\n\tTB: N/A");
				
				if (TC)
					LOG_N<level>("\n\tTC: %#.8hhb", *TC);
				else
					LOG_N<level>("\n\tTC: N/A");
				
				LOG_N<level>("\nHistorical bytes (%zu):\n", T1k.size());
				LOGA<level>(T1k.data(), T1k.size());
			}
		};
		
		static constexpr uint8_t Addr4Read(const Reg reg) { return 0b1000'0000 | (static_cast<uint8_t>(reg) << 1); }
		static constexpr uint8_t Addr4Write(const Reg reg) { return static_cast<uint8_t>(reg) << 1; }
		
		static constexpr strv StatusName(Status stat)
		{
			switch (stat)
			{
				case Status::OK:				return "OK"sv;
				case Status::ERROR:				return "ERROR"sv;
				case Status::COLLISION:			return "COLLISION"sv;
				case Status::TIMEOUT:			return "TIMEOUT"sv;
				case Status::NO_ROOM:			return "NO_ROOM"sv;
				case Status::INTERNAL_ERROR:	return "INTERNAL_ERROR"sv;
				case Status::INVALID:			return "INVALID"sv;
				case Status::CRC_WRONG:			return "CRC_WRONG"sv;
				case Status::MIFARE_NACK:		return "MIFARE_NACK"sv;
				default:						return "UNKNOWN"sv;
			}
		}
		
		uint8_t Read(const Reg reg)
		{
			uint8_t send[2] = { Addr4Read(reg), 0 }, recv[2] = { 0 };
			
			m_cs.Reset();
			HAL_SPI_TransmitReceive(p_spi, send, recv, 2, HAL_MAX_DELAY);
			m_cs.Set();
			
			return recv[1];
		}
		
		/**
		* @param count - must be <= 64.
		*/
		void Read(const Reg reg, uint8_t *vals, const size_t count)
		{
			if (count > 64 || count < 1)
				return;
			
			uint8_t addr[64];
			memset(addr, Addr4Read(reg), count - 1);
			addr[count - 1] = 0;
			
			m_cs.Reset();
			
			HAL_SPI_Transmit(p_spi, addr, 1, HAL_MAX_DELAY);
			HAL_SPI_TransmitReceive(p_spi, addr, vals, count, HAL_MAX_DELAY);
			
			m_cs.Set();
		}
		
		void Read(Reg reg, uint8_t *vals, uint8_t count, uint8_t rxAlign)
		{
			if (count > 64 || count < 2)
				return;
			
			uint8_t addr[64];
			memset(addr, Addr4Read(reg), count - 2);
			addr[count - 2] = 0;
			
			m_cs.Reset();
			
			HAL_SPI_Transmit(p_spi, addr, 1, HAL_MAX_DELAY);
			
			uint8_t first, mask = 0xFF << rxAlign;
			HAL_SPI_TransmitReceive(p_spi, addr, &first, 1, HAL_MAX_DELAY);
			vals[0] = (vals[0] & ~mask) | (first & mask);
			
			HAL_SPI_TransmitReceive(p_spi, addr, vals + 1, count - 1, HAL_MAX_DELAY);
			
			m_cs.Set();
		}
		
		void Write(const Reg reg, const uint8_t val)
		{
			uint8_t send[2] = { Addr4Write(reg), val };
			
			m_cs.Reset();
			HAL_SPI_Transmit(p_spi, send, 2, HAL_MAX_DELAY);
			m_cs.Set();
		}
		
		void Write(const Reg reg, const uint8_t *const vals, const size_t count)
		{
			uint8_t addr = Addr4Write(reg);
			
			m_cs.Reset();
			HAL_SPI_Transmit(p_spi, &addr, 1, HAL_MAX_DELAY);
			HAL_SPI_Transmit(p_spi, vals, count, HAL_MAX_DELAY);
			m_cs.Set();
		}
		
		void WriteBits(const Reg reg, const uint8_t mask, uint8_t bits)
		{
			bits &= mask;
			const uint8_t val = Read(reg);
			
			if ((val & mask) != bits)
				Write(reg, (val & ~mask) | bits);
		}
		
		void SetBits(const Reg reg, const uint8_t mask)
		{
			WriteBits(reg, mask, 0xFF);
		}
		
		void ClearBits(const Reg reg, const uint8_t mask)
		{
			WriteBits(reg, mask, 0);
		}
		
		bool Wait(const Reg reg, const uint8_t val, const uint16_t timeout_us)
		{
			const uint32_t start = Time::GetCycle();
			while (Read(reg) != val)
			{
				if (Time::Elapsed_us(start, timeout_us))
					return false;
			}
			
			return true;
		}
		
		bool WaitForBits(const Reg reg, const uint8_t mask, const uint8_t bits, const uint16_t timeout_us)
		{
			const uint32_t start = Time::GetCycle();
			while ((Read(reg) & mask) != bits)
			{
				if (Time::Elapsed_us(start, timeout_us))
					return false;
			}
			
			return true;
		}
		
		void Command(Cmd cmd)
		{
			// todo: Tranceive command doesn't need to be written every time.
			Write(Reg::Command, static_cast<uint8_t>(cmd));
		}
		
		//------------------------------------------------------------------------------------------------------------------------------------------------------
		
		static constexpr uint16_t CalculateCRC(const uint8_t *const data, const uint8_t length)
		{
			return reverse(SimpleCRC<uint16_t>(data, length, 0x1021, 0xC6C6, true));
		}
		
		/**
		* Transfers data to the MFRC522 FIFO, executes a command, waits for completion and transfers data back from the FIFO.
		* CRC validation can only be done if backData and backLen are specified.
		*
		* @param command			The command to execute. One of the PCD_Command enums.
		* @param send				The data to transfer to the FIFO. @todo make the extent static?
		* @param backData			nullptr or pointer to buffer if data should be read back after executing the command.
		* @param[inout] backLen		Max number of bytes to write to *backData / The number of bytes returned.
		* @param waitIRq			The bits in the ComIrqReg register that signals successful completion of the command.
		* @param[inout] validBits	The number of valid bits in the last byte. 0 for 8 valid bits.
		* @param[in] rxAlign		Defines the bit position in backData[0] for the first bit received. Default 0.
		* @param[in] checkCRC		True => The last two bytes of the response is assumed to be a CRC_A that must be validated.
		* 
		* @retval STATUS_OK on success
		* @retval STATUS_??? otherwise.
		*/
		Status PCD_CommunicateWithPICC(Cmd command, const span<const uint8_t> send, uint8_t *backData, uint8_t *backLen,
			uint8_t waitIRq, uint8_t *validBits = nullptr, uint8_t rxAlign = 0, bool checkCRC = false, const uint32_t timeout = 36)
		{
			// Prepare values for BitFramingReg
			uint8_t txLastBits = validBits ? *validBits : 0;
			uint8_t bitFraming = (rxAlign << 4) + txLastBits;		// RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]
			
			Command(Cmd::Idle);										// Stop any active command.
			Write(Reg::ComIrq, 0x7F);								// Clear all seven interrupt request bits
			Write(Reg::FIFOLevel, 0x80);							// FlushBuffer = 1, FIFO initialization
			Write(Reg::FIFOData, send.data(), send.size());			// Write sendData to the FIFO
			Write(Reg::BitFraming, bitFraming);						// Bit adjustments
			Command(command);										// Execute the command
			if (command == Cmd::Transceive)
				SetBits(Reg::BitFraming, 0x80);						// StartSend=1, transmission of data starts
			
			// Wait here for the command to complete. The bits specified in the
			// `waitIRq` parameter define what bits constitute a completed command.
			// When they are set in the ComIrqReg register, then the command is
			// considered complete.
			
			// todo: Take into account the tranmission time.
			const uint32_t deadline_start = HAL_GetTick();
			bool completed = false;
			
			do
			{
				uint8_t n = Read(Reg::ComIrq);		// ComIrqReg[7..0] bits are: Set1 TxIRq RxIRq IdleIRq HiAlertIRq LoAlertIRq ErrIRq TimerIRq
				if (n & waitIRq)					// One of the interrupts that signal success has been set.
				{
					completed = true;
					break;
				}
			}
			while (HAL_GetTick() - deadline_start <= timeout);

			// Nothing happened. Communication with the MFRC522 might be down.
			if (!completed)
				return Status::TIMEOUT;
			
			// Stop now if any errors except collisions were detected.
			uint8_t errorRegValue = Read(Reg::Error);	// ErrorReg[7..0] bits are: WrErr TempErr reserved BufferOvfl CollErr CRCErr ParityErr ProtocolErr
			if (errorRegValue & 0x13)					// BufferOvfl ParityErr ProtocolErr
				return Status::ERROR;
		  
			uint8_t _validBits = 0;
			
			// If the caller wants data back, get it from the MFRC522.
			if (backData && backLen)
			{
				uint8_t n = Read(Reg::FIFOLevel);	// Number of bytes in the FIFO
				if (n > *backLen)
					return Status::NO_ROOM;
				
				*backLen = n;								// Number of bytes returned
				Read(Reg::FIFOData, backData, n, rxAlign);	// Get received data from FIFO
				_validBits = Read(Reg::Control) & 0x07;		// RxLastBits[2:0] indicates the number of valid bits in the last received byte. If this value is 000b, the whole byte is valid.
				if (validBits)
					*validBits = _validBits;
			}
			
			// Tell about collisions
			if (errorRegValue & 0x08)		// CollErr
				return Status::COLLISION;
			
			// Perform CRC_A validation if requested.
			if (backData && backLen && checkCRC)
			{
				// In this case a MIFARE Classic NAK is not OK.
				if (*backLen == 1 && _validBits == 4)
					return Status::MIFARE_NACK;
				
				// We need at least the CRC_A value and all 8 bits of the last byte must be received.
				if (*backLen < 2 || _validBits != 0)
					return Status::CRC_WRONG;
				
				// Verify CRC_A - do our own calculation and store the control in controlBuffer.
				const uint16_t control = CalculateCRC(&backData[0], *backLen - 2);
				
				if (pack_le<uint16_t>(&backData[*backLen - 2]) != control)
					return Status::CRC_WRONG;
			}
			
			return Status::OK;
		}
		
		Status PCD_CommunicateWithPICC(Cmd command, span<uint8_t> send, uint8_t *backData, uint8_t *backLen,
			uint8_t waitIRq, uint8_t *validBits = nullptr, uint8_t rxAlign = 0, bool checkCRC = false, bool addCRC = false, const uint32_t timeout = 36)
		{
			if (addCRC)
			{
				if (send.size() < 2)
					return Status::INVALID;
				
				unpack_le(send.end() - 2, CalculateCRC(send.data(), send.size() - 2));
			}
			
			return PCD_CommunicateWithPICC(command, span<const uint8_t>(send), backData, backLen, waitIRq, validBits, rxAlign, checkCRC, timeout);
		}
		
		/**
		* Transmits REQA or WUPA commands.
		* Beware: When two PICCs are in the field at the same time I often get STATUS_TIMEOUT - probably due do bad antenna design.
		* 
		* @param command	The command to send - PICC_CMD_REQA or PICC_CMD_WUPA
		* @return A pair of {STATUS_OK on success, STATUS_??? otherwise, The ATQA returned or 0 in case of error}.
		*/
		Status PICC_REQA_or_WUPA(uint8_t command, uint16_t *atqa)
		{
			uint8_t bufferSize = sizeof(*atqa);
			
			ClearBits(Reg::Coll, 0x80);					// ValuesAfterColl=1 => Bits received after collision are cleared.
			uint8_t validBits = 7;						// For REQA and WUPA we need the short frame format - transmit only 7 bits of the last (and only) byte. TxLastBits = BitFramingReg[2..0]
			const Status status = PCD_CommunicateWithPICC(Cmd::Transceive, span<const uint8_t>{&command, 1u}, reinterpret_cast<uint8_t *>(&atqa), &bufferSize, 0x30, &validBits);
			if (status != Status::OK)	// todo: check COLLISION as well?
				return status;
			
			if (bufferSize != 2 || validBits != 0)		// ATQA must be exactly 16 bits.
				return Status::ERROR;
			
			return Status::OK;
		}
		
		Status PICC_Select(PICC *uid, uint8_t validBits = 0);
		Status PICC_RATS(ATS *ats, uint8_t FSDI = 5, uint8_t CID = 0)
		{
			uint8_t cmd[4] = { 0xE0, static_cast<uint8_t>((FSDI << 4) | (CID & 0xF)) }, resp[64], respLen = sizeof(resp);
			
			if (const Status stat = PCD_CommunicateWithPICC(Cmd::Transceive, cmd, resp, &respLen, 0x30, nullptr, 0, true, true); stat != Status::OK)
				return stat;
			
			if (respLen < 3 || respLen - 2 != resp[0])
				return Status::ERROR;
			
			ats->TL = resp[0];
			
			if (ats->TL == 1)
				return Status::OK;
			
			ats->T0 = resp[1];
			
			const bool hasTA = *(ats->T0) & b5, hasTB = *(ats->T0) & b6, hasTC = *(ats->T0) & b7;
			const uint8_t nonHistLen = 2 + hasTA + hasTB + hasTC;
			
			if (*(ats->T0) & b8 || ats->TL < nonHistLen)
				return Status::ERROR;
			
			if (hasTA) ats->TA = resp[2];
			if (hasTB) ats->TB = resp[2 + hasTA];
			if (hasTC) ats->TC = resp[2 + hasTA + hasTB];
			
			if (ats->TL - nonHistLen)
				ats->T1k.assign(resp + nonHistLen, resp + ats->TL);
			
			return Status::OK;
		}
		
		Status PICC_PPS(std::optional<uint8_t> DSI_DRI, uint8_t CID = 0)
		{
			uint8_t
				cmd[5] = { static_cast<uint8_t>(0b1101'0000 | (CID & 0b111)), static_cast<uint8_t>(0b0000'0001 | (bool(DSI_DRI) << 4)) },
				len = 2;
			
			if (DSI_DRI)
				cmd[len++] = *DSI_DRI & 0b0000'1111;
			
			len += 2;	// CRC
			
			uint8_t resp[3], respLen = sizeof(resp);
			
			const Status stat = PCD_CommunicateWithPICC(Cmd::Transceive, {cmd, size_t(len)}, resp, &respLen, 0x30, nullptr, 0, true, true);
			if (stat != Status::OK)
				return stat;
			
			if (respLen != sizeof(resp) || resp[0] != cmd[0])
				return Status::ERROR;
			
			return Status::OK;
		}
		
	public:
		static constexpr size_t SELF_TEST_RESULT_SIZE = 64;
		
		MFRC522(SPI_HandleTypeDef *spi, IO cs) : p_spi(spi), m_cs(cs) {}
		
		void Init()
		{
			Time::Init();
			
			Read(Reg::Reserved);
			
			Write(Reg::ModWidth, 0x26);
			Write(Reg::RxThreshold, 0x14);
			
			Write(Reg::TxASK, 0x40);				// Default 0x00. Force a 100 % ASK modulation independent of the ModGsPReg register setting
			Write(Reg::Mode, 0x3D);					// Default 0x3F. Set the preset value for the CRC coprocessor for the CalcCRC command to 0x6363 (ISO 14443-3 part 6.2.4)
			WriteBits(Reg::TxControl, 0b11, 0b11);	// Enable the antenna driver pins TX1 and TX2 (they were disabled by the reset)
		}
		
		bool SoftReset()
		{
			Command(Cmd::SoftReset);
			HAL_Delay(1);
			return WaitForBits(Reg::Command, 0b0001'0000, 0, 38);	// fixme: If powered down, returns 0 anyways.
		}
		
		void SelfTest(uint8_t (&result)[SELF_TEST_RESULT_SIZE])
		{
			SoftReset();
			
			uint8_t ZEROS[25] = { 0 };
			Write(Reg::FIFOData, ZEROS, std::size(ZEROS));
			Command(Cmd::Mem);
			Write(Reg::AutoTest, 0b0000'1001);
			Write(Reg::FIFOData, 0);
			Command(Cmd::CalcCRC);
			Wait(Reg::FIFOLevel, 64, 1'000);
			
			Command(Cmd::Idle);
			Read(Reg::FIFOData, result, SELF_TEST_RESULT_SIZE);
			Write(Reg::AutoTest, 0b0100'0000);
		}
		
		uint8_t Version()
		{
			return Read(Reg::Version);
		}
		
		/**
		* @retval The ATQA if there are any new (IDLE state) PICCs in the field.
		*/
		/*std::optional<uint16_t> IsIdlePICC()
		{
			// Reset baud rates
			Write(Reg::TxMode, 0);
			Write(Reg::RxMode, 0);
			// Reset ModWidthReg
			Write(Reg::ModWidth, 0x26);
			
			uint16_t atqa;
			const Status stat = PICC_REQA_or_WUPA(CmdPICC::REQA, &atqa);
			return (stat == Status::OK || stat == Status::COLLISION) ? std::make_optional(atqa) : std::nullopt;
		}*/
		
		/// Sends REQA command and automatically selects a PICC (if available).
		std::optional<PICC> SelectPICC();
	};
	
	struct PICC
	{
		enum class Type : uint8_t
		{
			UNKNOWN,
			INCOMPLETE,
			MF_CL_1K, MF_CL_2K, MF_CL_4K,
			MF_CL_1K_SMX, MF_CL_4K_SMX,
			MF_UL_EV1, MF_UL_AES,
			MF_PL_2K_SL2, MF_PL_4K_SL2,
			MF_PL_EV1_2K_SL3, MF_PL_EV1_4K_SL3,
			MF_PL_EV2_2K_SL3, MF_PL_EV2_4K_SL3,
			MF_DF_EV1_256B, MF_DF_EV1_2K, MF_DF_EV1_4K, MF_DF_EV1_8K,
			MF_DF_EV2_2K, MF_DF_EV2_4K, MF_DF_EV2_8K, MF_DF_EV2_16K, MF_DF_EV2_32K,
			MF_DF_EV3_2K, MF_DF_EV3_4K, MF_DF_EV3_8K,
			MF_DFL,
			MF_DUOX_2K, MF_DUOX_4K, MF_DUOX_8K, MF_DUOX_16K,
			MF_MINI,
			NTAG213, NTAG215, NTAG216,
			NTAG_I2C_PL,
			NTAG4xx,
			NTAG_X_DNA,
			TNP,					// TagNPlay (TNP3xxx)
			
			//ISO_14443_4		,		// PICC compliant with ISO/IEC 14443-4 
			//ISO_18092		, 		// PICC compliant with ISO/IEC 18092 (NFC)
		};
		
		PICC & operator=(const PICC &other)
		{
			m_type = other.m_type;
			m_sak = other.m_sak;
			m_idLen = other.m_idLen;
			memcpy(m_id, other.m_id, m_idLen);
			
			return *this;
		}
		
		bool operator==(const PICC &other) const
		{
			return m_type == other.m_type && m_sak == other.m_sak && m_idLen == other.m_idLen && memcmp(m_id, other.m_id, m_idLen) == 0;
		}
		
		bool operator!=(const PICC &other) const { return !operator==(other); }
		
		/**
		* Determines whether the PICC supports ISO 14443-4 based on the SAK.
		* @note Some PICCs support ISO 14443-4 despite the SAK not showing it.
		*/
		constexpr bool isISO_14443_4() const { return m_sak & b6; }
		
		span<const uint8_t> id() const { return {m_id, size_t(m_idLen)}; }
		
		uint8_t sak() const { return m_sak; }
		
		Type type() const { return m_type; }
		
		/**
		* Based on NXP AN10833 Fig. 1. Sets the type of the PICC. It can only be called once after the PICC is selected.
		* 
		* @note The type of the PICC is determined partly based on the SAK returned after selecting it.
		* The SAK contains proprietary information and the actual type of the PICC cannot be guaranteed in some cases.
		* @note Modifies the state of the PICC. It might send RATS and PPS commands which prohibits you from sending them again.
		*/
		void CheckType()
		{
			if (m_sak & b3 || !m_idLen)
			{
				m_type = Type::INCOMPLETE;
				return;
			}
			
			m_type = Type::UNKNOWN;
			
			if (m_sak & b2)	// non-NXP
				return;
			
			if (m_sak & b4)							// 0bxxxx'100x
			{
				if (m_sak & b5)						// 0bxxx1'100x
				{
					if (m_sak & b1)					// 0bxxx1'1001
					{
						if (m_sak == 0x19)
							m_type = Type::MF_CL_2K;
					}
					else							// 0bxxx1'1000
					{
						if (m_sak & b6)				// 0bxx11'1000
						{
							if (m_sak == 0x38)
								m_type = Type::MF_CL_4K_SMX;
						}
						else						// 0bxx01'1000
						{
							MFRC522::ATS ats;
							
							if (m_frontend.PICC_RATS(&ats) == MFRC522::Status::OK)
								return;		// todo: GET_VERSION_L4
							
							if (m_sak == 0x18)
								m_type = Type::MF_CL_4K;
						}
					}
				}
				else								// 0bxxx0'100x
				{
					if (m_sak & b1)					// 0bxxx0'1001
					{
						if (m_sak == 0x09)
							m_type = Type::MF_MINI;
					}
					else							// 0bxxx0'1000
					{
						if (m_sak & b6)				// 0bxx10'1000
						{
							if (m_sak == 0x28)
								m_type = Type::MF_CL_1K_SMX;
						}
						else						// 0bxx00'1000
						{
							MFRC522::ATS ats;
							
							if (m_frontend.PICC_RATS(&ats) == MFRC522::Status::OK)
								return;		// todo: GET_VERSION_L4
							
							if (m_sak == 0x08)
								m_type = Type::MF_CL_1K;
						}
					}
				}
			}
			else									// 0bxxxx'000x
			{
				if (m_sak & b5)						// 0bxxx1'000x
				{
					if (m_sak & b1)					// 0bxxx1'0001
					{
						if (m_sak == 0x11)
							m_type = Type::MF_PL_4K_SL2;
					}
					else							// 0bxxx1'0000
					{
						if (m_sak == 0x10)
							m_type = Type::MF_PL_2K_SL2;
					}
				}
				else								// 0bxxx0'000x
				{
					if (m_sak & b1)					// 0bxxx0'0001
						m_type = Type::TNP;
					else							// 0bxxx0'0000
					{
						if (m_sak & b6)				// 0bxx10'0000
						{
							if (m_sak != 0x20)
								return;
							
							MFRC522::ATS ats;
							if (m_frontend.PICC_RATS(&ats) != MFRC522::Status::OK)
								return;
							
							ats.Log();
							Time::HighResClock::sleep_for(ats.SFGT());
							
							const uint8_t ds = ats.DS(), dr = ats.DR();
							if (m_frontend.PICC_PPS(ds || dr ? std::make_optional(0) : std::nullopt) != MFRC522::Status::OK)
								return;
							
							Log::LOG_D("PPS");
							
							VerInfo info;
							if (NXP_GET_VERSION_L4(&info) != MFRC522::Status::OK)
							{
								if (m_sak != 0x20)
									return;
								
								// todo: historical bytes
								return;
							}
							
							Log::LOG_D("Version");
							
							switch (info.type)
							{
								case 0x01:
								case 0x81:
								{
									if (info.majorVersion == 0xA0)
									{
										if (info.type == 0x01)
										{
											switch (info.storageSize)
											{
												case 0x16:	m_type = Type::MF_DUOX_2K; break;
												case 0x18:	m_type = Type::MF_DUOX_4K; break;
												case 0x1A:	m_type = Type::MF_DUOX_8K; break;
												case 0x1C:	m_type = Type::MF_DUOX_16K; break;
											}
										}
										
										break;
									}
									else
									{
										switch (info.majorVersion & 0x0F)
										{
											case 0x1:
											{
												switch (info.storageSize)
												{
													case 0x10:	m_type = Type::MF_DF_EV1_256B; break;
													case 0x16:	m_type = Type::MF_DF_EV1_2K; break;
													case 0x18:	m_type = Type::MF_DF_EV1_4K; break;
													case 0x1A:	m_type = Type::MF_DF_EV1_8K; break;
												}
												
												break;
											}
											
											case 0x2:
											{
												switch (info.storageSize)
												{
													case 0x16:	m_type = Type::MF_DF_EV2_2K; break;
													case 0x18:	m_type = Type::MF_DF_EV2_4K; break;
													case 0x1A:	m_type = Type::MF_DF_EV2_8K; break;
													case 0x1C:	m_type = Type::MF_DF_EV2_16K; break;
													case 0x1E:	m_type = Type::MF_DF_EV2_32K; break;
												}
												
												break;
											}
											
											case 0x3:
											{
												switch (info.storageSize)
												{
													case 0x16:	m_type = Type::MF_DF_EV3_2K; break;
													case 0x18:	m_type = Type::MF_DF_EV3_4K; break;
													case 0x1A:	m_type = Type::MF_DF_EV3_8K; break;
												}
												
												break;
											}
										}
									}
									
									break;
								}
								
								case 0x02:
								case 0x82:
								{
									if (info.majorVersion == 0x11 && info.storageSize == 0x16)
										m_type = Type::MF_PL_EV1_2K_SL3;
									else if (info.majorVersion == 0x11 && info.storageSize == 0x18)
										m_type = Type::MF_PL_EV1_4K_SL3;
									else if (info.majorVersion == 0x22 && info.storageSize == 0x16)
										m_type = Type::MF_PL_EV2_2K_SL3;
									else if (info.majorVersion == 0x22 && info.storageSize == 0x18)
										m_type = Type::MF_PL_EV2_4K_SL3;
									
									break;
								}
								
								case 0x08:
								{
									m_type = Type::MF_DFL;
									break;
								}
								
								case 0x04:
								{
									if (info.majorVersion == 0xA0)
										m_type = Type::NTAG_X_DNA;
									else
										m_type = Type::NTAG4xx;
									
									break;
								}
							}
						}
						else						// 0bxx00'0000
						{
							if (m_sak != 0x00)
								return;
							
							VerInfo info;
							if (NXP_GET_VERSION_L3(&info) == MFRC522::Status::OK)
							{
								switch (info.type)
								{
									case 0x03:
									{
										switch (info.majorVersion)
										{
											case 0x01:	m_type = Type::MF_UL_EV1; break;
											case 0x04:	m_type = Type::MF_UL_AES; break;
										}
										
										break;
									}
									
									case 0x04:
									{
										switch (info.subType)
										{
											case 0x02:
											{
												switch (info.storageSize)
												{
													case 0x0F:	m_type = Type::NTAG213; break;
													case 0x11:	m_type = Type::NTAG215; break;
													case 0x13:	m_type = Type::NTAG216; break;
												}
												
												break;
											}
											
											case 0x05:	m_type = Type::NTAG_I2C_PL; break;
										}
										
										break;
									}
								}
							}
							else
								;	// todo: Auth L3
						}
					}
				}
			}
		}
		
	private:
		struct VerInfo { uint8_t vendorId, type, subType, majorVersion, minorVersion, storageSize, protocol; };
		
		friend class MFRC522;
		
		MFRC522 &m_frontend;
		uint16_t m_atqa;
		uint8_t
			m_sak = b3,		// The SAK (Select acknowledge) byte returned from the PICC after successful selection.
			m_idLen = 0,	// Number of bytes in the UID. 4, 7 or 10.
			m_id[10];
		
		Type m_type = Type::UNKNOWN;
		
		PICC(MFRC522 &frontend, uint16_t atqa) : m_frontend(frontend), m_atqa(atqa) {}
		
		MFRC522::Status NXP_GET_VERSION_L3(VerInfo *info)
		{
			static constexpr uint8_t CMD[3] = { 0x60, (uint8_t)MFRC522::CalculateCRC(CMD, 1), (uint8_t)(MFRC522::CalculateCRC(CMD, 1) >> 8) };
			
			uint8_t resp[10], respLen = sizeof(resp);
			
			const MFRC522::Status stat = m_frontend.PCD_CommunicateWithPICC(MFRC522::Cmd::Transceive, CMD, resp, &respLen, 0x30, nullptr, 0, true);
			if (stat != MFRC522::Status::OK)
				return stat;
			
			if (respLen != sizeof(resp) || resp[0] != 0x00 || resp[7] != 0x03)
				return MFRC522::Status::ERROR;
			
			memcpy(info, resp + 1, sizeof(*info));
			
			return MFRC522::Status::OK;
		}
		
		MFRC522::Status NXP_GET_VERSION_L4(VerInfo *info)
		{
			static constexpr uint8_t CMD[5] = { 0x90, 0x60, 0x00, 0x00, 0x00 };
			using namespace STM32T::Log::DefLog;
			uint8_t resp[9], respLen = sizeof(resp);
			
			const MFRC522::Status stat = m_frontend.PCD_CommunicateWithPICC(MFRC522::Cmd::Transceive, CMD, resp, &respLen, 0x30);
			if (stat != MFRC522::Status::OK)
			{
				LOG_D("Status: %s", MFRC522::StatusName(stat).data());
				return stat;
			}
			LOG_D("len: %hhu", respLen);
			if (respLen != sizeof(resp))	// || resp[6] != 0x05)
				return MFRC522::Status::ERROR;
			
			const uint16_t status = pack_le<uint16_t>(resp + respLen - 2);
			LOG_D("stat2: %04hX", status);
			if (status != 0x9100 && status != 0x91AF)
				return MFRC522::Status::ERROR;
			
			memcpy(info, resp, sizeof(*info));
			
			return MFRC522::Status::OK;
		}
	};
	
	/**
	* Transmits SELECT/ANTICOLLISION commands to select a single PICC.
	* Before calling this function the PICCs must be placed in the READY(*) state by calling PICC_RequestA() or PICC_WakeupA().
	* On success:
	* 		- The chosen PICC is in state ACTIVE(*) and all other PICCs have returned to state IDLE/HALT. (Figure 7 of the ISO/IEC 14443-3 draft.)
	* 		- The UID size and value of the chosen PICC is returned in *uid along with the SAK.
	* 
	* A PICC UID consists of 4, 7 or 10 bytes.
	* Only 4 bytes can be specified in a SELECT command, so for the longer UIDs two or three iterations are used:
	* 		UID size	Number of UID bytes		Cascade levels		Example of PICC
	* 		========	===================		==============		===============
	* 		single				 4						1				MIFARE Classic
	* 		double				 7						2				MIFARE Ultralight
	* 		triple				10						3				Not currently in use?
	* 
	* @param uid		- Pointer to Uid struct. Normally output, but can also be used to supply a known UID.
	* @param validBits	- The number of known UID bits supplied in *uid. Normally 0. If set you must also supply uid->size.
	* @return STATUS_OK on success, STATUS_??? otherwise.
	*/
	MFRC522::Status MFRC522::PICC_Select(PICC *uid, uint8_t validBits)
	{
		bool uidComplete, selectDone, useCascadeTag;
		uint8_t cascadeLevel = 1;
		Status result;
		uint8_t count;
		uint8_t checkBit;
		uint8_t index;
		uint8_t uidIndex;				// The first index in uid->uidByte[] that is used in the current Cascade Level.
		int8_t currentLevelKnownBits;	// The number of known UID bits in the current Cascade Level.
		uint8_t buffer[9];				// The SELECT/ANTICOLLISION commands uses a 7 byte standard frame + 2 bytes CRC_A
		uint8_t bufferUsed;				// The number of bytes used in the buffer, ie the number of bytes to transfer to the FIFO.
		uint8_t rxAlign;				// Used in BitFramingReg. Defines the bit position for the first bit received.
		uint8_t txLastBits;				// Used in BitFramingReg. The number of valid bits in the last transmitted byte. 
		uint8_t *responseBuffer;
		uint8_t responseLength;
		
		// Description of buffer structure:
		//		Byte 0: SEL 				Indicates the Cascade Level: PICC_CMD_SEL_CL1, PICC_CMD_SEL_CL2 or PICC_CMD_SEL_CL3
		//		Byte 1: NVB					Number of Valid Bits (in complete command, not just the UID): High nibble: complete bytes, Low nibble: Extra bits. 
		//		Byte 2: UID-data or CT		See explanation below. CT means Cascade Tag.
		//		Byte 3: UID-data
		//		Byte 4: UID-data
		//		Byte 5: UID-data
		//		Byte 6: BCC					Block Check Character - XOR of bytes 2-5
		//		Byte 7: CRC_A
		//		Byte 8: CRC_A
		// The BCC and CRC_A are only transmitted if we know all the UID bits of the current Cascade Level.
		//
		// Description of bytes 2-5: (Section 6.5.4 of the ISO/IEC 14443-3 draft: UID contents and cascade levels)
		//		UID size	Cascade level	Byte2	Byte3	Byte4	Byte5
		//		========	=============	=====	=====	=====	=====
		//		 4 bytes		1			uid0	uid1	uid2	uid3
		//		 7 bytes		1			CT		uid0	uid1	uid2
		//						2			uid3	uid4	uid5	uid6
		//		10 bytes		1			CT		uid0	uid1	uid2
		//						2			CT		uid3	uid4	uid5
		//						3			uid6	uid7	uid8	uid9
		
		// Sanity checks
		if (validBits > 80)
			return Status::INVALID;
		
		// Prepare MFRC522
		ClearBits(Reg::Coll, 0x80);		// ValuesAfterColl=1 => Bits received after collision are cleared.
		
		// Repeat Cascade Level loop until we have a complete UID.
		uidComplete = false;
		while (!uidComplete)
		{
			// Set the Cascade Level in the SEL byte, find out if we need to use the Cascade Tag in byte 2.
			switch (cascadeLevel)
			{
				case 1:
					buffer[0] = CmdPICC::SEL_CL1;
					uidIndex = 0;
					useCascadeTag = validBits && uid->m_idLen > 4;	// When we know that the UID has more than 4 bytes
					break;
				
				case 2:
					buffer[0] = CmdPICC::SEL_CL2;
					uidIndex = 3;
					useCascadeTag = validBits && uid->m_idLen > 7;	// When we know that the UID has more than 7 bytes
					break;
				
				case 3:
					buffer[0] = CmdPICC::SEL_CL3;
					uidIndex = 6;
					useCascadeTag = false;						// Never used in CL3.
					break;
				
				default:
					return Status::INTERNAL_ERROR;
					break;
			}
			
			// How many UID bits are known in this Cascade Level?
			currentLevelKnownBits = validBits - (8 * uidIndex);
			if (currentLevelKnownBits < 0)
				currentLevelKnownBits = 0;
			
			// Copy the known bits from uid->uidByte[] to buffer[]
			index = 2; // destination index in buffer[]
			if (useCascadeTag)
				buffer[index++] = CmdPICC::CT;
			
			uint8_t bytesToCopy = currentLevelKnownBits / 8 + (currentLevelKnownBits % 8 ? 1 : 0); // The number of bytes needed to represent the known bits for this level.
			if (bytesToCopy)
			{
				uint8_t maxBytes = useCascadeTag ? 3 : 4; // Max 4 bytes in each Cascade Level. Only 3 left if we use the Cascade Tag
				if (bytesToCopy > maxBytes)
					bytesToCopy = maxBytes;
				
				for (count = 0; count < bytesToCopy; count++)
					buffer[index++] = uid->m_id[uidIndex + count];
			}
			
			// Now that the data has been copied we need to include the 8 bits in CT in currentLevelKnownBits
			if (useCascadeTag)
				currentLevelKnownBits += 8;
			
			// Repeat anti collision loop until we can transmit all UID bits + BCC and receive a SAK - max 32 iterations.
			selectDone = false;
			while (!selectDone)
			{
				// Find out how many bits and bytes to send and receive.
				if (currentLevelKnownBits >= 32)	// All UID bits in this Cascade Level are known. This is a SELECT.
				{
					//Serial.print(F("SELECT: currentLevelKnownBits=")); Serial.println(currentLevelKnownBits, DEC);
					buffer[1] = 0x70; // NVB - Number of Valid Bits: Seven whole bytes
					// Calculate BCC - Block Check Character
					buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];
					// Calculate CRC_A
					unpack_le<uint16_t>(&buffer[7], CalculateCRC(buffer, 7));
					result = Status::OK;
					
					txLastBits		= 0; // 0 => All 8 bits are valid.
					bufferUsed		= 9;
					// Store response in the last 3 bytes of buffer (BCC and CRC_A - not needed after tx)
					responseBuffer	= &buffer[6];
					responseLength	= 3;
				}
				else	// This is an ANTICOLLISION.
				{
					//Serial.print(F("ANTICOLLISION: currentLevelKnownBits=")); Serial.println(currentLevelKnownBits, DEC);
					txLastBits		= currentLevelKnownBits % 8;
					count			= currentLevelKnownBits / 8;	// Number of whole bytes in the UID part.
					index			= 2 + count;					// Number of whole bytes: SEL + NVB + UIDs
					buffer[1]		= (index << 4) + txLastBits;	// NVB - Number of Valid Bits
					bufferUsed		= index + (txLastBits ? 1 : 0);
					// Store response in the unused part of buffer
					responseBuffer	= &buffer[index];
					responseLength	= sizeof(buffer) - index;
				}
				
				// Set bit adjustments
				rxAlign = txLastBits;											// Having a separate variable is overkill. But it makes the next line easier to read.
				Write(Reg::BitFraming, (rxAlign << 4) + txLastBits);			// RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]
				
				// Transmit the buffer and receive the response.
				result = PCD_CommunicateWithPICC(Cmd::Transceive, span<const uint8_t>{buffer, size_t(bufferUsed)}, responseBuffer, &responseLength, 0x30, &txLastBits, rxAlign);
				if (result == Status::COLLISION)	// More than one PICC in the field => collision.
				{
					uint8_t valueOfCollReg = Read(Reg::Coll); // CollReg[7..0] bits are: ValuesAfterColl reserved CollPosNotValid CollPos[4:0]
					if (valueOfCollReg & 0x20)		// CollPosNotValid
						return Status::COLLISION;	// Without a valid collision position we cannot continue
					
					uint8_t collisionPos = valueOfCollReg & 0x1F; // Values 0-31, 0 means bit 32.
					if (collisionPos == 0)
						collisionPos = 32;
					
					if (collisionPos <= currentLevelKnownBits)	// No progress - should not happen 
						return Status::INTERNAL_ERROR;
					
					// Choose the PICC with the bit set.
					currentLevelKnownBits	= collisionPos;
					count			= currentLevelKnownBits % 8; // The bit to modify
					checkBit		= (currentLevelKnownBits - 1) % 8;
					index			= 1 + (currentLevelKnownBits / 8) + (count ? 1 : 0); // First byte is index 0.
					buffer[index]	|= (1 << checkBit);
				}
				else if (result != Status::OK)
					return result;
				else	// STATUS_OK
				{
					if (currentLevelKnownBits >= 32)	// This was a SELECT.
						selectDone = true;				// No more anticollision; we continue below outside the while.
					else								// This was an ANTICOLLISION.
						// We now have all 32 bits of the UID in this Cascade Level
						currentLevelKnownBits = 32;
						// Run loop again to do the SELECT.
				}
			} // End of while (!selectDone)
			
			// We do not check the CBB - it was constructed by us above.
			
			// Copy the found UID bytes from buffer[] to uid->uidByte[]
			index			= (buffer[2] == CmdPICC::CT) ? 3 : 2; // source index in buffer[]
			bytesToCopy		= (buffer[2] == CmdPICC::CT) ? 3 : 4;
			for (count = 0; count < bytesToCopy; count++)
				uid->m_id[uidIndex + count] = buffer[index++];
			
			// Check response SAK (Select Acknowledge)
			if (responseLength != 3 || txLastBits != 0)		// SAK must be exactly 24 bits (1 byte + CRC_A).
				return Status::ERROR;
			
			// Verify CRC_A - do our own calculation and store the control in buffer[2..3] - those bytes are not needed anymore.
			unpack_le<uint16_t>(&buffer[2], CalculateCRC(responseBuffer, 1));
			result = Status::OK;
			
			if ((buffer[2] != responseBuffer[1]) || (buffer[3] != responseBuffer[2]))
				return Status::CRC_WRONG;
			
			if (responseBuffer[0] & 0x04)	// Cascade bit set - UID not complete yes
				cascadeLevel++;
			else
			{
				uidComplete = true;
				uid->m_sak = responseBuffer[0];
			}
		} // End of while (!uidComplete)
		
		// Set correct uid->size
		uid->m_idLen = 3 * cascadeLevel + 1;

		return Status::OK;
	}
	
	std::optional<PICC> MFRC522::SelectPICC()
	{
		// Reset baud rates
		Write(Reg::TxMode, 0);
		Write(Reg::RxMode, 0);
		// Reset ModWidthReg
		Write(Reg::ModWidth, 0x26);
		
		uint16_t atqa;
		const Status stat = PICC_REQA_or_WUPA(CmdPICC::REQA, &atqa);
		if (stat != Status::OK && stat != Status::COLLISION)
			return std::nullopt;
		
		PICC picc(*this, atqa);
		
		return PICC_Select(&picc) == Status::OK ? std::make_optional(std::move(picc)) : std::nullopt;
	}
}
