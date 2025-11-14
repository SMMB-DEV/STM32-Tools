#pragma once

#include "../IO.hpp"

namespace STM32T
{
	class MFRC522
	{
		// todo: adjust all SPI timeouts
	public:
		struct UID
		{
			uint8_t size, sak;
			uint8_t id[10];
		};
		
	private:
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
		
		uint8_t Addr4Read(const Reg reg) { return static_cast<uint8_t>(0b1000'0000 | (static_cast<uint8_t>(reg) << 1)); }
		uint8_t Addr4Write(const Reg reg) { return static_cast<uint8_t>(reg) << 1; }
		
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
		
		void Write(const Reg reg, uint8_t *const vals, const size_t count)
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
			Write(Reg::Command, static_cast<uint8_t>(cmd));
		}
		
		//------------------------------------------------------------------------------------------------------------------------------------------------------
		
		Status PCD_CalculateCRC(	uint8_t *data,		///< In: Pointer to the data to transfer to the FIFO for CRC calculation.
									uint8_t length,		///< In: The number of bytes to transfer.
									uint8_t *result)	///< Out: Pointer to result buffer. Result is written to result[0..1], low byte first.
		{
			Command(Cmd::Idle);						// Stop any active command.
			Write(Reg::DivIrq, 0x04);				// Clear the CRCIRq interrupt request bit
			Write(Reg::FIFOLevel, 0x80);			// FlushBuffer = 1, FIFO initialization
			Write(Reg::FIFOData, data, length);		// Write data to the FIFO
			Command(Cmd::CalcCRC);					// Start the calculation
			
			// Wait for the CRC calculation to complete. Check for the register to
			// indicate that the CRC calculation is complete in a loop. If the
			// calculation is not indicated as complete in ~90ms, then time out
			// the operation.
			const uint32_t deadline_start = HAL_GetTick();

			do {
				// DivIrqReg[7..0] bits are: Set2 reserved reserved MfinActIRq reserved CRCIRq reserved reserved
				uint8_t n = Read(Reg::DivIrq);
				if (n & 0x04)						// CRCIRq bit set - calculation done
				{
					Command(Cmd::Idle);				// Stop calculating CRC for new content in the FIFO.
					// Transfer the result from the registers to the result buffer
					result[0] = Read(Reg::CRCResultL);
					result[1] = Read(Reg::CRCResultH);
					return Status::OK;
				}
			}
			while (HAL_GetTick() - deadline_start < 89);

			// 89ms passed and nothing happened. Communication with the MFRC522 might be down.
			return Status::TIMEOUT;
		}
		
		Status PCD_CommunicateWithPICC(	Cmd command,		///< The command to execute. One of the PCD_Command enums.
										uint8_t waitIRq,		///< The bits in the ComIrqReg register that signals successful completion of the command.
										uint8_t *sendData,		///< Pointer to the data to transfer to the FIFO.
										uint8_t sendLen,		///< Number of bytes to transfer to the FIFO.
										uint8_t *backData,		///< nullptr or pointer to buffer if data should be read back after executing the command.
										uint8_t *backLen,		///< In: Max number of bytes to write to *backData. Out: The number of bytes returned.
										uint8_t *validBits,		///< In/Out: The number of valid bits in the last byte. 0 for 8 valid bits.
										uint8_t rxAlign,		///< In: Defines the bit position in backData[0] for the first bit received. Default 0.
										bool checkCRC)			///< In: True => The last two bytes of the response is assumed to be a CRC_A that must be validated.
		{
			// Prepare values for BitFramingReg
			uint8_t txLastBits = validBits ? *validBits : 0;
			uint8_t bitFraming = (rxAlign << 4) + txLastBits;		// RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]
			
			Command(Cmd::Idle);										// Stop any active command.
			Write(Reg::ComIrq, 0x7F);								// Clear all seven interrupt request bits
			Write(Reg::FIFOLevel, 0x80);							// FlushBuffer = 1, FIFO initialization
			Write(Reg::FIFOData, sendData, sendLen);				// Write sendData to the FIFO
			Write(Reg::BitFraming, bitFraming);						// Bit adjustments
			Command(command);										// Execute the command
			if (command == Cmd::Transceive)
				SetBits(Reg::BitFraming, 0x80);						// StartSend=1, transmission of data starts
			
			// In PCD_Init() we set the TAuto flag in TModeReg. This means the timer
			// automatically starts when the PCD stops transmitting.
			//
			// Wait here for the command to complete. The bits specified in the
			// `waitIRq` parameter define what bits constitute a completed command.
			// When they are set in the ComIrqReg register, then the command is
			// considered complete. If the command is not indicated as complete in
			// ~36ms, then consider the command as timed out.
			const uint32_t deadline_start = HAL_GetTick();
			bool completed = false;

			do {
				uint8_t n = Read(Reg::ComIrq);		// ComIrqReg[7..0] bits are: Set1 TxIRq RxIRq IdleIRq HiAlertIRq LoAlertIRq ErrIRq TimerIRq
				if (n & waitIRq)					// One of the interrupts that signal success has been set.
				{
					completed = true;
					break;
				}
				
				if (n & 0x01)						// Timer interrupt - nothing received in 25ms
					return Status::TIMEOUT;
			}
			while (HAL_GetTick() - deadline_start < 36);

			// 36ms and nothing happened. Communication with the MFRC522 might be down.
			if (!completed)
				return Status::TIMEOUT;
			
			// Stop now if any errors except collisions were detected.
			uint8_t errorRegValue = Read(Reg::Error); // ErrorReg[7..0] bits are: WrErr TempErr reserved BufferOvfl CollErr CRCErr ParityErr ProtocolErr
			if (errorRegValue & 0x13)	 // BufferOvfl ParityErr ProtocolErr
				return Status::ERROR;
		  
			uint8_t _validBits = 0;
			
			// If the caller wants data back, get it from the MFRC522.
			if (backData && backLen)
			{
				uint8_t n = Read(Reg::FIFOLevel);	// Number of bytes in the FIFO
				if (n > *backLen)
					return Status::NO_ROOM;
				
				*backLen = n;											// Number of bytes returned
				Read(Reg::FIFOData, backData, n, rxAlign);				// Get received data from FIFO
				_validBits = Read(Reg::Control) & 0x07;					// RxLastBits[2:0] indicates the number of valid bits in the last received byte. If this value is 000b, the whole byte is valid.
				if (validBits)
					*validBits = _validBits;
			}
			
			// Tell about collisions
			if (errorRegValue & 0x08)		// CollErr
				return Status::COLLISION;
			
			// Perform CRC_A validation if requested.
			if (backData && backLen && checkCRC) {
				// In this case a MIFARE Classic NAK is not OK.
				if (*backLen == 1 && _validBits == 4)
					return Status::MIFARE_NACK;
				
				// We need at least the CRC_A value and all 8 bits of the last byte must be received.
				if (*backLen < 2 || _validBits != 0)
					return Status::CRC_WRONG;
				
				// Verify CRC_A - do our own calculation and store the control in controlBuffer.
				uint8_t controlBuffer[2];
				Status status = PCD_CalculateCRC(&backData[0], *backLen - 2, &controlBuffer[0]);
				if (status != Status::OK)
					return status;
				
				if ((backData[*backLen - 2] != controlBuffer[0]) || (backData[*backLen - 1] != controlBuffer[1]))
					return Status::CRC_WRONG;
			}
			
			return Status::OK;
		}
		
		Status PCD_TransceiveData(	uint8_t *sendData,				///< Pointer to the data to transfer to the FIFO.
									uint8_t sendLen,				///< Number of bytes to transfer to the FIFO.
									uint8_t *backData,				///< nullptr or pointer to buffer if data should be read back after executing the command.
									uint8_t *backLen,				///< In: Max number of bytes to write to *backData. Out: The number of bytes returned.
									uint8_t *validBits = nullptr,	///< In/Out: The number of valid bits in the last byte. 0 for 8 valid bits. Default nullptr.
									uint8_t rxAlign = 0,			///< In: Defines the bit position in backData[0] for the first bit received. Default 0.
									bool checkCRC = false)			///< In: True => The last two bytes of the response is assumed to be a CRC_A that must be validated.
		{
			uint8_t waitIRq = 0x30;		// RxIRq and IdleIRq
			return PCD_CommunicateWithPICC(Cmd::Transceive, waitIRq, sendData, sendLen, backData, backLen, validBits, rxAlign, checkCRC);
		}
		
		/**
		* @param command - PICC_CMD_REQA or PICC_CMD_WUPA
		*/
		Status PICC_REQA_or_WUPA(uint8_t command)
		{
			uint8_t bufferATQA[2];
			uint8_t bufferSize = sizeof(bufferATQA);
			
			ClearBits(Reg::Coll, 0x80);					// ValuesAfterColl=1 => Bits received after collision are cleared.
			uint8_t validBits = 7;						// For REQA and WUPA we need the short frame format - transmit only 7 bits of the last (and only) byte. TxLastBits = BitFramingReg[2..0]
			Status status = PCD_TransceiveData(&command, 1, bufferATQA, &bufferSize, &validBits);
			if (status != Status::OK)
				return status;
			
			if (bufferSize != 2 || validBits != 0)		// ATQA must be exactly 16 bits.
				return Status::ERROR;
			
			return Status::OK;
		}
		
		
		Status PICC_REQA()
		{
			return PICC_REQA_or_WUPA(CmdPICC::REQA);
		}
		
		Status PICC_Select(	UID *uid,				///< Pointer to Uid struct. Normally output, but can also be used to supply a known UID.
							uint8_t validBits = 0)	///< The number of known UID bits supplied in *uid. Normally 0. If set you must also supply uid->size.
		{
			bool uidComplete;
			bool selectDone;
			bool useCascadeTag;
			uint8_t cascadeLevel = 1;
			Status result;
			uint8_t count;
			uint8_t checkBit;
			uint8_t index;
			uint8_t uidIndex;					// The first index in uid->uidByte[] that is used in the current Cascade Level.
			int8_t currentLevelKnownBits;		// The number of known UID bits in the current Cascade Level.
			uint8_t buffer[9];					// The SELECT/ANTICOLLISION commands uses a 7 byte standard frame + 2 bytes CRC_A
			uint8_t bufferUsed;				// The number of bytes used in the buffer, ie the number of bytes to transfer to the FIFO.
			uint8_t rxAlign;					// Used in BitFramingReg. Defines the bit position for the first bit received.
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
			if (validBits > 80) {
				return Status::INVALID;
			}
			
			// Prepare MFRC522
			ClearBits(Reg::Coll, 0x80);		// ValuesAfterColl=1 => Bits received after collision are cleared.
			
			// Repeat Cascade Level loop until we have a complete UID.
			uidComplete = false;
			while (!uidComplete) {
				// Set the Cascade Level in the SEL byte, find out if we need to use the Cascade Tag in byte 2.
				switch (cascadeLevel) {
					case 1:
						buffer[0] = CmdPICC::SEL_CL1;
						uidIndex = 0;
						useCascadeTag = validBits && uid->size > 4;	// When we know that the UID has more than 4 bytes
						break;
					
					case 2:
						buffer[0] = CmdPICC::SEL_CL2;
						uidIndex = 3;
						useCascadeTag = validBits && uid->size > 7;	// When we know that the UID has more than 7 bytes
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
				if (currentLevelKnownBits < 0) {
					currentLevelKnownBits = 0;
				}
				// Copy the known bits from uid->uidByte[] to buffer[]
				index = 2; // destination index in buffer[]
				if (useCascadeTag) {
					buffer[index++] = CmdPICC::CT;
				}
				uint8_t bytesToCopy = currentLevelKnownBits / 8 + (currentLevelKnownBits % 8 ? 1 : 0); // The number of bytes needed to represent the known bits for this level.
				if (bytesToCopy) {
					uint8_t maxBytes = useCascadeTag ? 3 : 4; // Max 4 bytes in each Cascade Level. Only 3 left if we use the Cascade Tag
					if (bytesToCopy > maxBytes) {
						bytesToCopy = maxBytes;
					}
					for (count = 0; count < bytesToCopy; count++) {
						buffer[index++] = uid->id[uidIndex + count];
					}
				}
				// Now that the data has been copied we need to include the 8 bits in CT in currentLevelKnownBits
				if (useCascadeTag) {
					currentLevelKnownBits += 8;
				}
				
				// Repeat anti collision loop until we can transmit all UID bits + BCC and receive a SAK - max 32 iterations.
				selectDone = false;
				while (!selectDone) {
					// Find out how many bits and bytes to send and receive.
					if (currentLevelKnownBits >= 32) { // All UID bits in this Cascade Level are known. This is a SELECT.
						//Serial.print(F("SELECT: currentLevelKnownBits=")); Serial.println(currentLevelKnownBits, DEC);
						buffer[1] = 0x70; // NVB - Number of Valid Bits: Seven whole bytes
						// Calculate BCC - Block Check Character
						buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];
						// Calculate CRC_A
						result = PCD_CalculateCRC(buffer, 7, &buffer[7]);
						if (result != Status::OK) {
							return result;
						}
						txLastBits		= 0; // 0 => All 8 bits are valid.
						bufferUsed		= 9;
						// Store response in the last 3 bytes of buffer (BCC and CRC_A - not needed after tx)
						responseBuffer	= &buffer[6];
						responseLength	= 3;
					}
					else { // This is an ANTICOLLISION.
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
					result = PCD_TransceiveData(buffer, bufferUsed, responseBuffer, &responseLength, &txLastBits, rxAlign);
					if (result == Status::COLLISION) { // More than one PICC in the field => collision.
						uint8_t valueOfCollReg = Read(Reg::Coll); // CollReg[7..0] bits are: ValuesAfterColl reserved CollPosNotValid CollPos[4:0]
						if (valueOfCollReg & 0x20) { // CollPosNotValid
							return Status::COLLISION; // Without a valid collision position we cannot continue
						}
						uint8_t collisionPos = valueOfCollReg & 0x1F; // Values 0-31, 0 means bit 32.
						if (collisionPos == 0) {
							collisionPos = 32;
						}
						if (collisionPos <= currentLevelKnownBits) { // No progress - should not happen 
							return Status::INTERNAL_ERROR;
						}
						// Choose the PICC with the bit set.
						currentLevelKnownBits	= collisionPos;
						count			= currentLevelKnownBits % 8; // The bit to modify
						checkBit		= (currentLevelKnownBits - 1) % 8;
						index			= 1 + (currentLevelKnownBits / 8) + (count ? 1 : 0); // First byte is index 0.
						buffer[index]	|= (1 << checkBit);
					}
					else if (result != Status::OK) {
						return result;
					}
					else { // STATUS_OK
						if (currentLevelKnownBits >= 32) { // This was a SELECT.
							selectDone = true; // No more anticollision 
							// We continue below outside the while.
						}
						else { // This was an ANTICOLLISION.
							// We now have all 32 bits of the UID in this Cascade Level
							currentLevelKnownBits = 32;
							// Run loop again to do the SELECT.
						}
					}
				} // End of while (!selectDone)
				
				// We do not check the CBB - it was constructed by us above.
				
				// Copy the found UID bytes from buffer[] to uid->uidByte[]
				index			= (buffer[2] == CmdPICC::CT) ? 3 : 2; // source index in buffer[]
				bytesToCopy		= (buffer[2] == CmdPICC::CT) ? 3 : 4;
				for (count = 0; count < bytesToCopy; count++) {
					uid->id[uidIndex + count] = buffer[index++];
				}
				
				// Check response SAK (Select Acknowledge)
				if (responseLength != 3 || txLastBits != 0) { // SAK must be exactly 24 bits (1 byte + CRC_A).
					return Status::ERROR;
				}
				// Verify CRC_A - do our own calculation and store the control in buffer[2..3] - those bytes are not needed anymore.
				result = PCD_CalculateCRC(responseBuffer, 1, &buffer[2]);
				if (result != Status::OK) {
					return result;
				}
				if ((buffer[2] != responseBuffer[1]) || (buffer[3] != responseBuffer[2])) {
					return Status::CRC_WRONG;
				}
				if (responseBuffer[0] & 0x04) { // Cascade bit set - UID not complete yes
					cascadeLevel++;
				}
				else {
					uidComplete = true;
					uid->sak = responseBuffer[0];
				}
			} // End of while (!uidComplete)
			
			// Set correct uid->size
			uid->size = 3 * cascadeLevel + 1;

			return Status::OK;
		}
		
	public:
		static constexpr size_t SELF_TEST_RESULT_SIZE = 64;
		
		MFRC522(SPI_HandleTypeDef *spi, IO cs) : p_spi(spi), m_cs(cs) {}
		
		void Init()
		{
			Read(Reg::Reserved);
			
			// https://github.com/miguelbalboa/rfid
			
			Write(Reg::ModWidth, 0x26);
			Write(Reg::TMode, 0x80);				// TAuto=1; timer starts automatically at the end of the transmission in all communication modes at all speeds
			Write(Reg::TPrescaler, 0xA9);			// TPreScaler = TModeReg[3..0]:TPrescalerReg, ie 0x0A9 = 169 => f_timer=40kHz, ie a timer period of 25µs.
			Write(Reg::TReloadH, 0x03);				// Reload timer with 0x3E8 = 1000, ie 25ms before timeout.
			Write(Reg::TReloadL, 0xE8);
			
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
			Write(Reg::FIFOData, ZEROS, _countof(ZEROS));
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
		
		bool IsIdlePICC()
		{
			// Reset baud rates
			Write(Reg::TxMode, 0);
			Write(Reg::RxMode, 0);
			// Reset ModWidthReg
			Write(Reg::ModWidth, 0x26);

			Status result = PICC_REQA();
			return (result == Status::OK || result == Status::COLLISION);
		}
		
		UID SelectPICC()
		{
			UID uid;
			
			return PICC_Select(&uid) == Status::OK ? uid : UID{.size = 0};
		}
	};
}
