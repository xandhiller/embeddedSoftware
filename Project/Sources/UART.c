/*! @file
 *
 *  @brief I/O routines for UART communications on the TWR-K70F120M.
 *
 *  This contains the functions for operating the UART (serial port).
 *
 *  @author 12551382 Samin Saif and 11850637 Alex Hiller
 *  @date 2018-05-29
 */
/*!
**  @addtogroup UART_module UART module documentation
**  @{
*/
/* MODULE UART */

#include "FIFO.h"
#include "brOS.h"
#include "Cpu.h"

#define SAMPLE_RATE 16

static TFIFO TxFIFO;
static TFIFO RxFIFO;

static OS_ECB* RxReady;
static OS_ECB* TxReady;
extern OS_ECB* ByteReceived;

static uint8_t RxByte;

OS_THREAD_STACK (UART_RxThreadStack,      THREAD_STACK_SIZE);
OS_THREAD_STACK (UART_TxThreadStack,      THREAD_STACK_SIZE);

// PUBLIC FUNCTIONS

bool UART_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  if (baudRate == 0)
  {
    return false;
  }

  // FIFO_Init also initialises the buffer semaphores
  FIFO_Init(&TxFIFO);
  FIFO_Init(&RxFIFO);

  uint16union_t intSBR;
  uint8_t intBRFD;

  SIM_SCGC4 |= SIM_SCGC4_UART2_MASK;      // Enable UART2 clock
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;      // Enable PORTE clock

  PORTE_PCR16 = PORT_PCR_MUX(3);          // Multiplexing PORTE pin 16 to ALT3 (UART2_TX)
  PORTE_PCR17 = PORT_PCR_MUX(3);          // Multiplexing PORTE pin 17 to ALT3 (UART2_RX)

  // Calculate the SBR
  // Due to integer division, this will be the integer component, BRFD will be the part after the decimal place.
  intSBR.l = moduleClk/(baudRate*SAMPLE_RATE);

  // Calculate the BRFD
  // moduleClk % (baudRate*16) will return the part after the decimal place that was lost due to the integer division.
  // Dividing again by (baudRate*16) will give us BRFD
  // Multiplying by 32 will give the BRFD in a union where the upper and lower 8 bits are individually accessible for the registers.
  intBRFD = (moduleClk%(baudRate*SAMPLE_RATE))*2*SAMPLE_RATE/(baudRate*SAMPLE_RATE);

  // Setting baud rate in registers
  UART2_BDH = UART_BDH_SBR(intSBR.s.Hi);  // Set BDH to the high half of the SBR union
  UART2_BDL = UART_BDL_SBR(intSBR.s.Lo);  // Set BDL to the low half of the SBR union

  UART2_C4 = UART_C4_BRFA(intBRFD);       // Set the 5 LSB of UART2_C4 to the determined BRFA

  UART2_C2 |= UART_C2_TE_MASK;            // Enable UART2 transmitter
  UART2_C2 |= UART_C2_RE_MASK;            // Enable UART2 receiver

  FIFO_Init(&TxFIFO);                     // Initialise transmitting FIFO buffer
  FIFO_Init(&RxFIFO);                     // Initialise receiving FIFO buffer


  UART2_C2 |= UART_C2_RIE_MASK;           // Receive Interrupt Enable

  // Interrupt enabling assumes the receive and transmit FIFOs have been initialised.
  // IRQ = 49
  // NVIC non-IPR=1, IPR=12
  // Left shift == IRQ % 32 (See quick ref guide, page 36-37)
  NVICICPR1 = (1 << 17);                  // Clear any pending interrupts on UART2
  NVICISER1 = (1 << 17);                  // Enable interrupts from UART2 module

  // Create semaphores for Rx and Tx threads
  RxReady         = OS_SemaphoreCreate(0);
  TxReady         = OS_SemaphoreCreate(0);

  OS_ERROR error[2];

  error[0] = OS_ThreadCreate(UART_RxThread,
                          NULL,
                          &UART_RxThreadStack[THREAD_STACK_SIZE - 1],
                          UART_RX_PRIORITY);

  error[1] = OS_ThreadCreate(UART_TxThread,
                          NULL,
                          &UART_TxThreadStack[THREAD_STACK_SIZE - 1],
                          UART_TX_PRIORITY);

  if (  (error[0] == OS_NO_ERROR) &&
        (error[1] == OS_NO_ERROR))
  {
    return true;
  }
  else
  {
    Cpu_ivINT_Hard_Fault();
  }
}


void UART_InChar(uint8_t * const dataPtr)
{
  FIFO_Get(&RxFIFO, dataPtr);    // Move byte from RxFIFO to Packet
}

void UART_OutChar(const uint8_t data)
{
  FIFO_Put(&TxFIFO, data);       // Move byte from Packet to TxFIFO
  UART2_C2 |= UART_C2_TIE_MASK;  // Enable transmit interrupts
}


void __attribute__ ((interrupt)) UART_ISR(void)
{
  OS_ISREnter();

  // Checks if the RDRF flag is set and receiving interrupts are enabled
  if ((UART2_S1 & UART_S1_RDRF_MASK) && (UART2_C2 & UART_C2_RIE_MASK))
  {
    RxByte = UART2_D;               // Read received byte into private global variable for use in RxThread, also clears RDRF flag
    OS_SemaphoreSignal(RxReady);    // Signal the UART_RxThread to go
  }

  // Checks if the TDRE flag is set and transmitting interrupts are enabled
  else if ((UART2_S1 & UART_S1_TDRE_MASK) && (UART2_C2 & UART_C2_TIE_MASK))
  {
    UART2_C2 &= ~UART_C2_TIE_MASK;  // Disable transmit interrupts so we can write to the data register in TxThread
    OS_SemaphoreSignal(TxReady);    // Signal the UART_TxThread to go
  }

  OS_ISRExit();
}


// THREADS

void UART_TxThread(void* pData)
{
  for (;;)
  {
    OS_SemaphoreWait(TxReady, 0);

    if (UART2_S1 & UART_S1_TDRE_MASK)         // Read S1[TDRE] flag, this is necessary to clear it
    {
      FIFO_Get(&TxFIFO, (uint8_t*)&UART2_D);  // Move byte from TxFIFO to UART2_D, this will clear the S1[TDRE] flag
    }

    UART2_C2 |= UART_C2_TIE_MASK;             // Enable transmit interrupts
  }
}


void UART_RxThread(void* pData)
{
  for (;;)
  {
    OS_SemaphoreWait(RxReady, 0);
    FIFO_Put(&RxFIFO, RxByte);
    OS_SemaphoreSignal(ByteReceived);
  }
}


/*!
** @}
*/
