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
/* MODULE FTM */

#include "brOS.h"

// Private global variables
static const uint8_t COUNT_RESET = 1;
static const uint32_t FIXED_FREQ_CLK = 0b10;


// PUBLIC FUNCTIONS

bool FTM_Init(void)
{

  SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK;        // Enable out the FTM0

  // TODO: Disable write protection

  // Set NVIC bits
  // IRQ = 62
  // NVIC non-IPR= 1, IPR=15
  // non-IPR == suffix to the NVIC registers
  // Clear any pending interrupts on FTM0
  // Left shift == IRQ % 32 == 62 % 32 == 30
  NVICICPR1 = (1 << 30);
  NVICISER1 = (1 << 30);


  FTM0_CNTIN  =   0x00;                         // Set the initial counter value to 0
  FTM0_CNT    =  COUNT_RESET;                  // Write to CNT
  FTM0_MOD    =  FTM_MOD_MOD_MASK;             // Write to MOD
  FTM0_SC     |=  FTM_SC_CLKS(FIXED_FREQ_CLK);  // Write (10)b to two-byte CLKS register


  // TODO: Enable write protection

  return true;
}


bool FTM_Set(const TFTMChannel* const aFTMChannel)
{
  // TODO: Channel validation

  // TODO: Disable protection on the register before writing

  switch (aFTMChannel->timerFunction)
  {
    case (TIMER_FUNCTION_INPUT_CAPTURE):
      // FIXME: Configure for input compare
      return false;
      // Not currently used, will configure later.
      break;

      case (TIMER_FUNCTION_OUTPUT_COMPARE):
      // Write to the MSnB:MSnA as per pg 1212 of Ref Manual
      FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_MSB_MASK;      // Write 0
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


bool FTM_StartTimer(const TFTMChannel* const aFTMChannel)
{
  // TODO: Channel validation

  // TODO: Do timerFunction check


  FTM0_C0V  =   aFTMChannel->delayCount;  // Load up a value into channel 0
  FTM0_C0SC |=  FTM_CnSC_CHIE_MASK;                // Enable interrupts

  return true;
}


void __attribute__ ((interrupt)) FTM0_ISR(void)
{
  OS_ISREnter();

  // TODO: Determine reason for varied delay in Blue LED

  // Interrupt has occurred for FTM0 -- which channel could it be?!
  for (int i = 0; i < NB_CHANNELS; i++)
  {
  // Check if channel i has interrupts enabled and has interrupt flag set
    if ((FTM0_CnSC(i) & FTM_CnSC_CHIE_MASK) && (FTM0_CnSC(i) & FTM_CnSC_CHF_MASK))
    {
      FTM0_CnSC(i) &= ~FTM_CnSC_CHF_MASK;   // Clear the flag
      FTM0_CnSC(i) &= ~FTM_CnSC_CHIE_MASK;  // Turn off the interrupts, the timer has run its purpose.

      // Move to necessary processing for each channel.
      switch(i)
      {
        case 0:
          // Channel 0 Processing
          LEDs_Off(LED_BLUE);
          break;

        case 1:
          // Channel 1 Processing

          break;

        case 2:
          // Channel 2 Processing
          break;

        case 3:
          // Channel 3 Processing
          break;

        case 4:
          // Channel 4 Processing
          break;

        case 5:
          // Channel 5 Processing
          break;

        case 6:
          // Channel 6 Processing
          break;

        case 7:
          // Channel 7 Processing
          break;
      }
    }
  }

  OS_ISRExit();
}


/*!
** @}
*/
