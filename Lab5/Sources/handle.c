/*! @file
 *
 *  @brief Routines to implement packet handling.
 *
 *  This contains the functions for handling received packets.
 *
 *  @author 12551382 Samin Saif and 11850637 Alex Hiller
 *  @date 2018-05-28
 */
/*!
**  @addtogroup handle_module handle module documentation
**  @{
*/
/* MODULE handle */

#include "packet.h"
#include "flash.h"
#include "RTC.h"
#include "FTM.h"
#include "LEDs.h"
#include "handle.h"
#include "Cpu.h"
#include "accel.h"

extern TPacket Packet;
extern TFTMChannel ChLoad[NB_CHANNELS];      // FTM Channel Structure, channel setup variable
TAccelMode ProtocolMode;

// Pointers to be allocated flash memory
static volatile uint16union_t *NvTowerNb;    // Non-volatile tower number
static volatile uint16union_t *NvTowerMode;  // Non-volatile tower mode

static TAccelSetup AccelSetup;


// PRIVATE FUNCTIONS

/*! @brief Sends the packet received back to the PC, with the acknowledgment bit set.
 *
 *  @return bool - TRUE if ACK packet is successfully sent.
 */
static void SendAckPacket(void)
{
  Packet_Put(Packet_Command | PACKET_ACK_MASK, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
}


/*! @brief Sends the packet received back to the PC, with the acknowledgment bit cleared.
 *
 *  @return bool - TRUE if NAK packet is successfully sent.
 */
static void SendNakPacket(void)
{
  Packet_Put(Packet_Command & ~PACKET_ACK_MASK, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
}


/*! @brief Executes the response to an unknown command.
 *
 *  @return bool - TRUE
 */
static bool HandleInvalidCommand(void)
{
  return false;
}


/*! @brief Executes the response to the "Special - Get startup values" command.
 *
 *  @return bool - TRUE if the response was successfully executed.
 */
static bool HandleGetStartupValues(void)
{
  // If the received packet has any invalid parameters
  if ((Packet_Parameter1 != 0) || (Packet_Parameter2 != 0) || (Packet_Parameter3 != 0))
    return false;

  // Normal action
  else
  {
    Packet_Put(TOWER_STARTUP, 0, 0, 0);
    Packet_Put(TOWER_VERSION, 'v', 1, 0);
    Packet_Put(TOWER_NUMBER, 1, NvTowerNb->s.Lo, NvTowerNb->s.Hi);
    Packet_Put(TOWER_MODE, 1, NvTowerMode->s.Lo, NvTowerMode->s.Hi);
    return true;
  }
}


/*! @brief Executes the response to the "Special - Get version" command.
 *
 *  @return bool - TRUE if the response was successfully executed.
 */
static bool HandleGetVersion(void)
{
  // If the received packet has any invalid parameters
  if ((Packet_Parameter1 != 'v') || (Packet_Parameter2 != 'x') || (Packet_Parameter3 != 0x0D))
    return false;

  // Normal action
  else
  {
    Packet_Put(TOWER_VERSION, 'v', 1, 0);
    return true;
  }
}


/*! @brief Executes the response to the "Tower Number" command.
 *
 *  @return bool - TRUE if the response was successfully executed.
 */
static bool HandleTowerNumber(void)
{
  // If the received packet has an invalid parameter1
  if ((Packet_Parameter1 != 1) && (Packet_Parameter1 != 2))
    return false;

  // If 'get' tower number is requested
  else if (Packet_Parameter1 == 1)
  {
    // If the received packet has an invalid parameter2 or parameter3
    if ((Packet_Parameter2 != 0) || (Packet_Parameter3 != 0))
      return false;

    // Normal action
    else
    {
      Packet_Put(TOWER_NUMBER, 1, NvTowerNb->s.Lo, NvTowerNb->s.Hi);
      return true;
    }
  }

  // If 'set' tower number is requested
  else if (Packet_Parameter1 == 2)
  {
    // If the received packet has an invalid parameter2 or parameter3
    if ((Packet_Parameter2 > 0xFF) || (Packet_Parameter3 > 0xFF))
      return false;

    // Normal action
    else
      return Flash_Write16(&(NvTowerNb->l), Packet_Parameter23);
  }
}


/*! @brief Executes the response to the "Tower Mode" command.
 *
 *  @return bool - TRUE if the response was successfully executed.
 */
static bool HandleTowerMode(void)
{
  // If the received packet has an invalid parameter1
  if ((Packet_Parameter1 != 1) && (Packet_Parameter1 != 2))
    return false;

  // If 'get' tower number is requested
  if (Packet_Parameter1 == 1)
  {
    // If the received packet has an invalid parameter2 or parameter3
    if ((Packet_Parameter2 != 0) || (Packet_Parameter3 != 0))
      return false;

    // Normal action
    else
    {
      Packet_Put(TOWER_MODE, 1, NvTowerNb->s.Lo, NvTowerNb->s.Hi);
      return true;
    }
  }

  // If 'set' tower number is requested
  if (Packet_Parameter1 == 2)
  {
    // If the received packet has an invalid parameter2 or parameter3
    if ((Packet_Parameter2 > 0xFF) || (Packet_Parameter3 > 0xFF))
      return false;

    // Normal action
    else
      return Flash_Write16(&(NvTowerMode->l), Packet_Parameter23);
  }
}


/*! @brief Executes the response to the "Flash - Program byte" command.
 *
 *  @return bool - TRUE if the response was successfully executed.
 */
static bool HandleFlashProgramByte(void)
{
  // If the received packet has any invalid parameters
  if ((Packet_Parameter1 > 0x08) || (Packet_Parameter1 < 0x00) || (Packet_Parameter2 != 0) || (Packet_Parameter3 > 0xFF))
    return false;

  // Normal action

  // If parameter1 is 0x08
  else if (Packet_Parameter1 == 0x08)
    return Flash_Erase();

  // If parameter1 is within the range of 0x00-0x07
  else
  {
    uint32_t offsetAddress = (uint32_t)FLASH_DATA_START + (uint32_t)Packet_Parameter1;
    return Flash_Write8((uint8_t*)offsetAddress, Packet_Parameter3);
  }
}


/*! @brief Executes the response to the "Flash - Read byte" command.
 *
 *  @return bool - TRUE if the response was successfully executed.
 */
static bool HandleFlashReadByte(void)
{
  // If the received packet has any invalid parameters
  if ((Packet_Parameter1 > 0x07) || (Packet_Parameter1 < 0x00) || (Packet_Parameter2 != 0) || (Packet_Parameter3 != 0))
    return false;

  // Normal action
  else
  {
    uint32_t offsetAddress = (uint32_t)FLASH_DATA_START + (uint32_t)Packet_Parameter1;
    uint8_t data = _FB(offsetAddress); // _FB macro function casts it as a 8-bit pointer.
    Packet_Put(FLASH_READ_BYTE, Packet_Parameter1, 0, data);
    return true;
  }
}


/*! @brief Sets the requested time in the TSR register.
 *
 *  @return bool - TRUE if the time was successfully set.
 */
static bool HandleSetTime(void)
{
  // If the received packet has any invalid parameters
  if ((Packet_Parameter1 > 23) || (Packet_Parameter2 > 59) || (Packet_Parameter3 > 59))
    return false;

  // Normal action
  else
  {
    RTC_Set(Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
    return true;
  }
}


/*! @brief Sets the protocol mode if requested, return the current accelerator protocol mode if requested.
 *
 *  @return bool - TRUE if successful.
 */
static bool HandleProtocolMode (void)
{
  if ((Packet_Parameter1 != 1 && Packet_Parameter1 != 2) || (Packet_Parameter2 != 0 && Packet_Parameter2 != 1) || (Packet_Parameter3 != 0))
    return false;

  switch (Packet_Parameter1)
  {
    case (GET_PROTOCOL):
      if (Packet_Parameter2 == ACCEL_POLL)
      {
        Packet_Put(PROTOCOL_MODE, 1, ASYNCHRONOUS, 0);  // Send that Tower is in asynchronous mode
        return true;
      }
      else if (Packet_Parameter2 == ACCEL_INT)
      {
        Packet_Put(PROTOCOL_MODE, 1, SYNCHRONOUS, 0);   // Send that tower is in synchronous mode
        return true;
      }
      break;

    case (SET_PROTOCOL):
      switch (Packet_Parameter2)
      {
        case (ASYNCHRONOUS):
          Accel_SetMode(ACCEL_POLL);
          return true;
          break;

        case (SYNCHRONOUS):
          Accel_SetMode(ACCEL_INT);
          return true;
          break;
      }
      break;
  }

  return false;
}


// PUBLIC FUNCTIONS

void Handle_Packet(void)
{
  // Packet command with acknowledgment bit cleared
  uint8_t command = Packet_Command & ~PACKET_ACK_MASK;

  // Check if acknowledgment was requested or not
  bool acknowledgement = false;

  if ((Packet_Command & PACKET_ACK_MASK) == PACKET_ACK_MASK)
    acknowledgement = true;

  bool success;
  switch (command)
  {
    case (GET_STARTUP_VALUES):
      success = HandleGetStartupValues();
      break;

    case (GET_VERSION):
      success = HandleGetVersion();
      break;

    case (TOWER_NUMBER):
      success = HandleTowerNumber();
      break;

    case (TOWER_MODE):
      success = HandleTowerMode();
      break;

    case (FLASH_PROGRAM_BYTE):
      success = HandleFlashProgramByte();
      break;

    case (FLASH_READ_BYTE):
      success = HandleFlashReadByte();
      break;

    case (SET_TIME):
      success = HandleSetTime();
      break;

    case (PROTOCOL_MODE):
      success = HandleProtocolMode();
      break;

    default:
      success = HandleInvalidCommand();
  }

  if (acknowledgement)
  {
    switch (success)
    {
      case (true):
        SendAckPacket();

      case (false):
        SendNakPacket();
    }
  }
}


bool Initial_TowerNb(void)
{
  if (!Flash_AllocateVar((void*)&NvTowerNb, sizeof(*NvTowerNb)))
    return false;

  else if (NvTowerNb->l == 0xFFFF)
    return Flash_Write16(&(NvTowerNb->l), 1382);

  else
    return Flash_Write16(&(NvTowerNb->l), NvTowerNb->l);
}


 bool Initial_TowerMode(void)
{
  if (!Flash_AllocateVar((void*)&NvTowerMode, sizeof(*NvTowerMode)))
    return false;

  else if (NvTowerMode->l == 0xFFFF)
    return Flash_Write16(&(NvTowerMode->l), 1);

  else
    return Flash_Write16(&(NvTowerMode->l), NvTowerMode->l);
}


bool Tower_Startup(void)
{
  Packet_Put(TOWER_STARTUP, 0, 0, 0);
  Packet_Put(TOWER_VERSION, 'v', 1, 0);
  Packet_Put(TOWER_NUMBER, 1, NvTowerNb->s.Lo, NvTowerNb->s.Hi);
  Packet_Put(TOWER_MODE, 1, NvTowerMode->s.Lo, NvTowerMode->s.Hi);
  return true;
}
