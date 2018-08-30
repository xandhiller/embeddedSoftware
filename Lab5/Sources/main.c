/*!
** @file
** @version 1.0
** @brief  Main module.
**
**   This file contains the high-level code for the project.
**   It initialises appropriate hardware subsystems,
**   creates application threads, and then starts the OS.
**
**   An example of two threads communicating via a semaphore
**   is given that flashes the orange LED. These should be removed
**   when the use of threads and the RTOS is understood.
*/
/*!
**  @addtogroup main_module main module documentation
**  @{
*/
/* MODULE main */

// CPU module - contains low level hardware initialization routines
#include "Cpu.h"

// Simple OS
#include "brOS.h"
#include "LEDs.h"
#include "packet.h"
#include "handle.h"
#include "FIFO.h"
#include "RTC.h"
#include "FTM.h"
#include "accel.h"
#include "UART.h"
#include "PIT.h"
#include "Flash.h"
#include "I2C.h"
#include "analog.h"



TFTMChannel ChLoad[NB_CHANNELS];  // FTM Channel Structure
static TAccelSetup AccelSetup;    // Accelerometer setup structure

// Global semaphores
OS_ECB* ByteReceived;
OS_ECB* OneSecond;

extern TAccelMode ProtocolMode;

// Thread stack inits
OS_THREAD_STACK (InitModulesThreadStack,  THREAD_STACK_SIZE);
OS_THREAD_STACK (PacketHandleThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK (RTCThreadStack,          THREAD_STACK_SIZE);
OS_THREAD_STACK (AccelThreadStack,        THREAD_STACK_SIZE);
OS_THREAD_STACK (I2CThreadStack,          THREAD_STACK_SIZE);
OS_THREAD_STACK (UART_RxThreadStack,      THREAD_STACK_SIZE);
OS_THREAD_STACK (UART_TxThreadStack,      THREAD_STACK_SIZE);
OS_THREAD_STACK (PITThreadStack,          THREAD_STACK_SIZE);
OS_THREAD_STACK (I2C_RxThreadStack,          THREAD_STACK_SIZE);


// CALLBACK FUNCTIONS

/*! @brief Function to be called after I2C finishes read.
 *
 */
static void ReadComplete(void* arg)
{
  if (ProtocolMode == ACCEL_POLL)
  {
    if ((NEW_DATA_X != OLD_DATA_X) || (NEW_DATA_Y != OLD_DATA_Y) || (NEW_DATA_Z != OLD_DATA_Z))
    {
      Packet_Put(ACCEL_DATA, NEW_DATA_X, NEW_DATA_Y, NEW_DATA_Z);
      LEDs_Toggle(LED_GREEN);
    }
  }

  else if (ProtocolMode == ACCEL_INT)
  {
    Packet_Put(ACCEL_DATA, NEW_DATA_X, NEW_DATA_Y, NEW_DATA_Z);
    LEDs_Toggle(LED_GREEN);
  }
}


/*! @brief Function to be called when accelerometer data is ready.
 *
 */
static void DataReady(void* arg)
{
  Accel_ReadXYZ(ACCEL_BUFFER_DATA_INPUT);
}


/*! @brief Calls Polls the accelerometer.
 *
 */
static void PollAccel(void* arg)
{
  if (ProtocolMode == ACCEL_POLL)
  {
    Accel_ReadXYZ(ACCEL_BUFFER_DATA_INPUT);
  }
}


// PRIVATE FUNCTIONS

/*! @brief Calls all initialization functions.
 *
 *  @return bool - TRUE if all modules are successfully initialized.
 */
static bool Tower_Init(void)
{
  return (FTM_Init()                                &&                                   // Initialise FTM module
          RTC_Init()                                &&                                   // Initialise RTC module
          PIT_Init(CPU_BUS_CLK_HZ, PollAccel, NULL) &&    // Initialise PIT module
          Packet_Init(BAUD_RATE, CPU_BUS_CLK_HZ)    &&       // Initialise packet module
          LEDs_Init()                               &&                                  // Initialise LED module
          Flash_Init()                              &&                                 // Initialise flash module
          Accel_Init(&AccelSetup)                   &&                      // Initialise the accelerometer
          Analog_Init((uint32_t)CPU_BUS_CLK_HZ)     &&
          (Initial_TowerNb() || true)               &&                  // Write tower number as last 4 digits of student number to flash
          (Initial_TowerMode() || true));                 // Write tower mode as 1 to flash
}


// THREADS

/*! @brief Initializes the modules to support the LEDs and low power timer.
 *
 *  @param pData is not used but is required by the OS to create a thread.
 *  @note This thread deletes itself after running for the first time.
 */
static void InitModulesThread(void* pData)
{
  // Initialize global semaphores
  ByteReceived 	  = OS_SemaphoreCreate(0);
  OneSecond 	    = OS_SemaphoreCreate(0);

  // Set up channel 0 to blink LED_BLUE when valid packet received
  ChLoad[0].channelNb            = 0;
  ChLoad[0].delayCount           = CPU_MCGFF_CLK_HZ_CONFIG_0;
  ChLoad[0].timerFunction        = TIMER_FUNCTION_OUTPUT_COMPARE;
  ChLoad[0].ioType.outputAction  = TIMER_OUTPUT_TOGGLE;  // Arbitrary decision (?)

  // Set up the accelerometer setup structure
  AccelSetup.moduleClk                      = CPU_BUS_CLK_HZ;
  AccelSetup.dataReadyCallbackFunction      = DataReady;
  AccelSetup.dataReadyCallbackArguments     = NULL;
  AccelSetup.readCompleteCallbackFunction   = ReadComplete;
  AccelSetup.readCompleteCallbackArguments  = NULL;

  // Blink the LED if it sets up correctly
  if (Tower_Init() && Tower_Startup())
    LEDs_On(LED_ORANGE);

  PIT_Set(1e9, false);                // Set the PIT
  PIT_Enable(true);                   // Enables the PIT
  FTM_Set(&ChLoad[0]);                // Set up FTM Channel 0

  OS_ThreadDelete(OS_PRIORITY_SELF);  // We only do this once - therefore we should now delete this thread
}


/*! @brief Gets a packet from RxFIFO and responds accordingly to command and parameter bytes.
 */
static void PacketHandleThread(void* pData)
{
  for (;;)
  {
    OS_SemaphoreWait(ByteReceived, 0);

    Packet_Get();                // Gets a packet from RxFIFO

    LEDs_On(LED_BLUE);           // Turns on blue LED
    FTM_StartTimer(&ChLoad[0]);  // Starts FTM timer

    Handle_Packet();             // Responds accordingly to command and parameter bytes of received packet
  }
}


/*! @brief Gets the time in RTC_TSR and sends it to the PC in hours, minutes, seconds.
 */
static void RTCThread(void* pData)
{
  uint8_t hours, minutes, seconds;                  // Local variables for storing the time in the RTC TSR
  for (;;)
  {
    OS_SemaphoreWait(OneSecond, 0);

    RTC_Get(&hours, &minutes, &seconds);            // Get time from RTC_TSR in hours, minutes, and seconds
    Packet_Put(SET_TIME, hours, minutes, seconds);  // Send time to PC
    LEDs_Toggle(LED_YELLOW);                        // Toggle yellow LED
  }
}


// MAIN

/*! @brief Initialises the hardware, sets up threads, and starts the OS.
 *
 */
/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  OS_ERROR error;

  // Initialize low-level clocks etc using Processor Expert cod  PE_low_level_init();

  OS_Init(CPU_CORE_CLK_HZ, false);  // Initialize the RTOS - without flashing the orange LED "heartbeat"

  // Threads
  error = OS_ThreadCreate(InitModulesThread,
                          NULL,
                          &InitModulesThreadStack[THREAD_STACK_SIZE - 1],
                          INIT_MODULES_PRIORITY);

  error = OS_ThreadCreate(PacketHandleThread,
                          NULL,
                          &PacketHandleThreadStack[THREAD_STACK_SIZE - 1],
                          PACKET_HANDLE_PRIORITY);

  error = OS_ThreadCreate(RTCThread,
                          NULL,
                          &RTCThreadStack[THREAD_STACK_SIZE - 1],
                          RTC_PRIORITY);

  error = OS_ThreadCreate(Accel_Thread,
                          NULL,
                          &AccelThreadStack[THREAD_STACK_SIZE - 1],
                          ACCEL_PRIORITY);

  error = OS_ThreadCreate(I2C_RxThread,
                          NULL,
                          &I2C_RxThreadStack[THREAD_STACK_SIZE - 1],
                          I2C_PRIORITY);

  error = OS_ThreadCreate(UART_RxThread,
                          NULL,
                          &UART_RxThreadStack[THREAD_STACK_SIZE - 1],
                          UART_RX_PRIORITY);

  error = OS_ThreadCreate(UART_TxThread,
                          NULL,
                          &UART_TxThreadStack[THREAD_STACK_SIZE - 1],
                          UART_TX_PRIORITY);

  error = OS_ThreadCreate(PIT_Thread,
                          NULL,
                          &PITThreadStack[THREAD_STACK_SIZE - 1],
                          PIT_PRIORITY);

  OS_Start();  // Start multithreading - never returns!
}


/*!
** @}
*/
