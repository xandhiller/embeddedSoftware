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

#include "Flash.h"
#include "Cpu.h"

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

/*! @brief Enables the Flash module.
 *
 *  @return bool - TRUE if the Flash was setup successfully.
 */
bool Flash_Init(void)
{
  return true;
}

/*! @brief Allocates space for a non-volatile variable in the Flash memory.
 *
 *  @param variable is the address of a pointer to a variable that is to be allocated space in Flash memory.
 *         The pointer will be allocated to a relevant address:
 *         If the variable is a byte, then any address.
 *         If the variable is a half-word, then an even address.
 *         If the variable is a word, then an address divisible by 4.
 *         This allows the resulting variable to be used with the relevant Flash_Write function which assumes a certain memory address.
 *         e.g. a 16-bit variable will be on an even address
 *  @param size The size, in bytes, of the variable that is to be allocated space in the Flash memory. Valid values are 1, 2 and 4.
 *  @return bool - TRUE if the variable was allocated space in the Flash memory.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_AllocateVar(volatile void** variable, const uint8_t size)
{
  uint8_t* bytePtr;
  uint16_t* halfwordPtr;
  uint32_t* wordPtr;

  switch(size)
  {
    case 1:
      bytePtr = (uint8_t*)FLASH_DATA_START;  // Pointer to the first byte of the phrase being used
      for (uint8_t i = 0; i < 8; i++)
      {
        if (*bytePtr == 0xFF)
        {
          *variable = bytePtr;
          return true;
        }
        bytePtr++;
      }
      return false;

    case 2:
      halfwordPtr = (uint16_t*)FLASH_DATA_START;  // Pointer to the first half-word of the phrase being used
      for (uint8_t i = 0; i < 4; i++)
      {
        if (*halfwordPtr == 0xFFFF)
        {
          *variable = halfwordPtr;
          return true;
        }
        halfwordPtr++;
      }
      return false;

    case 4:
      wordPtr = (uint32_t*)FLASH_DATA_START;  // Pointer to the first word of the phrase being used
      for (uint8_t i = 0; i < 2; i++)
      {
        if (*wordPtr == 0xFFFFFFFF)
        {
          *variable = wordPtr;
          return true;
        }
        wordPtr++;
      }

    default:
      return false;
  }
}

/*! @brief Writes a 32-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 32-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 4-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
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

/*! @brief Writes a 16-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 16-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 2-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
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

/*! @brief Writes an 8-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 8-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
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

/*! @brief Erases the entire Flash sector.
 *
 *  @return bool - TRUE if the Flash "data" sector was erased successfully.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Erase(void)
{
  return EraseSector(FLASH_DATA_START);
}

/*!
** @}
*/
