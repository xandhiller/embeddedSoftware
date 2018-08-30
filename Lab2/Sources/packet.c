/*! @file
 *
 *  @brief Routines to implement packet encoding and decoding for the serial port.
 *
 *  This contains the functions for implementing the "Tower to PC Protocol" 5-byte packets.
 *
 *  @author 12551382 Samin Saif
 *  @date 2018-03-23
 */

// Include the Packet header file
#include <stdbool.h>
#include "packet.h"
#include "UART.h"

TPacket Packet;
const uint8_t PACKET_ACK_MASK   = 0x80;

/*! @brief Initializes the packets by calling the initialization routines of the supporting software modules.
 *
 *  @param baudRate The desired baud rate in bits/sec.
 *  @param moduleClk The module clock rate in Hz
 *  @return bool - TRUE if the packet module was successfully initialized.
 */
bool Packet_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  return UART_Init(baudRate, moduleClk);
}

/*! @brief Attempts to get a packet from the received data.
 *
 *  @return bool - TRUE if a valid packet was received.
 */
bool Packet_Get(void)
{
  while (UART_InChar(&Packet_Checksum))
  {
    // Checksum condition
    if ((Packet_Command^Packet_Parameter1^Packet_Parameter2^Packet_Parameter3) == Packet_Checksum)
    {
      return true;
    }

    // Shift each byte in the packet left by 1
    else
    {
      Packet_Command    = Packet_Parameter1;
      Packet_Parameter1 = Packet_Parameter2;
      Packet_Parameter2 = Packet_Parameter3;
      Packet_Parameter3 = Packet_Checksum;
    }
  }

  return false;
}

/*! @brief Builds a packet and places it in the transmit FIFO buffer.
 *
 *  @param command First byte in the packet
 *  @param parameter1 Second byte in the packet
 *  @param parameter2 Third byte in the packet
 *  @param parameter3 Fourth byte in the packet
 *  @return bool - TRUE if a valid packet was sent.
 */
bool Packet_Put(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
{
  // Transfer the command byte
  if (!UART_OutChar(command))
    return false;

  // Transfer the parameter1 byte
  else if (!UART_OutChar(parameter1))
    return false;

  // Transfer the parameter2 byte
  else if (!UART_OutChar(parameter2))
    return false;

  // Transfer the parameter3 byte
  else if (!UART_OutChar(parameter3))
    return false;

  // Create and transfer the checksum byte
  else if (!UART_OutChar(command^parameter1^parameter2^parameter3))
    return false;

  // Return true if a valid packet was sent
  else
    return true;
}
