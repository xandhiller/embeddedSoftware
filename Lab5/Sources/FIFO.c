/*! @file
 *
 *  @brief Routines to implement a FIFO buffer.
 *
 *  This contains the structure and "methods" for accessing a byte-wide FIFO.
 *
 *  @author 12551382 Samin Saif and 11850637 Alex Hiller
 *  @date 2018-05-29
 */
/*!
**  @addtogroup FIFO_module FIFO module documentation
**  @{
*/
/* MODULE FIFO */

#include "types.h"
#include "FIFO.h"


// PUBLIC FUNCTIONS

void FIFO_Init(TFIFO * const FIFO)
{
  FIFO->End = 0;
  FIFO->Start = 0;
  FIFO->SpaceUsed = OS_SemaphoreCreate(0);
  FIFO->SpaceAvailable = OS_SemaphoreCreate(FIFO_SIZE);
}


void FIFO_Put(TFIFO * const FIFO, const uint8_t data)
{
  OS_DisableInterrupts();

  OS_SemaphoreWait(FIFO->SpaceAvailable, 0);  // One less space available in FIFO
  FIFO->Buffer[FIFO->End] = data;             // Place byte into FIFO
  OS_SemaphoreSignal(FIFO->SpaceUsed);        // One more space taken in FIFO

  // Increment FIFO->End to be in next position
  if (FIFO->End < LAST_INDEX)
    FIFO->End++;
  else
    FIFO->End = 0;

  OS_EnableInterrupts();
}


void FIFO_Get(TFIFO * const FIFO, uint8_t * const dataPtr)
{
  OS_DisableInterrupts();

  OS_SemaphoreWait(FIFO->SpaceUsed, 0);      // One less space used in FIFO
  *(dataPtr) = FIFO->Buffer[FIFO->Start];    // There's space, place data at address
  OS_SemaphoreSignal(FIFO->SpaceAvailable);  // One more space available in FIFO

  // Increment Start index to be in next position, as long as < FULL
  if (FIFO->Start < LAST_INDEX)
    FIFO->Start++;
  else
    FIFO->Start = 0;

  OS_EnableInterrupts();
}


/*!
** @}
*/
