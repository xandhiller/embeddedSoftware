/*! @file
 *
 *  @brief Routines for controlling the Real Time Clock (RTC) on the TWR-K70F120M.
 *
 *  This contains the functions for operating the real time clock (RTC).
 *
 *  @author 11251382 Samin Saif and 11850637 Alex Hiller
 *  @date 2018-04-28
 */
/*!
**  @addtogroup RTC_module RTC module documentation
**  @{
*/
/* MODULE RTC */

// Header files
#include "Cpu.h"
#include "brOS.h"
#include "RTC.h"

// Externs
extern OS_ECB* OneSecond;


// PUBLIC FUNCTIONS

bool RTC_Init(void)
{
  SIM_SCGC6 |= SIM_SCGC6_RTC_MASK;     // Enable clock gate

  RTC_CR |= RTC_CR_SC2P_MASK;          // Enable 2pF capacitor
  RTC_CR |= RTC_CR_SC16P_MASK;         // Enable 16pF capacitor

  RTC_IER |= RTC_IER_TSIE_MASK;        // Enable Time Seconds Interrupt
  RTC_SR |= RTC_SR_TCE_MASK;           // Enable Time Counter (this will make the TSR and TPR registers increment but non-writable)

  RTC_CR |= RTC_CR_OSCE_MASK;          // Enable Oscillator
  for (int i = 0; i < 0x600000; i++);  // Wait for oscillators to stabilize

  if (RTC_SR & RTC_SR_TIF_MASK)
  {
    RTC_Set(0,0,0);                    // If SR[TIF] is set, then reset timer, this will also clear the SR[TIF] flag
  }

  RTC_LR &= ~RTC_LR_CRL_MASK;          // Lock the control register

  NVICICPR2 = (1 << 3);                // Clear pending interrupts on the RTC timer
  NVICISER2 = (1 << 3);                // Enable RTC Interrupt Service Routine

  return true;
}


void RTC_Set(const uint8_t hours, const uint8_t minutes, const uint8_t seconds)
{

  RTC_SR &= ~RTC_SR_TCE_MASK;                                                 // Disable Time Counter
  RTC_TSR = (uint32_t)seconds + (uint32_t)minutes*60 + (uint32_t)hours*3600;  // Assign total time in seconds to Time Seconds Register
  RTC_SR |= RTC_SR_TCE_MASK;                                                  // Enable Time Counter
}


void RTC_Get(uint8_t* const hours, uint8_t* const minutes, uint8_t* const seconds)
{
  // Reset time to 0 when it goes past 23h 59m 59s
  uint32_t totalTimeSeconds = RTC_TSR % 86400;

  // Time-seconds register can give an inaccurate read, so read twice and make sure they're the same value
  do
  {
    totalTimeSeconds = RTC_TSR;
  } while(totalTimeSeconds != RTC_TSR);

  *hours = (totalTimeSeconds/3600);
  *minutes = (totalTimeSeconds % 3600) / 60;
  *seconds = totalTimeSeconds - (*hours)*3600 - (*minutes)*60;
}


void __attribute__ ((interrupt)) RTC_ISR(void)
{
  OS_ISREnter();
  OS_SemaphoreSignal(OneSecond);
  OS_ISRExit();
}


/*!
** @}
*/
