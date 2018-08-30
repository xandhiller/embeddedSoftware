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
#include "PIT.h"
#include "RTC.h"
#include "FTM.h"
#include "accel.h"
#include "I2C.h"

// PC to Tower commands
#define GET_STARTUP_VALUES  0x04
#define GET_VERSION         0x09
#define TOWER_NUMBER        0x0B
#define TOWER_MODE          0x0D
#define FLASH_PROGRAM_BYTE  0x07
#define FLASH_READ_BYTE     0x08
#define SET_TIME            0x0C
#define PROTOCOL_MODE       0x0A
#define ACCEL_VALUES	    0x10

// Tower to PC commands
#define TOWER_STARTUP       0x04
#define TOWER_VERSION       0x09
#define PROTOCOL_MODE	    0x0A

// Accelerometer macros
#define GET_PROTOCOL	    0x01
#define SET_PROTOCOL	    0x02
#define ASYNCHRONOUS	    0x00
#define SYNCHRONOUS	    0x01

// Desired baud rate
static const uint32_t BAUD_RATE = 115200;

// Pointers to be allocated flash memory
static volatile uint16union_t *NvTowerNb;    // Non-volatile tower number
static volatile uint16union_t *NvTowerMode;  // Non-volatile tower mode

// FTM Channel 0 Structure
static TFTMChannel Channel0Load;

// Measured in nanoseconds, required for PIT_Set() call in main.
const uint32_t Timer0Period = 5e8;

// Accelerometer setup structure
static TAccelSetup AccelSetup;

// Accelerometer mode setup
static uint32_t AccelMode;

// Accelerometer data
struct
{
  TAccelData Buffer[3];
  TAccelData FilteredAccelData;
  uint8_t Index;
} AccelDataBuffer;


// User Defined Function Prototypes

/*! @brief Calls all initialisation functions.
 *
 *  @return bool - TRUE if all modules are successfully initialised.
 */
bool TowerInit(void);

/*! @brief Initialise global structures that need to be loaded.
 */
bool GlobalsInit(void);

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

/*! @brief Toggles the green LED.
 *
 */
void PITCallback(void* arg);

/*! @brief Toggles the blue LED.
 *
 */
void FTMCallback(void* arg);

/*! @brief Sends out the current time in the RTC TSR in hours, minutes, and seconds.
 *
 */
static void UpdateTimeRTC(void);

/*! @brief Sets callback function to be called when I2C is finished.
 *
 */
void AccelCallbackReadComplete(void);

/*! @brief Sets callback function to be called when Accel_ISR runs.
 *
 */
void AccelCallbackDataReady(void);

/*! @brief Set all the variables in the Public Global data buffer to zero.
 *
 */
bool AccelBufferInit(void);

/*! @brief Compare most recent x, y and z values with previous value in buffer.
 *
 *  @return BOOL - TRUE if the newest data is not similar to the previous entry
 */
bool AccelCheckIfNewData(void);

/*! @brief Sends the packet received back to the PC, with the acknowledgement bit set.
 *
 *  @return bool - TRUE if ACK packet is successfully sent.
 */
bool SendAckPacket(void);

/*! @brief Sends the packet received back to the PC, with the acknowledgement bit cleared.
 *
 *  @return bool - TRUE if NAK packet is successfully sent.
 */
bool SendNakPacket(void);

/*! @brief Executes the response to an unknown command.
 *
 *  @return bool - TRUE if the response was successfully executed.
 */
 bool HandleInvalidCommand(void);

/*! @brief Executes the response to the "Special - Get startup values" command.
 *
 *  @return bool - TRUE if the response was successfully executed.
 */
bool HandleGetStartupValues(void);

/*! @brief Executes the response to the "Special - Get version" command.
 *
 *  @return bool - TRUE if the response was successfully executed.
 */
bool HandleGetVersion(void);

/*! @brief Executes the response to the "Tower Number" command.
 *
 *  @return bool - TRUE if the response was successfully executed.
 */
bool HandleTowerNumber(void);

/*! @brief Executes the response to the "Tower Mode" command.
 *
 *  @return bool - TRUE if the response was successfully executed.
 */
bool HandleTowerMode(void);

/*! @brief Executes the response to the "Flash - Program byte" command.
 *
 *  @return bool - TRUE if the response was successfully executed.
 */
bool HandleFlashProgramByte(void);

/*! @brief Executes the response to the "Flash - Read byte" command.
 *
 *  @return bool - TRUE if the response was successfully executed.
 */
bool HandleFlashReadByte(void);

/*! @brief Sets the requested time in the TSR register.
 *
 *  @return bool - TRUE if the time was successfully set.
 */
static bool HandleSetTime(void);

/*! @brief Sets the protocol mode if requested, return the current accelerator protocol mode if requested.
 *
 *  @return bool - TRUE if successful.
 */
bool HandleProtocolMode (void);

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

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/

  /* Write your code here */

  // Disable all interrupts whilst setup is performed
  __DI();

  // Turn on orange LED if Flash and UART are initialised successfully
  if (TowerInit())
    LEDs_On(LED_ORANGE);

  // Send the 4 initial packets on tower startup
  TowerStartup();

  // Set the value of PIT
  PIT_Set(Timer0Period, true);

  // Set FTM0 characteristics
  FTM_Set(&Channel0Load);

  // Setup accelerometer in interrupt mode or polling mode
  Accel_SetMode(ACCEL_POLL);

  // Setup complete, enable all interrupts and hardware exceptions.
  __EI();

  for (;;)
  {
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
  return (GlobalsInit() &&	  			  // Initialise global variables
	  FTM_Init() &&                                   // Initialise FTM module
          RTC_Init((void*)UpdateTimeRTC, NULL) &&         // Initialise RTC module
	  PIT_Init(CPU_BUS_CLK_HZ, PITCallback, NULL) &&  // Initialise PIT module
          Packet_Init(BAUD_RATE, CPU_BUS_CLK_HZ) &&       // Initialise packet module
          LEDs_Init() &&                                  // Initialise LED module
          Flash_Init() &&				  // Initialise flash module
	  Accel_Init(&AccelSetup) &&			  // Initialise the accelerometer
	  AccelBufferInit() &&				  // Zero the values in the Accelerometer value buffer
	  (InitialTowerNb() || true) &&                   // Write tower number as last 4 digits of student number to flash
	  (InitialTowerMode() || true));                  // Write tower mode as 1 to flash
}

/*! @brief Initialise global structures that need to be loaded.
 */
bool GlobalsInit(void)
{
  // Passing in the channel 0 set up information
  Channel0Load.channelNb            = 0;
  Channel0Load.delayCount           = CPU_MCGFF_CLK_HZ_CONFIG_0;
  Channel0Load.timerFunction        = TIMER_FUNCTION_OUTPUT_COMPARE;
  Channel0Load.ioType.outputAction  = TIMER_OUTPUT_TOGGLE;  // Arbitrary decision (?)
  Channel0Load.userFunction         = FTMCallback;
  Channel0Load.userArguments        = NULL;

  // Set up the accelerometer setup structure
  AccelSetup.moduleClk                      = CPU_BUS_CLK_HZ;
  AccelSetup.dataReadyCallbackFunction      = (void*)&AccelCallbackDataReady;
  AccelSetup.dataReadyCallbackArguments     = NULL;
  AccelSetup.readCompleteCallbackFunction   = (void*)&AccelCallbackReadComplete;
  AccelSetup.readCompleteCallbackArguments  = NULL;

  // Set Accelerometer mode
  AccelMode = ACCEL_POLL;

  return true;
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

/*! @brief Toggles the green LED.
 *
 */
void PITCallback(void* arg)
{
//  LEDs_Toggle(LED_GREEN);
  // Doing nothing because RTC is controlling green LED for accelerometer (Lab 4)
}

/*! @brief Toggles the blue LED.
 *
 */
void FTMCallback(void* arg)
{
  LEDs_Off(LED_BLUE);
}

/*! @brief Sends out the current time in the RTC TSR in hours, minutes, and seconds.
 *
 */
static void UpdateTimeRTC(void)
{
   // Local variables for storing the time in the RTC TSR
   uint8_t hours, minutes, seconds;

   // Get time from RTC TSR in hours, minutes, and seconds
   RTC_Get(&hours, &minutes, &seconds);

   // Send time to PC
   Packet_Put(SET_TIME, hours, minutes, seconds);

   // Toggle yellow LED
   LEDs_Toggle(LED_YELLOW);

   if (AccelMode == ACCEL_POLL)
   {
     // Call the polling of the accelerometer, save to global variable
     Accel_ReadXYZ(ACCEL_BUFFER_DATA_INPUT);
     // Send to PC if new
     if (AccelCheckIfNewData())
     {
       Accel_FilterData();
       Packet_Put(ACCEL_VALUES, FILTERED_DATA_X, FILTERED_DATA_Y, FILTERED_DATA_Z);
       LEDs_Toggle(LED_GREEN);
     }
   }
}

/*! @brief Sets callback function to be called when I2C is finished.
 *
 */
void AccelCallbackReadComplete(void)
{
  Accel_FilterData();

  Packet_Put(ACCEL_VALUES, FILTERED_DATA_X, FILTERED_DATA_Y, FILTERED_DATA_Z);
}

/*! @brief Sets callback function to be called when Accel_ISR runs.
 *
 */
void AccelCallbackDataReady(void)
{
  Accel_ReadXYZ(ACCEL_BUFFER_DATA_INPUT);
}

/*! @brief Set all the variables in the Public Global data buffer to zero.
 *
 */
bool AccelBufferInit(void)
{
  // Zero all the things.
  AccelDataBuffer.Buffer[0].axes.x = 0;
  AccelDataBuffer.Buffer[0].axes.y = 0;
  AccelDataBuffer.Buffer[0].axes.z = 0;
  AccelDataBuffer.Buffer[1].axes.x = 0;
  AccelDataBuffer.Buffer[1].axes.y = 0;
  AccelDataBuffer.Buffer[1].axes.z = 0;
  AccelDataBuffer.Buffer[2].axes.x = 0;
  AccelDataBuffer.Buffer[2].axes.y = 0;
  AccelDataBuffer.Buffer[2].axes.z = 0;
  AccelDataBuffer.FilteredAccelData.axes.x = 0;
  AccelDataBuffer.FilteredAccelData.axes.y = 0;
  AccelDataBuffer.FilteredAccelData.axes.z = 0;
  AccelDataBuffer.Index = 0;
  return true;
}

/*! @brief Compare most recent x, y and z values with previous value in buffer.
 *
 *  @return BOOL - TRUE if the newest data is not similar to the previous entry
 */
bool AccelCheckIfNewData(void)
{
  switch (ACCEL_BUFFER_INDEX)
  {
    case (0):
    if (AccelDataBuffer.Buffer[2].bytes == AccelDataBuffer.Buffer[1].bytes)
    {
      return false;
    }
    else
    {
      return true;
    }
    break;

    case (1):
    if (AccelDataBuffer.Buffer[0].bytes == AccelDataBuffer.Buffer[2].bytes)
    {
      return false;
    }
    else
    {
      return true;
    }
    break;

    case (2):
    if (AccelDataBuffer.Buffer[0].bytes == AccelDataBuffer.Buffer[1].bytes)
    {
      return false;
    }
    else
    {
      return true;
    }
    break;
  }
}

/*! @brief Sends the packet received back to the PC, with the acknowledgement bit set.
 *
 *  @return bool - TRUE if ACK packet is successfully sent.
 */
bool SendAckPacket(void)
{
  return Packet_Put(Packet_Command | PACKET_ACK_MASK, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
}

/*! @brief Sends the packet received back to the PC, with the acknowledgement bit cleared.
 *
 *  @return bool - TRUE if NAK packet is successfully sent.
 */
bool SendNakPacket(void)
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

/*! @brief Sets the requested time in the TSR register.
 *
 *  @return bool - TRUE if the time was successfully set.
 */
bool HandleSetTime(void)
{
  // If the received packet has any invalid parameters
  if ((Packet_Parameter1 > 23) || (Packet_Parameter2 > 59) || (Packet_Parameter3 > 59))
    return false;

  // Normal action
  else
    RTC_Set(Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
    return true;
}

/*! @brief Sets the protocol mode if requested, return the current accelerator protocol mode if requested.
 *
 *  @return bool - TRUE if successful.
 */
bool HandleProtocolMode (void)
{
  if ((Packet_Parameter1 != 1 && Packet_Parameter1 != 2) || (Packet_Parameter2 != 0 && Packet_Parameter2 != 1) || (Packet_Parameter3 != 0))
    return false;

  switch(Packet_Parameter1)
  {
    case (GET_PROTOCOL):
    if (AccelMode == ACCEL_POLL)
    {
      // Send that Tower is in asynchronous mode
      Packet_Put(PROTOCOL_MODE, 1, ASYNCHRONOUS, 0);
      return true;
    }
    if (AccelMode == ACCEL_INT)
    {
      // Send that tower is in synchronous mode
      Packet_Put(PROTOCOL_MODE, 1, SYNCHRONOUS, 0);
      return true;
    }
    break;

    case (SET_PROTOCOL):
      switch (Packet_Parameter2)
      {
        case (ASYNCHRONOUS):
          AccelMode = ASYNCHRONOUS;
          Accel_SetMode(AccelMode);
          return true;
          break;
        case (SYNCHRONOUS):
          AccelMode = SYNCHRONOUS;
          Accel_SetMode(AccelMode);
          return true;
          break;
      }
      break;
  }

  return false;
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

  // Turn on the BLUE LED for one second using the FTM module.
  LEDs_On(LED_BLUE);
  // Start a timer that will turn off the LED in 1 second
  FTM_StartTimer(&Channel0Load);

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
        return SendAckPacket();

      case (false):
        return SendNakPacket();
    }
  }
  else
    {
      return success;
    }
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
