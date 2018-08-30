/*! @file
 *
 *  @brief Routines to implement packet handling.
 *
 *  This contains the functions for handling received packets.
 *
 *  @author 12551382 Samin Saif and 11850637 Alex Hiller
 *  @date 2018-06-27
 */
/*!
**  @addtogroup handle_module handle module documentation
**  @{
*/
/* MODULE handle */

#include "brOS.h"

extern TPacket Packet;

// Pointers to be allocated flash memory
static volatile uint16union_t *NvTowerNb;    // Non-volatile tower number
static volatile uint16union_t *NvTowerMode;  // Non-volatile tower mode

// PRIVATE FUNCTIONS

static void SendAckPacket(void)
{
  Packet_Put(Packet_Command | PACKET_ACK_MASK, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
}


static void SendNakPacket(void)
{
  Packet_Put(Packet_Command & ~PACKET_ACK_MASK, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
}


static bool HandleInvalidCommand(void)
{
  return false;
}


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

static bool HandleTimingMode(void)
{
  if (AlarmStopWatch.TimingType == DEF_TIMING)
  {
    AlarmStopWatch.TimingType = INV_TIMING;
    return true;
  }
  else if (AlarmStopWatch.TimingType == INV_TIMING)
  {
    AlarmStopWatch.TimingType = DEF_TIMING;
    return true;
  }
  else
  {
    return false;
  }
}

static bool HandleNbRaises(void)
{
  if (Packet_Parameter1 == GET_NV)
  {
    Packet_Put(Packet_Command, PhaseA.NvNbRaises->s.Lo, PhaseA.NvNbRaises->s.Hi, 0);
  }

  if (Packet_Parameter1 == RESET_NV)
  {
    // TODO
  }

  return true;
}

static bool HandleNbLowers(void)
{
    if (Packet_Parameter1 == GET_NV)
    {
      Packet_Put(Packet_Command, PhaseA.NvNbLowers->s.Lo, PhaseA.NvNbLowers->s.Hi, 0);
    }

    if (Packet_Parameter1 == RESET_NV)
    {
      // TODO
    }

    return true;
}

static bool HandleFreq(void)
{
  uint16union_t freq = (uint16union_t)((uint16_t)((int)(10*AlarmStopWatch.WaveformFrequency)));
  Packet_Put(Packet_Command, freq.s.Lo, freq.s.Hi, 0);
}

static bool HandleVoltage(void)
{
  uint16union_t RmsValIntA = (uint16union_t)((uint16_t)((int)PhaseA.RmsVal));
  uint16union_t RmsValIntB = (uint16union_t)((uint16_t)((int)PhaseB.RmsVal));
  uint16union_t RmsValIntC = (uint16union_t)((uint16_t)((int)PhaseC.RmsVal));
  switch (Packet_Parameter1)
  {
    case (CHANNEL_A):
        Packet_Put(Packet_Command, CHANNEL_A, RmsValIntA.s.Lo, RmsValIntA.s.Hi);
        break;
    case (CHANNEL_B):
        Packet_Put(Packet_Command, CHANNEL_B, RmsValIntB.s.Lo, RmsValIntB.s.Hi);
        break;
    case (CHANNEL_C):
        Packet_Put(Packet_Command, CHANNEL_C, RmsValIntC.s.Lo, RmsValIntC.s.Hi);
        break;
    default:
      HandleInvalidCommand();
  }
}

static bool HandleSpectrum(void)
{

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

    case (TIMING_MODE):
      success = HandleTimingMode();
      break;

    case (NB_RAISES):
      success = HandleNbRaises();
      break;

    case (NB_LOWERS):
      success = HandleNbLowers();
      break;

    case (FREQ):
      success = HandleFreq();
      break;

    case (VOLTAGE):
      success = HandleVoltage();
      break;

    case (SPECTRUM):
      success = HandleSpectrum();
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
        break;

      case (false):
        SendNakPacket();
        break;
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

