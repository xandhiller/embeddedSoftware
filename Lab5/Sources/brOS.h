/*! @file
 *
 *  @brief Includes OS.h, defines, and enumerations used in OS implementation.
 *
 *  @author 12551382 Samin Saif and 11850637 Alex Hiller
 *  @date 2018-05-29
 */

#ifndef SOURCES_BROS_H_
#define SOURCES_BROS_H_

#include "OS.h"

#define THREAD_STACK_SIZE 300

// Thread priorities
enum
{
  INIT_MODULES_PRIORITY,
  UART_RX_PRIORITY,
  UART_TX_PRIORITY,
  I2C_PRIORITY,
  ACCEL_PRIORITY,
  PIT_PRIORITY,
  RTC_PRIORITY,
  PACKET_HANDLE_PRIORITY,
};

#endif /* SOURCES_BROS_H_ */
