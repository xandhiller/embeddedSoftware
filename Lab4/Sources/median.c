/*! @file
 *
 *  @brief Median filter.
 *
 *  This contains the functions for performing a median filter on byte-sized data.
 *
 *  @author 12551382 Samin Saif and 11850637 Alex Hiller
 *  @date 2018-05-12
 */
/*!
**  @addtogroup median_module median module documentation
**  @{
*/
/* MODULE median */

#include "median.h"

#define TWOS_COMPLEMENT(x) ~(x)+(0b1)

/*! @brief Median filters 3 bytes.
 *
 *  @param n1 is the first  of 3 bytes for which the median is sought.
 *  @param n2 is the second of 3 bytes for which the median is sought.
 *  @param n3 is the third  of 3 bytes for which the median is sought.
 */
uint8_t Median_Filter3(const uint8_t n1, const uint8_t n2, const uint8_t n3)
{
  uint8_t tcN1, tcN2, tcN3;

  // Turn the two's complement into regular binary for median comparison
  tcN1 = TWOS_COMPLEMENT(n1);
  tcN2 = TWOS_COMPLEMENT(n2);
  tcN3 = TWOS_COMPLEMENT(n3);

  // In the case that n1>n2>n3 or n3>n2>n1
  if (((tcN1 > tcN2) && (tcN2 > tcN3)) || ((tcN3 > tcN2) && (tcN2 > tcN1)))
  {
    // Return the two's complement number
    return n2;
  }

  // In the case that n1>n3>n2 or n2>n3>n1
  else if (((tcN1 > tcN3) && (tcN3 > tcN2)) || ((tcN2 > tcN3) && (tcN3 > tcN1)))
  {
    // Return the two's complement number
    return n3;
  }

  // In the case that n2>n1>n3 or n3>n1>n2
  else
  {
    // Return the two's complement number
    return n1;
  }
}

/*!
** @}
*/
