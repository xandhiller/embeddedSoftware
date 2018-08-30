/* ###################################################################
**     Filename    : main.c
**     Project     : Lab1
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
** @version 1.0
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
#include "Cpu.h"
#include "Events.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "types.h"

// Header files from main module
#include "UART.h"
#include "FIFO.h"
#include "packet.h"

// Defines
#define BAUD_RATE 38400
#define SAMPLE_RATE 16
	//PC to tower commands
#define GET_STARTUP_VALUES 0x04
#define GET_VERSION 0x09
#define TOWER_NUMBER 0x0B
	//Tower to PC commands
#define TOWER_STARTUP 0x04
#define TOWER_VERSION 0x09
//#define TOWER_NUMBER 0x0B (already defined)



// Packet components
uint8_t packetCommand;
uint8_t packetParamater1;
uint8_t packetParameter2;
uint8_t packetParameter3;
uint8_t packetChecksum;

// Packet acknowledge mask
const uint8_t PACKET_ACK_MASK = 0x80;

// The tower number is an unsigned 16-bit number 
uint16union_t vTowerNumber;

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /* Write your local variable definition here */
  uint32_t moduleClk = CPU_BUS_CLK_HZ;
  uint32_t baudRate = BAUD_RATE;
  uint8_t uCommand;
  bool bAcknowledge;
  
  //Initialise tower number (Student ID 12551382)
  vTowerNumber.l = 1382;

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/
  
  /* Write your code here */
  
  //Initialise UART2 with determined baud rate and module clock rate
  Packet_Init(baudRate, moduleClk);
  
  //Send PC to tower startup packets
  Packet_Put(TOWER_STARTUP,0,0,0);
  Packet_Put(TOWER_VERSION,'v',1,0);
  Packet_Put(TOWER_NUMBER,1, vTowerNumber.s.Lo, vTowerNumber.s.Hi);

  for (;;)
  {
	  //Start every repetition by checking the RDRF and TDRE flags
      UART_Poll();
	  
	  if (Packet_Get() == TRUE)	//Get a packet
	  {
		uCommand = packetCommand & ~PACKET_ACK_MASK;	//Clear the MSB in uCommand
		bAcknowledge = FALSE;	//Resets to FALSE every repetition
		
		//Check if acknowledge bit in packetCommand is set
		if ((packetCommand & PACKET_ACK_MASK) == PACKET_ACK_MASK)
		{
			bAcknowledge = TRUE;
		}
		
		//Check which tower-to-PC packet corresponds to the command received
		//Check whether or not the acknowledge bit is set
		//Send the appropriate packet
		if (uCommand == GET_STARTUP_VALUES) //Default packet command values assume non-acknowledge
		{
				Packet_Put(TOWER_STARTUP,0,0,0);
				Packet_Put(TOWER_VERSION,'v',1,0);
				Packet_Put(TOWER_NUMBER,1,vTowerNumber.s.Lo,vTowerNumber.s.Hi);
			
			if (bAcknowledge == TRUE)
			{
				Packet_Put(TOWER_STARTUP | PACKET_ACK_MASK,0,0,0);
			}
		}
		
		if (uCommand == GET_VERSION)
		{
			Packet_Put(TOWER_VERSION,'v',1,0);
			
			if (bAcknowledge == TRUE)
			{
				Packet_Put(TOWER_VERSION | PACKET_ACK_MASK,'v',1,0);
			}
		}
		
		if (uCommand == TOWER_NUMBER)
		{
			if (packetParamater1 == 1) //Requesting to get tower number
			{
				Packet_Put(TOWER_NUMBER,1,vTowerNumber.s.Lo, vTowerNumber.s.Hi);
				
				if (bAcknowledge == TRUE)
				{
					Packet_Put(TOWER_NUMBER | PACKET_ACK_MASK,1,vTowerNumber.s.Lo, vTowerNumber.s.Hi);
				}
			}
			
			else if (packetParamater1 == 2) //Requesting to set tower number
			{
				vTowerNumber.s.Lo = packetParameter2;
				vTowerNumber.s.Hi = packetParameter3;
				
				Packet_Put(TOWER_NUMBER,1,vTowerNumber.s.Lo,vTowerNumber.s.Hi);
				
				if (bAcknowledge == TRUE)
				{
					Packet_Put(TOWER_NUMBER | PACKET_ACK_MASK,1,vTowerNumber.s.Lo,vTowerNumber.s.Hi);
				}
			}
		}
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
