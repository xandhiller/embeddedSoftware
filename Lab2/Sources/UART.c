/*! @file
 *
 *  @brief I/O routines for UART communications on the TWR-K70F120M.
 *
 *  This contains the functions for operating the UART (serial port).
 *
 *  @author 12551382 Samin Saif
 *  @date 2018-04-16
 */

#include "FIFO.h"
#include "UART.h"
#include "Cpu.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"

// new types
#include "types.h"

#define SAMPLE_RATE 16

// Global variables
static TFIFO TxFIFO;  //Transmitting FIFO
static TFIFO RxFIFO;  //Receiving FIFO

/*! @brief Sets up the UART interface before first use.
 *
 *  @param baudRate The desired baud rate in bits/sec.
 *  @param moduleClk The module clock rate in Hz.
 *  @return bool - TRUE if the UART was successfully initialized.
 */
bool UART_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  if (baudRate == 0)
  {
    return FALSE;
  }

  uint16union_t intSBR;
  uint8_t intBRFD;

  // Enable UART2 clock
  SIM_SCGC4 |= SIM_SCGC4_UART2_MASK;
  //Enable PORTE clock
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;

  //Multiplexing PORTE pin 16 to ALT3 (UART2_TX)
  PORTE_PCR16 = PORT_PCR_MUX(3);
  //Multiplexing PORTE pin 17 to ALT3 (UART2_RX)
  PORTE_PCR17 = PORT_PCR_MUX(3);

  // Calculate the SBR
  intSBR.l = moduleClk/(baudRate*SAMPLE_RATE);  // Due to integer division, this will be the integer component, BRFD will be the part after the decimal place.

  // Calculate the BRFD
  // moduleClk % (baudRate*16) will return the part after the decimal place that was lost due to the integer division.
  // Dividing again by (baudRate*16) will give us BRFD
  // Multiplying by 32 will give the BRFD in a union where the upper and lower 8 bits are individually accessible for the registers.
  intBRFD = (moduleClk%(baudRate*SAMPLE_RATE))*2*SAMPLE_RATE/(baudRate*SAMPLE_RATE);

  // Setting baud rate in registers
  // Set BDH to the high half of the SBR union
  UART2_BDH = UART_BDH_SBR(intSBR.s.Hi);
  // Set BDL to the low half of the SBR union
  UART2_BDL = UART_BDL_SBR(intSBR.s.Lo);

  // Set the 5 LSB of UART2_C4 to the determined BRFA
  UART2_C4 = UART_C4_BRFA(intBRFD);

  // Enable UART2 transmitter
  UART2_C2 |= UART_C2_TE_MASK;
  // Enable UART2 receiver
  UART2_C2 |= UART_C2_RE_MASK;

  // Initialise the receiving and transmitting FIFO buffers
  FIFO_Init(&TxFIFO);
  FIFO_Init(&RxFIFO);

  return TRUE;
}

/*! @brief Get a character from the receive FIFO if it is not empty.
 *
 *  @param dataPtr A pointer to memory to store the retrieved byte.
 *  @return bool - TRUE if the receive FIFO returned a character.
 *  @note Assumes that UART_Init has been called.
 */
bool UART_InChar(uint8_t * const dataPtr)
{
  return (FIFO_Get(&RxFIFO, dataPtr));
}

/*! @brief Put a byte in the transmit FIFO if it is not full.
 *
 *  @param data The byte to be placed in the transmit FIFO.
 *  @return bool - TRUE if the data was placed in the transmit FIFO.
 *  @note Assumes that UART_Init has been called.
 */
bool UART_OutChar(const uint8_t data)
{
  return (FIFO_Put(&TxFIFO, data));
}

/*! @brief Poll the UART status register to try and receive and/or transmit one character.
 *
 *  @return void
 *  @note Assumes that UART_Init has been called.
 */
void UART_Poll(void)
{
  // Checks if the RDRF flag is set
  if ((UART2_S1 & UART_S1_RDRF_MASK) != 0)
  {
    FIFO_Put(&RxFIFO, UART2_D);
  }

  // Checks if the TDRE flag is set
  if ((UART2_S1 & UART_S1_TDRE_MASK) != 0)
  {
    FIFO_Get(&TxFIFO, (uint8_t*)&UART2_D);
  }
}