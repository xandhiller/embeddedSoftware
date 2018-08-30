/*
 * FIFO.c
 *
 *  Created on: 23 Mar 2018
 *      Author: 11850637 Alex Hiller and 12551382 Samin Saif
 */

#include "types.h"
#include "FIFO.h"

#define FIRST_INDEX 0
#define LAST_INDEX FIFO_SIZE-1
#define FIFO_FULL FIFO_SIZE+1000
// Fifo FULL always points End to an index bigger than the array
// Can't do NULL, as it evaluates to zero. Giving the array the
// appearance of being ready to enter on the zeroth element.
// Just have to be careful with the syntax until we think of a
// better solution.

void nbBytesCalc(TFIFO * const FIFO);

void FIFO_Init(TFIFO * const FIFO)
{
    FIFO->End = 0;
    FIFO->Start = 0;
    FIFO->NbBytes = 0;
}

bool FIFO_Put(TFIFO * const FIFO, const uint8_t data)
{
    // First check if the FIFO buffer is full
    // If it is full, stop! return an error!
    
    // If not, CONTINUE! 
    
    // Now that we know there is space, let us put the byte into the fifo.
    
    // END should be the index location of where this incomming byte should go.
    
    // Now that we have used the index location END, we need to increment END to the next position.
    
    // What if END is now beyond the maximum index position (255)?
    // If so, set END to zero.
    
    
    // Also don't forget to increment the count of the Number of Bytes in the FIFO buffer.
    
    // Finally return true
    
    
}

bool FIFO_Put(TFIFO * const FIFO, const uint8_t data)
{
    // FIFO empty
    if( (FIFO->Start) == (FIFO->End) )
    {
        // Save data to FIFO
        FIFO->Buffer[FIFO->End] = data;
        // Increment End
        FIFO->End += 1;
        // End++ won't work, as it increments it by the size of the
        // pointer, FIFO.
        nbBytesCalc(FIFO);
        return true;
    }

    // FIFO full
    if( (FIFO->End) == (FIFO_FULL) )
    {
        nbBytesCalc(FIFO);
        return false;
    }

    // FIFO about to be FULL

        // Start is at zero
    if( (FIFO->End) == (LAST_INDEX) )
    {
        FIFO->Buffer[FIFO->End] = data;
        FIFO->End = FIFO_FULL;
        nbBytesCalc(FIFO);
        return true;
    }
        // End in front of start
    // Other than Start = 0 and End = 255, this won't happen.
    // Handled with 'Start is at zero'

        // Start in front of end
    if( ((FIFO->Start) == (FIFO->End) + 1) )
    {
        FIFO->Buffer[FIFO->End] = data;
        FIFO->End = FIFO_FULL;
        nbBytesCalc(FIFO);
        return true;
    }

    // FIFO not empty and not FULL

        // End != LAST_INDEX
    if(( (FIFO->End) != (LAST_INDEX) ) && ( (FIFO->Start) != ( (FIFO->End) + 1) ))
    {
        FIFO->Buffer[FIFO->End] = data;
        FIFO->End += 1;
        nbBytesCalc(FIFO);
        return true;
    }

        // End == LAST_INDEX
    if (( (FIFO->End) == (LAST_INDEX) ) && ( (FIFO->Start) != (FIRST_INDEX) ) )
    {
        FIFO->Buffer[FIFO->End] = data;
        FIFO->End = FIRST_INDEX;
        nbBytesCalc(FIFO);
        return true;
    }
}

bool FIFO_Get(TFIFO * const FIFO, uint8_t * const dataPtr)
{
    // This function is about pulling stuff out of the buffer, so it will
    // be focused on moving FIFO->Start around.

    // FIFO is empty
    if( (FIFO->Start) == (FIFO->End) )
    {
        nbBytesCalc(FIFO);
        return false;
    }

    // FIFO IS FULL -- important to place FIFO->End in the appropriate place
        // Start is not at the end
    if( (FIFO->Start) != (LAST_INDEX) && ((FIFO->End) == (FIFO_FULL)) )
    {
        // Copy oldest data to address
        *(dataPtr) = FIFO->Buffer[FIFO->Start];
        // Move FIFO->End to FIFO->Start position
        FIFO->End = FIFO->Start;
        // Oldest data is now one index ahead
        FIFO->Start += 1;
        // Success
        nbBytesCalc(FIFO);
        return true;
    }
        // Start is at the end
    if( ((FIFO->Start) == (LAST_INDEX)) && ((FIFO->End) == (FIFO_FULL)) )
    {
        // Copy oldest data to address
        *(dataPtr) = FIFO->Buffer[FIFO->Start];
        // Move FIFO->End to FIFO->Start position
        FIFO->End = FIFO->Start;
        // Move counter to start of Buffer
        FIFO->Start = FIRST_INDEX;
        // Success
        nbBytesCalc(FIFO);
        return true;
    }


    // FIFO NOT EMPTY, NOT FULL
        // Start is at the end
    if ( ((FIFO->Start) == (LAST_INDEX)) && ((FIFO->End) != (FIFO_FULL)) )
    {
        // Copy oldest data to address
        *(dataPtr) = FIFO->Buffer[FIFO->Start];
        // Move counter to start of Buffer
        FIFO->Start = FIRST_INDEX;
        // Success
        nbBytesCalc(FIFO);
        return true;
    }
        // Start is not at the end
    if ( ((FIFO->Start) != (LAST_INDEX)) && ((FIFO->End) != (FIFO_FULL)) )
    {
        // Copy oldest data to address
        *(dataPtr) = FIFO->Buffer[FIFO->Start];
        // Move counter up to next index, as that is now oldest data
        FIFO->Start += 1;
        // Success
        nbBytesCalc(FIFO);
        return true;
    }

    // Code should not reach here
    nbBytesCalc(FIFO);
    return false;
}

// nb: returns number of bytes, not bits.
void nbBytesCalc(TFIFO * const FIFO)
{
    // FIFO empty
    if( (FIFO->End) == (FIFO->Start) )
    {
        FIFO->NbBytes = 0;
        return;
    }

    // FIFO full
    if( (FIFO->End) == (FIFO_FULL) )
    {
        FIFO->NbBytes = FIFO_SIZE;
        return;
    }

    // Start < End
    if( (FIFO->Start) < (FIFO->End) )
    {
        FIFO->NbBytes = (( (FIFO->End) - (FIFO->Start) ) + 1);
        return;
    }

    // End > Start
    if( (FIFO->End) < (FIFO->Start) )
    {
        FIFO->NbBytes = ( (FIFO_SIZE - ((FIFO->Start) + 1)) + (((FIFO->End)+1)));
        return;
    }

}
