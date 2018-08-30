/*! @file
 *
 *  @brief Routines to access the LEDs on the TWR-K70F120M.
 *
 *  This contains the functions for operating the LEDs.
 *
 *  @author 12551382 Samin Saif and 11850637 Alex Hiller
 *  @date 2018-04-14
 */
/*!
**  @addtogroup LEDs_module LEDs module documentation
**  @{
*/
/* MODULE LEDs */

#include "LEDs.h"
#include "IO_Map.h"


// PUBLIC FUNCTIONS

bool LEDs_Init(void)
{
  // Enable port A
  SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;

  // Mux port A ALT1
  PORTA_PCR10 |= PORT_PCR_MUX(1);
  PORTA_PCR11 |= PORT_PCR_MUX(1);
  PORTA_PCR28 |= PORT_PCR_MUX(1);
  PORTA_PCR29 |= PORT_PCR_MUX(1);

  // Set the GPIO_PDOR bits that correspond to the orange, yellow, green, and blue LEDs
  GPIOA_PSOR  |= GPIO_PSOR_PTSO(LED_ORANGE);
  GPIOA_PSOR  |= GPIO_PSOR_PTSO(LED_YELLOW);
  GPIOA_PSOR  |= GPIO_PSOR_PTSO(LED_GREEN);
  GPIOA_PSOR  |= GPIO_PSOR_PTSO(LED_BLUE);

  // Set direction for the data
  GPIOA_PDDR  |= GPIO_PDDR_PDD(LED_ORANGE);
  GPIOA_PDDR  |= GPIO_PDDR_PDD(LED_YELLOW);
  GPIOA_PDDR  |= GPIO_PDDR_PDD(LED_GREEN);
  GPIOA_PDDR  |= GPIO_PDDR_PDD(LED_BLUE);

  return true;
}


void LEDs_On(const TLED color)
{
  GPIOA_PCOR |= GPIO_PCOR_PTCO(color);
}


void LEDs_Off(const TLED color)
{
  GPIOA_PSOR |= GPIO_PSOR_PTSO(color);
}


void LEDs_Toggle(const TLED color)
{
  GPIOA_PTOR |= GPIO_PTOR_PTTO(color);
}


/*!
** @}
*/
