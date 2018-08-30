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

#include "RTC.h"
#include "Cpu.h"

static void (*UserFunction)(void*);
static void *UserArguments;

/*! @brief Initializes the RTC before first use.
 *
 *  Sets up the control register for the RTC and locks it.
 *  Enables the RTC and sets an interrupt every second.
 *  @param userFunction is a pointer to a user callback function.
 *  @param userArguments is a pointer to the user arguments to use with the user callback function.
 *  @return bool - TRUE if the RTC was successfully initialized.
 */
bool RTC_Init(void (*userFunction)(void*), void* userArguments)
{
  EnterCritical();  // Beginning of critical/atomic code

  // Assign callback function and its arguments to private global variables
  UserFunction = userFunction;
  UserArguments = userArguments;

  // Enable clock gate
  SIM_SCGC6 |= SIM_SCGC6_RTC_MASK;

  // Enable necessary capacitors for the required load capacitance (18pF)
  RTC_CR |= RTC_CR_SC2P_MASK;   //2pF
  RTC_CR |= RTC_CR_SC16P_MASK;  //16pF

  // Check Time Invalid Flag and clear if set
  if (RTC_SR & RTC_SR_TIF_MASK)
  {
    RTC_Set(0, 0, 0);
  }

  // Enable Time Seconds Interrupt
  RTC_IER |= RTC_IER_TSIE_MASK;

  // Enable Time Counter (this will make the TSR and TPR registers increment but non-writable)
  RTC_SR |= RTC_SR_TCE_MASK;

  // Enable Oscillator
  RTC_CR |= RTC_CR_OSCE_MASK;

  NVICICPR2 = (1 << 3);  // Clear pending interrupts on the RTC timer
  NVICISER2 = (1 << 3);  // Enable RTC Interrupt Service Routine

  ExitCritical();  // Completion of critical/atomic code

  return true;
}

/*! @brief Sets the value of the real time clock.
 *
 *  @param hours The desired value of the real time clock hours (0-23).
 *  @param minutes The desired value of the real time clock minutes (0-59).
 *  @param seconds The desired value of the real time clock seconds (0-59).
 *  @note Assumes that the RTC module has been initialized and all input parameters are in range.
 */
void RTC_Set(const uint8_t hours, const uint8_t minutes, const uint8_t seconds)
{
  // Disable Time Counter
  RTC_SR &= ~RTC_SR_TCE_MASK;

  // Assign total time in seconds to Time Seconds Register
  RTC_TSR = (uint32_t)seconds + (uint32_t)minutes*60 + (uint32_t)hours*3600;

  // Enable Time Counter
  RTC_SR |= RTC_SR_TCE_MASK;
}

/*! @brief Gets the value of the real time clock.
 *
 *  @param hours The address of a variable to store the real time clock hours.
 *  @param minutes The address of a variable to store the real time clock minutes.
 *  @param seconds The address of a variable to store the real time clock seconds.
 *  @note Assumes that the RTC module has been initialized.
 */
void RTC_Get(uint8_t* const hours, uint8_t* const minutes, uint8_t* const seconds)
{
  // Reset time to 0 when it goes past 23h 59m 59s
  uint32_t totalTimeSeconds = RTC_TSR % 86400;

  *hours = (totalTimeSeconds/3600);
  *minutes = (totalTimeSeconds % 3600) / 60;
  *seconds = totalTimeSeconds - (*hours)*3600 - (*minutes)*60;
}

/*! @brief Interrupt service routine for the RTC.
 *
 *  The RTC has incremented one second.
 *  The user callback function will be called.
 *  @note Assumes the RTC has been initialized.
 */
void __attribute__ ((interrupt)) RTC_ISR(void)
{
  // No flags to clear

  // Call usercallback function
  if (UserFunction)
  {
    (*UserFunction)(UserArguments);
  }
}

/*!
** @}
*/
