/*! @file
 *
 *  @brief Routines for controlling Periodic Interrupt Timer (PIT) on the TWR-K70F120M.
 *
 *  This contains the functions for operating the periodic interrupt timer (PIT).
 *
 *  @author 12551382 Samin Saif and 11850637 Alex Hiller
 *  @date 2018-04-28
 */
/*!
**  @addtogroup PIT_module PIT module documentation
**  @{
*/
/* MODULE PIT */

#include "brOS.h"
#include "PIT.h"
#include "MK70F12.h" // Included only to resolve error, seems stupid to do so though.

// Macros
#define MODULE_CLK_PERIOD_NANOSECONDS 47                                                              // For macro function integer calculation. Too big for uint32_t
#define LDVAL_CALC(DESIRED_PERIOD)  (uint32_t)((DESIRED_PERIOD)/(MODULE_CLK_PERIOD_NANOSECONDS) - 1)  // Macro function for PIT load value calculation

// Global variables
static uint32_t NanoSecondPerClockTick;

// Semaphores
static OS_ECB* CallCallback;

static void (*UserCallback)(void* args);
static void *UserArguments;


// PUBLIC FUNCTIONS

bool PIT_Init(const uint32_t moduleClk, void (*userFunction)(void*), void* userArguments)
{
  // Assign usercallback function to private global variables
  UserCallback  = userFunction;
  UserArguments = userArguments;

  // Initialize semaphore
  CallCallback = OS_SemaphoreCreate(0);

  // Following example in K70 Ref Manual pg 1340

  SIM_SCGC6 |= SIM_SCGC6_PIT_MASK;           // Enable the clock to the PIT

  // Enable the PIT, making MDIS 0 so that PIT can be used (this must be done first)
  // Also enable the FRZ register to freeze PIT timers during debug mode.
  PIT_MCR = ~PIT_MCR_MDIS_MASK;

  PIT_TCTRL0 |= PIT_TCTRL_TIE_MASK;          // Set arm bit (enable timer 0 interrupts by writing 1 to TIE [timer interrupt enable] register)
  NanoSecondPerClockTick = 1e9 / moduleClk;  // Set timer0 up and make PIT restart

  // Set NVIC bits
  // IRQ = 68
  // NVIC non-IPR=2, IPR=17
  // Clear any pending interrupts on PIT timer0
  // Left shift == IRQ % 32 == 68 % 32 == 4
  NVICICPR2 = (1 << 4);
  NVICISER2 = (1 << 4);

  return true;
}


void PIT_Set(const uint32_t period, const bool restart)
{
  if (restart)
  {
    PIT_Enable(false);                // Stop the PIT
    PIT_LDVAL0 = LDVAL_CALC(period);  // New value
    PIT_Enable(true);                 // Restart the PIT
  }
  else
  {
    // Loading the value lets the current timer finish,
    // after the PIT interrupt, a new timer with the new value will begin
    PIT_LDVAL0 = LDVAL_CALC(period);
  }
}


void PIT_Enable(const bool enable)
{
  // Act on MDIS (Module Disable) 1 - Stop all timers, 0 - Enable all timers
  if (enable)
    PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK;   // Start timer 0 by writing 1 to TEN (timer enable)
  else
    PIT_TCTRL0 &= ~PIT_TCTRL_TEN_MASK;  // Stop timer 0 by writing 1 to TEN (timer enable)
}


void __attribute__ ((interrupt)) PIT_ISR(void)
{
  OS_ISREnter();

  PIT_TFLG0 |= PIT_TFLG_TIF_MASK;    // Clear flag by w1c
  OS_SemaphoreSignal(CallCallback);  // Signal

  OS_ISRExit();
}


// THREADS

void PIT_Thread(void* pData)
{
  for (;;)
  {
    OS_SemaphoreWait(CallCallback, 0);
    if (UserCallback)
    {
      (*UserCallback)(UserArguments);
    }
  }
}


/*!
** @}
*/
