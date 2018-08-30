/*! @file
 *
 *  @brief Routines to implement a FIFO buffer.
 *
 *  This contains the functions for manipulating the FIFO.
 *
 *  @author 11850637 Alex Hiller and 12551382 Samin Saif
 *  @date 2018-03-23
 */
/*!
**  @addtogroup FIFO_module FIFO module documentation
**  @{
*/

#include "FIFO.h"
#include "Cpu.h"  // Needed for critical sections/atomic-code

const int FIRST_INDEX = 0;
const int LAST_INDEX = FIFO_SIZE-1;

/*! @brief Initialize the FIFO before first use.
 *
 *  @param FIFO A pointer to the FIFO that needs initializing.
 *  @return void
 */
void FIFO_Init(TFIFO * const FIFO)
{
  EnterCritical();  // Beginning of critical/atomic code

  FIFO->End = FIRST_INDEX;
  FIFO->Start = FIRST_INDEX;
  FIFO->NbBytes = 0;

  ExitCritical();  // Completion of critical/atomic code
}

/*! @brief Put one character into the FIFO.
 *
 *  @param FIFO A pointer to a FIFO struct where data is to be stored.
 *  @param data A byte of data to store in the FIFO buffer.
 *  @return bool - TRUE if data is successfully stored in the FIFO.
 *  @note Assumes that FIFO_Init has been called.
 */
bool FIFO_Put(TFIFO * const FIFO, const uint8_t data)
{
  EnterCritical();  // Beginning of critical/atomic code

  // FIFO full? If so, return false
  if ( (FIFO->NbBytes) >= FIFO_SIZE )
  {
    ExitCritical();  // Completion of critical/atomic code
    return false;
  }

  // Place byte into FIFO
  FIFO->Buffer[FIFO->End] = data;

  // Increment FIFO->End to be in next position
  if ( (FIFO->End) < (LAST_INDEX) )
  {
    FIFO->End++;
  }

  else
  {
    FIFO->End = 0;
  }

  // Increase number of bytes
  FIFO->NbBytes++;

  ExitCritical();  // Completion of critical/atomic code

  return true;
}

/*! @brief Get one character from the FIFO.
 *
 *  @param FIFO A pointer to a FIFO struct with data to be retrieved.
 *  @param dataPtr A pointer to a memory location to place the retrieved byte.
 *  @return bool - TRUE if data is successfully retrieved from the FIFO.
 *  @note Assumes that FIFO_Init has been called.
 */
bool FIFO_Get(TFIFO * const FIFO, uint8_t * const dataPtr)
{
  EnterCritical();  // Beginning of critical/atomic code

  // Check if FIFO is empty, if yes, nothing to fetch, return error.
  if ( (FIFO->NbBytes) == 0 )
  {
    ExitCritical();  // Completion of critical/atomic code
    return false;
  }

  // There's space, place data at address
  *(dataPtr) = FIFO->Buffer[FIFO->Start];

  // Increment Start index to be in next position, as long as <
  if ( (FIFO->Start) < (LAST_INDEX) )
  {
    FIFO->Start++;
  }

  else
  {
    FIFO->Start = 0;
  }

  // Decrement the count of the number of bytes
  FIFO->NbBytes--;

  ExitCritical();  // Completion of critical/atomic code

  return true;
}

/*!
** @}
*/
