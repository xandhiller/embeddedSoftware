/*
 * UART2.c
 *
 *  Created on: 23 Mar 2018
 *      Author: 11850637
 */

#include "UART.h"
#include "Cpu.h"
#include "Events.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "FIFO.h"

#define SAMPLE_RATE 16

// Global variables
TFIFO TxFIFO;	//Transmitting FIFO
TFIFO RxFIFO;	//Receiving FIFO

bool UART_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  if (baudRate == 0)
    {
      return FALSE;
    }

  uint16union_t vSBR;
  uint8_t uBRFD;

  // Enable UART2 clock
  SIM_SCGC4 |= SIM_SCGC4_UART2_MASK;
  //Enable PORTE clock
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;

  //Multiplexing PORTE pin 16 to ALT3 (UART2_TX)
  PORTE_PCR16 = PORT_PCR_MUX(3);
  //Multiplexing PORTE pin 17 to ALT3 (UART2_RX)
  PORTE_PCR17 = PORT_PCR_MUX(3);

  // Calculate the SBR
  vSBR.l = moduleClk/(baudRate*SAMPLE_RATE); // Due to integer division, this will be the integer component, BRFD will be the part after the decimal place.

  // Calculate the BRFD
  // moduleClk % (baudRate*16) will return the part after the decimal place that was lost due to the integer division.
  // Dividing again by (baudRate*16) will give us BRFD
  // Multiplying by 32 will give the BRFD in a union where the upper and lower 8 bits are individually accessible for the registers.
  uBRFD = (moduleClk%(baudRate*SAMPLE_RATE))*2*SAMPLE_RATE/(baudRate*SAMPLE_RATE);

  //Setting baud rate in registers
  //Set BDH to the high half of the SBR union
  UART2_BDH = UART_BDH_SBR(vSBR.s.Hi);
  //Set BDL to the low half of the SBR union
  UART2_BDH = UART_BDH_SBR(vSBR.s.Lo);

  //Set the 5 LSB of UART2_C4 to the determined BRFA
  UART2_C4 = UART_C4_BRFA(uBRFD);

  //Enable UART2 transmitter
  UART2_C2 |= UART_C2_TE_MASK;
  //Enable UART2 receiver
  UART2_C2 |= UART_C2_RE_MASK;

  //Initialise the receiving and transmitting FIFO buffers
  FIFO_Init(&TxFIFO);
  FIFO_Init(&RxFIFO);

  return TRUE;
}

bool UART_InChar(uint8_t * const dataPtr)
{
  return (FIFO_Get(&RxFIFO, dataPtr));
}

bool UART_OutChar(const uint8_t data)
{
  return (FIFO_Put(&TxFIFO, data));
}

void UART_Poll(void)
{
  //Checks if the RDRF flag is set
  if ((UART2_S1 & UART_S1_RDRF_MASK) != 0)
  {
      FIFO_Put(&RxFIFO, UART2_D);
  }

  //Checks if the TDRE flag is set
  if ((UART2_S1 & UART_S1_TDRE_MASK) != 0)
  {
      FIFO_Get(&TxFIFO, &UART2_D);
  }
}
