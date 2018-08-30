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

// Macros
#define MODULE_CLK_PERIOD_NANOSECONDS 47                                                              // For macro function integer calculation. Too big for uint32_t
#define LDVAL_CALC(DESIRED_PERIOD)  (uint32_t)((DESIRED_PERIOD)/(MODULE_CLK_PERIOD_NANOSECONDS) - 1)  // Macro function for PIT load value calculation

// PUBLIC FUNCTIONS

bool PIT_Init(const uint32_t moduleClk)
{
  // Following example in K70 Ref Manual pg 1340
  SIM_SCGC6 |= SIM_SCGC6_PIT_MASK;  // Enable the clock to the PIT (all channels)

  PIT_MCR = ~PIT_MCR_MDIS_MASK;     // Enable the PIT, making MDIS 0 so that PIT can be used (this must be done first)
  PIT_MCR |= PIT_MCR_FRZ_MASK;      // Also enable the FRZ register to freeze PIT timers during debug mode.

  PIT_TCTRL0 |= PIT_TCTRL_TIE_MASK; // Set arm bit (enable timer 0 interrupts by writing 1 to TIE [timer interrupt enable] register)
  PIT_TCTRL1 |= PIT_TCTRL_TIE_MASK;

  // FIXME: Re-enable interrupts for PIT channel 3 and 4 later, if necessary.
//  PIT_TCTRL2 |= PIT_TCTRL_TIE_MASK;
//  PIT_TCTRL3 |= PIT_TCTRL_TIE_MASK;

  // PIT Channel 0 Interrupts
  // Set NVIC bits
  // IRQ = 68
  // NVIC non-IPR=2, IPR=17
  // Clear any pending interrupts on PIT timer0
  // Left shift == IRQ % 32 == 68 % 32 == 4
  NVICICPR2 = (1 << 4);
  NVICISER2 = (1 << 4);

  // PIT Channel 1 Interrupts
  // Set NVIC bits
  // IRQ = 69
  // NVIC non-IPR=2, IPR=17
  // Clear any pending interrupts on PIT timer0
  // Left shift == IRQ % 32 == 69 % 32 == 5
  NVICICPR2 = (1 << 5);
  NVICISER2 = (1 << 5);


  return true;
}

void PIT_Set(const uint32_t period, const bool restart, uint8_t channel)
{

  switch(channel)
  {
    // Channel 0
    case (0):
      if (restart)
      {
        PIT_Enable(false, 0);                // Stop the PIT
        PIT_LDVAL0 = LDVAL_CALC(period);  // New value
        PIT_Enable(true, 0);                 // Restart the PIT
      }
      else
      {
        // Loading the value lets the current timer finish,
        // after the PIT interrupt, a new timer with the new value will begin
        PIT_LDVAL0 = LDVAL_CALC(period);
      }
      break;

    // Channel 1
    case (1):
      if (restart)
      {
        PIT_Enable(false, 1);                // Stop the PIT
        PIT_LDVAL1 = LDVAL_CALC(period);  // New value
        PIT_Enable(true, 1);                 // Restart the PIT
      }
      else
      {
        // Loading the value lets the current timer finish,
        // after the PIT interrupt, a new timer with the new value will begin
        PIT_LDVAL1 = LDVAL_CALC(period);
      }
      break;

    // Channel 2
    case (2):
      if (restart)
      {
        PIT_Enable(false, 2);                // Stop the PIT
        PIT_LDVAL2 = LDVAL_CALC(period);  // New value
        PIT_Enable(true, 2);                 // Restart the PIT
      }
      else
      {
        // Loading the value lets the current timer finish,
        // after the PIT interrupt, a new timer with the new value will begin
        PIT_LDVAL2 = LDVAL_CALC(period);
      }
      break;

    // Channel 3
    case (3):
      if (restart)
      {
        PIT_Enable(false, 3);                // Stop the PIT
        PIT_LDVAL3 = LDVAL_CALC(period);  // New value
        PIT_Enable(true, 3);                 // Restart the PIT
      }
      else
      {
        // Loading the value lets the current timer finish,
        // after the PIT interrupt, a new timer with the new value will begin
        PIT_LDVAL3 = LDVAL_CALC(period);
      }
      break;

    default:
      break;
  }
}

void PIT_Enable(const bool enable, uint8_t channel)
{
  // Act on MDIS (Module Disable) 1 - Stop all timers, 0 - Enable all timers

  switch(channel)
  {
    // Channel 0
    case (0):
      if (enable)
        PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK;   // Start timer 0 by writing 1 to TEN (timer enable)
      else
        PIT_TCTRL0 &= ~PIT_TCTRL_TEN_MASK;  // Stop timer 0 by writing 1 to TEN (timer enable)
      break;

    // Channel 1
    case (1):
      if (enable)
        PIT_TCTRL1 |= PIT_TCTRL_TEN_MASK;   // Start timer 0 by writing 1 to TEN (timer enable)
      else
        PIT_TCTRL1 &= ~PIT_TCTRL_TEN_MASK;  // Stop timer 0 by writing 1 to TEN (timer enable)
      break;

    // Channel 2
    case (2):
      if (enable)
        PIT_TCTRL2 |= PIT_TCTRL_TEN_MASK;   // Start timer 0 by writing 1 to TEN (timer enable)
      else
        PIT_TCTRL2 &= ~PIT_TCTRL_TEN_MASK;  // Stop timer 0 by writing 1 to TEN (timer enable)
      break;

    // Channel 3
    case (3):
      if (enable)
        PIT_TCTRL3 |= PIT_TCTRL_TEN_MASK;   // Start timer 0 by writing 1 to TEN (timer enable)
      else
        PIT_TCTRL3 &= ~PIT_TCTRL_TEN_MASK;  // Stop timer 0 by writing 1 to TEN (timer enable)
      break;
  }
}

// Sampling PIT
void __attribute__ ((interrupt)) PIT0_ISR(void)
{
  OS_ISREnter();

  PIT_TFLG0 |= PIT_TFLG_TIF_MASK;    // Clear flag by w1c

  // TODO: Remove the three separate counters, they'll always be sampled at the same time, hence the counters will always be equal.
  Analog_Get(CHANNEL_A, &NewestPhaseA);
  PhaseA.Counter++;
  Analog_Get(CHANNEL_B, &NewestPhaseB);
  PhaseB.Counter++;
  Analog_Get(CHANNEL_C, &NewestPhaseC);
  PhaseC.Counter++;

  if (PhaseA.Counter >= 16 || PhaseB.Counter >= 16 || PhaseC.Counter >= 16  )
  {
    if (PhaseA.Counter >= NB_SAMPLES)
    {
      PhaseA.Counter -= 16;
    }
    if (PhaseB.Counter >= NB_SAMPLES)
    {
      PhaseB.Counter -= 16;
    }
    if (PhaseC.Counter >= NB_SAMPLES)
    {
      PhaseC.Counter -= 16;
    }
    OS_SemaphoreSignal(SampleComplete);
  }
  OS_ISRExit();
}

// StopWatch Timer
void __attribute__ ((interrupt)) PIT1_ISR(void)
{
  OS_ISREnter();

  PIT_TFLG1 |= PIT_TFLG_TIF_MASK;    // Clear flag by w1c

  // Only increment the stopwatch if we want to be timing for an alarm
  if (AlarmStopWatch.SetLevel != NOT_SET)
  {
    AlarmStopWatch.StopWatch++;

    // If we've reached the stop-watch limit (with wiggle-room), output the alarm.
    if ((AlarmStopWatch.StopWatch >= AlarmStopWatch.StopWatchStop) && (AlarmStopWatch.StopWatch < AlarmStopWatch.StopWatchStop + 10))
    {
      switch (AlarmStopWatch.SetLevel)
      {
        case (SET_HIGH):
          Logic_OutputLower(ON);
          AlarmStopWatch.StopWatch = 0;
          break;
        case (SET_LOW):
          Logic_OutputRaise(ON);
          AlarmStopWatch.StopWatch = 0;
          break;
      }
    }
  }

  OS_ISRExit();
}

/*!
** @}
*/
