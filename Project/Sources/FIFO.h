/*! @file
 *
 *  @brief Routines to implement a FIFO buffer.
 *
 *  This contains the structure and "methods" for accessing a byte-wide FIFO.
 *
 *  @author PMcL
 *  @date 2015-07-23
 */


#ifndef FIFO_H
#define FIFO_H

// new types
#include "brOS.h"

// Number of bytes in a FIFO
#define FIFO_SIZE 	256
#define LAST_INDEX 	FIFO_SIZE-1

/*!
 * @struct TFIFO
 */
typedef struct
{
  uint16_t Start;		/*!< The index of the position of the oldest data in the FIFO */
  uint16_t End; 		/*!< The index of the next available empty position in the FIFO */
  uint8_t Buffer[FIFO_SIZE];	/*!< The actual array of bytes to store the data */
  OS_ECB* SpaceUsed;
  OS_ECB* SpaceAvailable;
} TFIFO;

/*! @brief Initialize the FIFO before first use.
 *
 *  @param FIFO A pointer to the FIFO that needs initializing.
 *  @return void
 */
void FIFO_Init(TFIFO* const FIFO);

/*! @brief Put one character into the FIFO.
 *
 *  @param FIFO A pointer to a FIFO struct where data is to be stored.
 *  @param data A byte of data to store in the FIFO buffer.
 *  @note Assumes that FIFO_Init has been called.
 */
void FIFO_Put(TFIFO* const FIFO, const uint8_t data);

/*! @brief Get one character from the FIFO.
 *
 *  @param FIFO A pointer to a FIFO struct with data to be retrieved.
 *  @param dataPtr A pointer to a memory location to place the retrieved byte.
 *  @note Assumes that FIFO_Init has been called.
 */
void FIFO_Get(TFIFO* const FIFO, uint8_t* const dataPtr);


#endif /* SOURCES_FIFO_H_ */
