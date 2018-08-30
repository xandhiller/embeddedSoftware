/*! @file
 *
 *  @brief Routines for erasing and writing to the Flash.
 *
 *  This contains the functions needed for accessing the internal Flash.
 *
 *  @author 12551382 Samin Saif and 11850637 Alex Hiller
 *  @date 2018-04-15
 */
/*!
**  @addtogroup Flash_module Flash module documentation
**  @{
*/
/* MODULE Flash */

#include "brOS.h"

// FCCOB type struct
typedef struct
{
  uint8_t command;

  union
  {
    uint32_t combined;
    struct
    {
      uint8_t flashAddress0;
      uint8_t flashAddress1;
      uint8_t flashAddress2;
      uint8_t flashAddress3;
    } separate;
  } parameterA;  // The first 4 FCCOB registers (0-3)

  union
  {
    uint64_t combined;
    struct
    {
      uint8_t dataByte0;
      uint8_t dataByte1;
      uint8_t dataByte2;
      uint8_t dataByte3;
      uint8_t dataByte4;
      uint8_t dataByte5;
      uint8_t dataByte6;
      uint8_t dataByte7;
    } separate;
  } parameterB;  // The last 8 FCCOB registers (4-B)
} TFCCOB;

// Private Functions

/*! @brief Executes the received flash command.
 *
 *  @param commonCommandObject A pointer to an FCCOB type that holds the flash command and parameters to be excuted.
 *  @return bool - TRUE if the command is successfully executed.
 */
static bool LaunchCommand(TFCCOB* commonCommandObject)
{
  while (!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));  // Keep checking FSTAT until CCIF is set
  FTFE_FSTAT = FTFE_FSTAT_ACCERR_MASK;  // Write 1 to clear access error flag
  FTFE_FSTAT = FTFE_FSTAT_FPVIOL_MASK;  // Write 1 to clear flash protection violation flag

  FTFE_FCCOB0 = commonCommandObject->command;
  FTFE_FCCOB1 = commonCommandObject->parameterA.separate.flashAddress2;
  FTFE_FCCOB2 = commonCommandObject->parameterA.separate.flashAddress1;
  FTFE_FCCOB3 = commonCommandObject->parameterA.separate.flashAddress0;
  FTFE_FCCOB4 = commonCommandObject->parameterB.separate.dataByte7;
  FTFE_FCCOB5 = commonCommandObject->parameterB.separate.dataByte6;
  FTFE_FCCOB6 = commonCommandObject->parameterB.separate.dataByte5;
  FTFE_FCCOB7 = commonCommandObject->parameterB.separate.dataByte4;
  FTFE_FCCOB8 = commonCommandObject->parameterB.separate.dataByte3;
  FTFE_FCCOB9 = commonCommandObject->parameterB.separate.dataByte2;
  FTFE_FCCOBA = commonCommandObject->parameterB.separate.dataByte1;
  FTFE_FCCOBB = commonCommandObject->parameterB.separate.dataByte0;

  FTFE_FSTAT = FTFE_FSTAT_CCIF_MASK;  // Write 1 to clear command complete interrupt flag in order launch loaded command
  while (!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));
  return true;
}

/*! @brief Writes 8 bytes to a phrase.
 *
 *  @param address The address of the first byte of the phrase which is being written to.
 *  @param phrase The value of the phrase being written.
 *  @return bool - TRUE if phrase was successfully writen.
 */
static bool WritePhrase(const uint32_t address, const uint64_t phrase)
{
  TFCCOB write;                         // Declare an FCCOB type
  write.command = 0x07;                 // Write the command to the FCCOB type
  write.parameterA.combined = address;  // Write the address to the FCCOB type
  write.parameterB.combined = phrase;   // Write the phrase to the FCCOB type
  return LaunchCommand(&write);         // Load and execute the FCCOB
}

/*! @brief Erases all bytes in a sector.
 *
 *  @param address The address of the first byte of the sector being erased.
 *  @return bool - TRUE if sector was successfully erased.
 */
static bool EraseSector(const uint32_t address)
{
  TFCCOB erase;
  erase.command = 0x09;
  erase.parameterA.combined = address;
  return LaunchCommand(&erase);
}

/*! @brief Erase a sector and then write a phrase.
 *
 *  @param address The address of the first byte of the sector.
 *  @param phrase The value of the phrase being written.
 *  @return bool - TRUE if phrase was successfully modified.
 */
static bool ModifyPhrase(const uint32_t address, const uint64_t phrase)
{
  if (!EraseSector(address))
    return false;

  return WritePhrase(address, phrase);
}

// Public Functions
bool Flash_Init(void)
{
  return true;
}

bool Flash_AllocateVar(volatile void** variable, const uint8_t size)
{
  static uint8_t FLASH_MAP = 0xFF;
  uint8_t byteMask     = 0x1;
  uint8_t halfwordMask = 0x3;
  uint8_t wordMask     = 0xF;

  switch(size)
  {
    case 1:
    {
      uint8_t* bytePtr = (uint8_t*)FLASH_DATA_START;  // Pointer to the first byte of the phrase being used
      for (uint8_t i = 0; i < 8; i++)
      {
        if (FLASH_MAP & byteMask)
        {
          FLASH_MAP &= ~byteMask;
          *variable = bytePtr;
          return true;
        }
        byteMask <<= 1;
        bytePtr++;
      }
      return false;
    }

    case 2:
    {
      uint16_t* halfwordPtr = (uint16_t*)FLASH_DATA_START;  // Pointer to the first half-word of the phrase being used
      for (uint8_t i = 0; i < 4; i++)
      {
        if (FLASH_MAP & halfwordMask)
        {
          FLASH_MAP &= ~halfwordMask;
          *variable = halfwordPtr;
          return true;
        }
        halfwordMask <<= 2;
        halfwordPtr++;
      }
      return false;
    }

    case 4:
    {
      uint32_t* wordPtr = (uint32_t*)FLASH_DATA_START;  // Pointer to the first word of the phrase being used
      for (uint8_t i = 0; i < 2; i++)
      {
        if (FLASH_MAP & wordMask)
        {
          FLASH_MAP &= ~wordMask;
          *variable = wordPtr;
          return true;
        }
        wordMask <<= 4;
        wordPtr++;
      }
    }

    default:
      return false;
  }
}

bool Flash_Write32(volatile uint32_t* const address, const uint32_t data)
{
  uint32_t newAddress = FLASH_DATA_START;
  uint64union_t phraseBuffer;

  if (address == (volatile uint32_t*)FLASH_DATA_START)
  {
    phraseBuffer.s.Hi = data;
    phraseBuffer.s.Lo = _FW(address + 1);
  }

  else if (address == (volatile uint32_t*)(FLASH_DATA_START+4))
  {
    phraseBuffer.s.Hi = _FW(address - 1);
    phraseBuffer.s.Lo = data;
  }

  else
  {
    return false;
  }

  return ModifyPhrase(newAddress, phraseBuffer.l);
}

bool Flash_Write16(volatile uint16_t* const address, const uint16_t data)
{
  volatile uint32_t* newAddress;
  uint32union_t wordBuffer;

  if ((uint32_t)address % 4 == 0)
  {
    newAddress = (uint32_t*)address;
    wordBuffer.s.Lo = data;
    wordBuffer.s.Hi = _FH(address + 1);
  }

  else if ((uint32_t)address % 2 == 0)
  {
    newAddress = (uint32_t*)(address - 1);
    wordBuffer.s.Hi = data;
    wordBuffer.s.Lo = _FH(address - 1);
  }

  else
  {
    return false;
  }

  return Flash_Write32(newAddress, wordBuffer.l);
}

bool Flash_Write8(volatile uint8_t* const address, const uint8_t data)
{
  volatile uint16_t* newAddress;
  uint16union_t halfWordBuffer;

  if ((uint32_t) address % 2 == 0)
  {
    newAddress = (uint16_t *) address;
    halfWordBuffer.s.Lo = data;
    halfWordBuffer.s.Hi = _FB(address);
  }

  else
  {
    newAddress = (uint16_t *)(address - 1);
    halfWordBuffer.s.Hi = data;
    halfWordBuffer.s.Lo = _FB(address);
  }

  return Flash_Write16(newAddress, halfWordBuffer.l);
}

bool Flash_Erase(void)
{
  return EraseSector(FLASH_DATA_START);
}

bool Flash_NbLowers(void)
{
  uint16_t dummy;
  OS_DisableInterrupts();
  dummy = _FH(PhaseA.NvNbLowers);

  if (!Flash_AllocateVar((void*)&PhaseA.NvNbLowers, sizeof(*PhaseA.NvNbLowers)))
  {
    OS_EnableInterrupts();
    return false;
  }

  else if (PhaseA.NvNbLowers->l == 0xFFFF)
  {
    Flash_Write16(&(PhaseA.NvNbLowers->l), dummy+1);
    OS_EnableInterrupts();
    return true;
  }

  else
  {
    Flash_Write16(&(PhaseA.NvNbLowers->l), PhaseA.NvNbLowers->l);
    OS_EnableInterrupts();
    return true;
  }
}

bool Flash_NbRaises(void)
{
  uint16_t dummy;
  OS_DisableInterrupts();
  dummy = _FH(PhaseA.NvNbRaises);

  if (!Flash_AllocateVar((void*)&PhaseA.NvNbRaises, sizeof(*PhaseA.NvNbRaises)))
  {
    OS_EnableInterrupts();
    return false;
  }

  else if (PhaseA.NvNbRaises->l == 0xFFFF)
  {
    Flash_Write16(&(PhaseA.NvNbRaises->l), dummy+1);
    OS_EnableInterrupts();
    return true;
  }

  else
  {
    Flash_Write16(&(PhaseA.NvNbRaises->l), PhaseA.NvNbRaises->l);
    OS_EnableInterrupts();
    return true;
  }
}

/*!
** @}
*/
