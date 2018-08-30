/*! @file
 *
 *  @brief Routines to implement packet encoding and decoding for the serial port.
 *
 *  This contains the functions for implementing the "Tower to PC Protocol" 5-byte packets.
 *
 *  @author 12551382 Samin Saif and 11850637 Alex Hiller
 *  @date 2018-03-23
 */
/*!
**  @addtogroup packet_module packet module documentation
**  @{
*/
/* MODULE packet */

// Include the Packet header file
#include "brOS.h"

TPacket Packet;
const uint8_t PACKET_ACK_MASK = 0x80;


// PUBLIC FUNCTIONS

bool Packet_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  // Set up channel 0 to blink LED_BLUE when valid packet received
  FTMChLoad[0].channelNb            = 0;
  FTMChLoad[0].delayCount           = CPU_MCGFF_CLK_HZ_CONFIG_0;
  FTMChLoad[0].timerFunction        = TIMER_FUNCTION_OUTPUT_COMPARE;
  FTMChLoad[0].ioType.outputAction  = TIMER_OUTPUT_TOGGLE;  // Arbitrary decision (?)

  // Set up FTM Channel 0
  FTM_Set(&FTMChLoad[0]);

  // Create UART-receive thread
  return UART_Init(baudRate, moduleClk);
}

void Packet_Get(void)
{
  for (;;)
  {
    // This is a blocking function, the rest of Packet_Get will not proceed unless something is in the FIFO
    UART_InChar(&Packet_Checksum);

    // Checksum condition
    if ((Packet_Command^Packet_Parameter1^Packet_Parameter2^Packet_Parameter3) == Packet_Checksum)
    {
      return;
    }

    // Shift each byte in the packet left by 1 so we can check the next byte
    else
    {
      Packet_Command    = Packet_Parameter1;
      Packet_Parameter1 = Packet_Parameter2;
      Packet_Parameter2 = Packet_Parameter3;
      Packet_Parameter3 = Packet_Checksum;
    }
  }
}


void Packet_Put(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
{
  UART_OutChar(command);                                   // Transfer the command byte
  UART_OutChar(parameter1);                                // Transfer the parameter1 byte
  UART_OutChar(parameter2);                                // Transfer the parameter2 byte
  UART_OutChar(parameter3);                                // Transfer the parameter3 byte
  UART_OutChar(command^parameter1^parameter2^parameter3);  // Create and transfer the checksum byte
}


/*!
** @}
*/
