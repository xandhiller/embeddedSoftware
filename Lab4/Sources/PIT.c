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

#include "PIT.h"
#include "MK70F12.h" // Included only to resolve error, seems stupid to do so though.

// Global variables
static void (*UserFunction)(void*);
static void* UserArguments;
static uint32_t NanoSecondPerClockTick;

// Constants

#define MODULE_CLK_PERIOD_NANOSECONDS 47 	// For macro function integer calculation. Too big for uint32_t.

// Macro function for PIT load value calculation
#define LDVAL_CALC(DESIRED_PERIOD)	(uint32_t)((DESIRED_PERIOD)/(MODULE_CLK_PERIOD_NANOSECONDS) - 1)

/*! @brief Sets up the PIT before first use.
 *
 *  Enables the PIT and freezes the timer when debugging.
 *  @param moduleClk The module clock rate in Hz.
 *  @param userFunction is a pointer to a user callback function.
 *  @param userArguments is a pointer to the user arguments to use with the user callback function.
 *  @return bool - TRUE if the PIT was successfully initialized.
 *  @note Assumes that moduleClk has a period which can be expressed as an integral number of nanoseconds.
 */
bool PIT_Init(const uint32_t moduleClk, void (*userFunction)(void*), void* userArguments)
{
  // Following example in K70 Ref Manual pg 1340

  // Enable the clock to the PIT
  SIM_SCGC6 |= SIM_SCGC6_PIT_MASK;

  // Set up callback functions for later use during PIT_ISR
  UserFunction = userFunction;
  UserArguments = userArguments;

  // Enable the PIT, making MDIS 0 so that PIT can be used (this must be done first)
  // Also enable the FRZ register to freeze PIT timers during debug mode.
  PIT_MCR = ~PIT_MCR_MDIS_MASK;

  // Set arm bit (enable timer 0 interrupts by writing 1 to TIE [timer interrupt enable] register)
  PIT_TCTRL0 |= PIT_TCTRL_TIE_MASK;

  // Set timer0 up and make PIT restart

  NanoSecondPerClockTick = 1e9 / moduleClk;

  // Set NVIC bits
  // IRQ = 68
  // NVIC non-IPR=2, IPR=17
  // Clear any pending interrupts on PIT timer0
  // Left shift == IRQ % 32 == 68 % 32 == 4
  NVICICPR2 = (1 << 4);
  // Enable interrupts from PIT timer0 module
  NVICISER2 = (1 << 4);

  return true;
}

/*! @brief Sets the value of the desired period of the PIT.
 *
 *  @param period The desired value of the timer period in nanoseconds.
 *  @param restart TRUE if the PIT is disabled, a new value set, and then enabled.
 *                 FALSE if the PIT will use the new value after a trigger event.
 *  @note The function will enable the timer and interrupts for the PIT.
 */
void PIT_Set(const uint32_t period, const bool restart)
{
  if (restart)
  {
    // Stop the PIT
    PIT_Enable(false);

    // New value
    PIT_LDVAL0 = LDVAL_CALC(period);

    // Restart the PIT
    PIT_Enable(true);
  }
  else
  {
      // Loading the value lets the current timer finish,
      // after the PIT interrupt, a new timer with the new value will begin
      PIT_LDVAL0 = LDVAL_CALC(period);
  }
}

/*! @brief Enables or disables the PIT.
 *
 *  @param enable - TRUE if the PIT is to be enabled, FALSE if the PIT is to be disabled.
 */
void PIT_Enable(const bool enable)
{
  // Act on MDIS (Module Disable) 1 - Stop all timers, 0 - Enable all timers
  if (enable)
    {
      // Start timer 0 by writing 1 to TEN (timer enable)
      PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK;
    }
  else
    {
      // Stop timer 0 by writing 1 to TEN (timer enable)
      PIT_TCTRL0 &= ~PIT_TCTRL_TEN_MASK;
    }
}

/*! @brief Interrupt service routine for the PIT.
 *
 *  The periodic interrupt timer has timed out.
 *  The user callback function will be called.
 *  @note Assumes the PIT has been initialized.
 */
void __attribute__ ((interrupt)) PIT_ISR(void)
{
  // Clear flag by w1c
  PIT_TFLG0 |= PIT_TFLG_TIF_MASK;

  if (UserFunction)
    {
      (*UserFunction)(UserArguments);
    }
}

/*!
** @}
*/
