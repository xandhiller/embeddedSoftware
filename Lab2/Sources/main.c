/* ###################################################################
**     Filename    : main.c
**     Project     : Lab2
**     Processor   : MK70FN1M0VMJ12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2015-07-20, 13:27, # CodeGen: 0
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 2.0
** @brief
**         Main module.
**         This module contains user's application code.
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */


// CPU module - contains low level hardware initialization routines
#include <stdbool.h>
#include "Cpu.h"
#include "types.h"
#include "Events.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"

//Main Module Header files
#include "FIFO.h"
#include "UART.h"
#include "packet.h"
#include "LEDs.h"
#include "Flash.h"

// PC to Tower commands
#define GET_STARTUP_VALUES  0x04
#define GET_VERSION         0x09
#define TOWER_NUMBER        0x0B
#define TOWER_MODE          0x0D
#define FLASH_PROGRAM_BYTE  0x07
#define FLASH_READ_BYTE     0x08

// Tower to PC commands
#define TOWER_STARTUP       0x04
#define TOWER_VERSION       0x09

// Desired baud rate
static const uint32_t BAUD_RATE = 115200;

// Pointers to be allocated flash memory
static volatile uint16union_t *NvTowerNb;    // Non-volatile tower number
static volatile uint16union_t *NvTowerMode;  // Non-volatile tower mode


// User Defined Function Prototypes

/*! @brief Calls all initialisation functions.
 *
 *  @return bool - TRUE if all modules are successfully initialised.
 */
bool TowerInit(void);

/*! @brief Sends the initial 4 packets when tower starts up.
 *
 *  @return bool - TRUE if the 4 packets are all successfully sent.
 */
bool TowerStartup(void);

/*! @brief Sets the tower number to last 4 digits of student number and stores in flash if tower number has not yet been written.
 *
 *  @return bool - TRUE if the tower number was successfully stored in flash.
 */
bool InitialTowerNb(void);

/*! @brief Sets the tower mode to 1 and stores in flash if tower mode has not yet been set.
 *
 *  @return bool - TRUE if the tower mode was successfully stored in flash.
 */
bool InitialTowerMode(void);

/*! @brief Sends the packet received back to the PC, with the acknowledgement bit set.
 *
 *  @return bool - TRUE if ACK packet is successfully sent.
 */
bool sendAckPacket(void);

/*! @brief Sends the packet received back to the PC, with the acknowledgement bit cleared.
 *
 *  @return bool - TRUE if NAK packet is successfully sent.
 */
bool sendNakPacket(void);

/*! @brief Executes the response to an unknown command.
 *
 *  @param acknowledgement Command acknowledgement flag. 
 *  @return bool - TRUE if the response was successfully executed.
 */
 bool HandleInvalidCommand(void);

/*! @brief Executes the response to the "Special - Get startup values" command.
 *
 *  @param acknowledgement Command acknowledgement flag. 
 *  @return bool - TRUE if the response was successfully executed.
 */
bool HandleGetStartupValues(void);

/*! @brief Executes the response to the "Special - Get version" command.
 *
 *  @param acknowledgement Command acknowledgement flag.
 *  @return bool - TRUE if the response was successfully executed.
 */
bool HandleGetVersion(void);

/*! @brief Executes the response to the "Tower Number" command.
 *
 *  @param acknowledgement Command acknowledgement flag.
 *  @return bool - TRUE if the response was successfully executed.
 */
bool HandleTowerNumber(void);

/*! @brief Executes the response to the "Tower Mode" command.
 *
 *  @param acknowledgement Command acknowledgement flag.
 *  @return bool - TRUE if the response was successfully executed.
 */
bool HandleTowerMode(void);

/*! @brief Executes the response to the "Flash - Program byte" command.
 *
 *  @param acknowledgement Command acknowledgement flag.
 *  @return bool - TRUE if the response was successfully executed.
 */
bool HandleFlashProgramByte(void);

/*! @brief Executes the response to the "Flash - Read byte" command.
 *
 *  @param acknowledgement Command acknowledgement flag.
 *  @return bool - TRUE if the response was successfully executed.
 */
bool HandleFlashReadByte(void);

/*! @brief Calls the appropriate "Handle" function based on the command received.
 *
 *  @return bool - TRUE if the command received was successfully executed.
 */
bool PacketHandle(void);


/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /* Write your local variable definition here */
  bool initSuccess;

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/

  /* Write your code here */

  // Initialise all necessary registers
  initSuccess = TowerInit();

  // Write the tower number as last 4 digits of student number and tower mode as 1 to flash if erased
  initSuccess = InitialTowerNb();
  initSuccess = InitialTowerMode();

  // Turn on orange LED if Flash and UART are initialised successfully
  if (initSuccess)
    LEDs_On(LED_ORANGE);

  // Send the 4 initial packets on tower startup
  TowerStartup();

  for (;;)
  {
    UART_Poll();

    if (!PacketHandle())
    {
      continue;
    }
  }

  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/


// User Defined Functions

/*! @brief Calls all initialisation functions.
 *
 *  @return bool - TRUE if all modules are successfully initialised.
 */
bool TowerInit(void)
{
  return (Packet_Init(BAUD_RATE, CPU_BUS_CLK_HZ) && LEDs_Init() && Flash_Init());
}

/*! @brief Sends the initial 4 packets when tower starts up.
 *
 *  @return bool - TRUE if the 4 packets are all successfully sent.
 */
bool TowerStartup(void)
{
  return (Packet_Put(TOWER_STARTUP, 0, 0, 0) &&
          Packet_Put(TOWER_VERSION, 'v', 1, 0) &&
          Packet_Put(TOWER_NUMBER, 1, NvTowerNb->s.Lo, NvTowerNb->s.Hi) &&
          Packet_Put(TOWER_MODE, 1, NvTowerMode->s.Lo, NvTowerMode->s.Hi));
}

/*! @brief Sets the tower number to last 4 digits of student number and stores in flash if tower number has not yet been written.
 *
 *  @return bool - TRUE if the tower number was successfully stored in flash.
 */
bool InitialTowerNb(void)
{
  if (!Flash_AllocateVar((void*)&NvTowerNb, sizeof(*NvTowerNb)))
    return false;

  else if (NvTowerNb->l == 0xFFFF)
    return Flash_Write16(&(NvTowerNb->l), 1382);

  else
    return Flash_Write16(&(NvTowerNb->l), NvTowerNb->l);
}

/*! @brief Sets the tower mode to 1 and stores in flash if tower mode has not yet been set.
 *
 *  @return bool - TRUE if the tower mode was successfully stored in flash.
 */
bool InitialTowerMode(void)
{
  if (!Flash_AllocateVar((void*)&NvTowerMode, sizeof(*NvTowerMode)))
    return false;

  else if (NvTowerMode->l == 0xFFFF)
    return Flash_Write16(&(NvTowerMode->l), 1);

  else
    return Flash_Write16(&(NvTowerMode->l), NvTowerMode->l);
}

/*! @brief Sends the packet received back to the PC, with the acknowledgement bit set.
 *
 *  @return bool - TRUE if ACK packet is successfully sent.
 */
bool sendAckPacket(void)
{
  return Packet_Put(Packet_Command | PACKET_ACK_MASK, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
}

/*! @brief Sends the packet received back to the PC, with the acknowledgement bit cleared.
 *
 *  @return bool - TRUE if NAK packet is successfully sent.
 */
bool sendNakPacket(void)
{
  return Packet_Put(Packet_Command & ~PACKET_ACK_MASK, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
}

/*! @brief Executes the response to an unknown command.
 *
 *  @return bool - TRUE
 */
 bool HandleInvalidCommand(void)
{
  return false;
}

/*! @brief Executes the response to the "Special - Get startup values" command.
 *
 *  @return bool - TRUE if the response was successfully executed.
 */
bool HandleGetStartupValues(void)
{
  // If the received packet has any invalid parameters
  if ((Packet_Parameter1 != 0) || (Packet_Parameter2 != 0) || (Packet_Parameter3 != 0))
    return false;

  // Normal action
  else
    return (Packet_Put(TOWER_STARTUP, 0, 0, 0) &&
            Packet_Put(TOWER_VERSION, 'v', 1, 0) &&
            Packet_Put(TOWER_NUMBER, 1, NvTowerNb->s.Lo, NvTowerNb->s.Hi) &&
            Packet_Put(TOWER_MODE, 1, NvTowerMode->s.Lo, NvTowerMode->s.Hi));
}

/*! @brief Executes the response to the "Special - Get version" command.
 *
 *  @return bool - TRUE if the response was successfully executed.
 */
bool HandleGetVersion(void)
{
  // If the received packet has any invalid parameters
  if ((Packet_Parameter1 != 'v') || (Packet_Parameter2 != 'x') || (Packet_Parameter3 != 0x0D))
    return false;

  // Normal action
  else
    return Packet_Put(TOWER_VERSION, 'v', 1, 0);
}

/*! @brief Executes the response to the "Tower Number" command.
 *
 *  @return bool - TRUE if the response was successfully executed.
 */
bool HandleTowerNumber(void)
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
      return Packet_Put(TOWER_NUMBER, 1, NvTowerNb->s.Lo, NvTowerNb->s.Hi);
  }

  // If 'set' tower number is requested
  if (Packet_Parameter1 == 2)
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
bool HandleTowerMode(void)
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
      return Packet_Put(TOWER_MODE, 1, NvTowerNb->s.Lo, NvTowerNb->s.Hi);
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
bool HandleFlashProgramByte(void)
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
bool HandleFlashReadByte(void)
{
  // If the received packet has any invalid parameters
  if ((Packet_Parameter1 > 0x07) || (Packet_Parameter1 < 0x00) || (Packet_Parameter2 != 0) || (Packet_Parameter3 != 0))
    return false;

  // Normal action
  else
  {
    uint32_t offsetAddress = (uint32_t)FLASH_DATA_START + (uint32_t)Packet_Parameter1;
    uint8_t data = _FB(offsetAddress); // _FB macro function casts it as a 8-bit pointer.
    return Packet_Put(FLASH_READ_BYTE, Packet_Parameter1, 0, data);
  }
}

/*! @brief Calls the appropriate "Handle" function based on the command received.
 *
 *  @return bool - TRUE if the command received was successfully executed.
 */
bool PacketHandle(void)
{
  // Get a valid packet
  if (!(Packet_Get()))
  {
    return false;
  }

  // Packet command with acknowledgement bit cleared
  uint8_t command = Packet_Command & ~PACKET_ACK_MASK;

  // Check if acknowledgement was requested or not
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

    default:
      success = HandleInvalidCommand();
  }

  if (acknowledgement)
  {
    switch (success)
    {
      case (true):
        return sendAckPacket();

      case (false):
        return sendNakPacket();
    }
  }

  else
    return success;
}

/* END main */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.5 [05.21]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
