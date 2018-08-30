/*
 * packet.c
 *
 *  Created on: 23 Mar 2018
 *      Author: 11850637
 */

#include "packet.h"
#include "UART.h"

bool Packet_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  return UART_Init(baudRate, moduleClk);
}

bool Packet_Get(void)
{
  //Return FALSE if there are less than 5 bytes in the RxFIFO
  if (RxFIFO.NbBytes < 5)
    {
      return FALSE;
    }

  int nFSMstate = 0;	//Keeps track of the FSM state

  for(;;)
  {
      switch(nFSMstate)
      {
	case 0:
	  if (UART_InChar(&packetCommand) == TRUE)
	  {
	      nFSMstate++;
	  }
	  else
	  {
	      return FALSE;
	  }

	case 0:
		  if (UART_InChar(&packetCommand) == TRUE)
		  {
		      nFSMstate++;
		  }
		  else
		  {
		      return FALSE;
		  }

	case 1:
		  if (UART_InChar(&packetParamater1) == TRUE)
		  {
		      nFSMstate++;
		  }
		  else
		  {
		      return FALSE;
		  }

	case 2:
		  if (UART_InChar(&packetParameter2) == TRUE)
		  {
		      nFSMstate++;
		  }
		  else
		  {
		      return FALSE;
		  }

	case 3:
		  if (UART_InChar(&packetParameter3) == TRUE)
		  {
		      nFSMstate++;
		  }
		  else
		  {
		      return FALSE;
		  }

	case 4:
		  if (UART_InChar(&packetChecksum) == TRUE)
		  {
		      nFSMstate++;
		  }
		  else
		  {
		      return FALSE;
		  }

	case 5:
		  if (packetChecksum == packetCommand^packetParameter1^packetParameter2^packetParameter3)
		  {
		      nFSMstate = 0;
		      return TRUE;
		  }
		  else
		  {
		      //Shift each byte in the packet left by 1 if the checksum fails
		      packetCommand = packetParameter1;
		      packetParameter1 = packetParameter2;
		      packetParameter2 = packetParameter3;
		      packetParameter3 = packetChecksum;

		      //Return to FSM state 4
		      nFSMstate--;

		      //Stop building packets if RxFIFO is empty
		      if (RxFIFO.NbBytes == 0)
		      {
			return FALSE;
		      }
		  }
      }
  }
}

bool Packet_Put(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
{
  if (UART_OutChar(command) != TRUE)
    {
      return FALSE;
    }

  else if (UART_OutChar(paramater1) != TRUE)
    {
      return FALSE;
    }

  else if (UART_OutChar(parameter2) != TRUE)
    {
      return FALSE;
    }

  else if (UART_OutChar(parameter3) != TRUE)
    {
      return FALSE;
    }

  //Creates and transfers the checksum byte
  else if (UART_OutChar(command^parameter1^parameter2^parameter3) != TRUE)
    {
      return FALSE;
    }

  else
    {
      return TRUE;
    }
}
