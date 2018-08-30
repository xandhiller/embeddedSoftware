/*! @file
 *
 *  @brief Routines for setting up the FlexTimer module (FTM) on the TWR-K70F120M.
 *
 *  This contains the functions for operating the FlexTimer module (FTM).
 *
 *  @author 12551382 Samin Saif and 11850637 Alex Hiller
 *  @date 2018-04-27
 */
/*!
**  @addtogroup FTM_module FTM module documentation
**  @{
*/

#include "FTM.h"
// for EnterCritical() and ExitCritical()
#include "Cpu.h"

// Private global variables
static const uint8_t COUNT_RESET = 1;
static const uint32_t FIXED_FREQ_CLK = 0b10;
static void (*UserFunction)(void*);
static void* UserArguments;

/*! @brief Sets up the FTM before first use.
 *
 *  Enables the FTM as a free running 16-bit counter.
 *  @return bool - TRUE if the FTM was successfully initialized.
 */
bool FTM_Init()
{
  EnterCritical();

  // Enable out the FTM0
  SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK;

  // TODO: Disable write protection

  // Set NVIC bits
  // IRQ = 62
  // NVIC non-IPR= 1, IPR=15
  // non-IPR == suffix to the NVIC registers
  // Clear any pending interrupts on FTM0
  // Left shift == IRQ % 32 == 62 % 32 == 30
  NVICICPR1 = (1 << 30);
  // Enable interrupts from PIT timer0 module
  NVICISER1 = (1 << 30);

  // Set the initial counter value to 0
  FTM0_CNTIN = 0x00;

  // Write to MOD
  FTM0_MOD |= FTM_MOD_MOD_MASK;

  // Write to CNT
  FTM0_CNT |= COUNT_RESET;

  // Write CLKS[1:0]
  FTM0_SC |= FTM_SC_CLKS(FIXED_FREQ_CLK);

  // TODO: Enable write  protection

  ExitCritical();

  return true;
}

/*! @brief Sets up a timer channel.
 *
 *  @param aFTMChannel is a structure containing the parameters to be used in setting up the timer channel.
 *    channelNb is the channel number of the FTM to use.
 *    delayCount is the delay count (in module clock periods) for an output compare event.
 *    timerFunction is used to set the timer up as either an input capture or an output compare.
 *    ioType is a union that depends on the setting of the channel as input capture or output compare:
 *      outputAction is the action to take on a successful output compare.
 *      inputDetection is the type of input capture detection.
 *    userFunction is a pointer to a user callback function.
 *    userArguments is a pointer to the user arguments to use with the user callback function.
 *  @return bool - TRUE if the timer was set up successfully.
 *  @note Assumes the FTM has been initialized.
 */
bool FTM_Set(const TFTMChannel* const aFTMChannel)
{
  UserFunction = aFTMChannel->userFunction;
  UserArguments = aFTMChannel->userArguments;

  // TODO: Un-protect the register before writing
  switch (aFTMChannel->timerFunction)
  {
    case (TIMER_FUNCTION_INPUT_CAPTURE):
      // FIXME: Configure for input compare
      return false; // Not currently used, will configure later.
      break;
    case (TIMER_FUNCTION_OUTPUT_COMPARE):
      // Write to the MSnB:MSnA as per page
      FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_MSB_MASK;     // Write 0
      FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_MSA_MASK; 	    // Write 1
      switch(aFTMChannel->ioType.outputAction)
      {
	case (TIMER_OUTPUT_DISCONNECT):
	  // Set ELSB & ELSA registers as per pg 1212 of Ref Manual
          FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSB_MASK; // Write 0
          FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSA_MASK; // Write 0
	  break;
	case (TIMER_OUTPUT_TOGGLE):
	  // Set ELSB & ELSA registers as per pg 1212 of Ref Manual
	  FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSB_MASK; // Write 0
	  FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSA_MASK;  // Write 1
	  break;
	case (TIMER_OUTPUT_LOW):
	  // Set ELSB & ELSA registers as per pg 1212 of Ref Manual
	  FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSB_MASK;  // Write 1
	  FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSA_MASK; // Write 0
	  break;
	case (TIMER_OUTPUT_HIGH):
	  // Set ELSB & ELSA registers as per pg 1212 of Ref Manual
	  FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSB_MASK;  // Write 1
	  FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSA_MASK;  // Write 1
	  break;
      }
      break;
  }
}

/*! @brief Starts a timer if set up for output compare.
 *
 *  @param aFTMChannel is a structure containing the parameters to be used in setting up the timer channel.
 *  @return bool - TRUE if the timer was started successfully.
 *  @note Assumes the FTM has been initialized.
 */
bool FTM_StartTimer(const TFTMChannel* const aFTMChannel)
{
  // Load up a value into channel 0
  FTM0_C0V |= (FTM0_CNT + aFTMChannel->delayCount);

  // Set arm bit
  FTM0_C0SC |= FTM_CnSC_CHIE_MASK;

  return true;
}

/*! @brief Interrupt service routine for the FTM.
 *
 *  If a timer channel was set up as output compare, then the user callback function will be called.
 *  @note Assumes the FTM has been initialized.
 */
void __attribute__ ((interrupt)) FTM0_ISR(void)
{
  // Clear the interrupt flag
  FTM0_CnSC(0) &= ~FTM_CnSC_CHF_MASK;

  // Call user callback function
  if (UserFunction)
  {
    (*UserFunction)(UserArguments);
  }
}

/*!
** @}
*/
