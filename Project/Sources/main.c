/* ###################################################################
 **     Filename    : main.c
 **     Project     : Project
 **     Processor   : MK70FN1M0VMJ12
 **     Version     : Driver 01.01
 **     Compiler    : GNU C Compiler
 **     Date/Time   : 2015-07-20, 13:27, # CodeGen: 0
 **     Abstract    :
 **         Main module.
 **         This module contains user's application code.
 **     Settings    :
 **     Contents    :
 **         No public methods
 **
 ** ###################################################################*/
/*!
 ** @file main.c
 ** @version 6.0
 ** @brief
 **   This file contains the high-level code for the project.
 **   It initialises appropriate hardware subsystems,
 **   creates application threads, and then starts the OS.
 */
/*!
 **  @addtogroup main_module main module documentation
 **  @{
 */

/* MODULE main */


#include "brOS.h"

static bool Tower_Init(void)
{
  return (FTM_Init()                                  &&  // Initialise FTM module
          RTC_Init()                                  &&  // Initialise RTC module
          PIT_Init(CPU_BUS_CLK_HZ)                    &&  // Initialise PIT module
          Packet_Init(BAUD_RATE, CPU_BUS_CLK_HZ)      &&  // Initialise packet module
          LEDs_Init()                                 &&  // Initialise LED module
          Flash_Init()                                &&  // Initialise flash module
          Analog_Init((uint32_t)CPU_BUS_CLK_HZ)       &&  // Initialise analog module
//          Initial_TowerNb()                           &&  // Write tower number as last 4 digits of student number to flash
//          Initial_TowerMode()                         &&  // Write tower mode as 1 to flash
          (Logic_SampleInit(&PhaseA))                 &&
          (Logic_SampleInit(&PhaseB))                 &&
          (Logic_SampleInit(&PhaseC))                 &&
          (Logic_AlarmStopWatchInit(&AlarmStopWatch))
          );
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

/*! @brief Initializes the modules to support the LEDs and low power timer.
 *
 *  @param pData is not used but is required by the OS to create a thread.
 *  @note This thread deletes itself after running for the first time.
 */
static void InitModulesThread(void* pData)
{
  // Initialize global semaphores
  OneSecond 	    = OS_SemaphoreCreate(0);
  SampleComplete  = OS_SemaphoreCreate(0);
  ByteReceived    = OS_SemaphoreCreate(0);

  // Blink the LED if it sets up correctly
  if (Tower_Init() && Tower_Startup())
    LEDs_On(LED_ORANGE);

  // Set up timer for sampling
  PIT_Set(INITIAL_SAMPLE_T, false, SAMPLING_CHANNEL);
  PIT_Enable(ON, SAMPLING_CHANNEL);

  // Set a PIT for a stopwatch
  PIT_Set(1e7, false, STOPWATCH_CHANNEL);
  PIT_Enable(ON, STOPWATCH_CHANNEL);

  OS_ThreadDelete(OS_PRIORITY_SELF);  // We only do this once - therefore we should now delete this thread
}

/*! @brief Gets a packet from RxFIFO and responds accordingly to command and parameter bytes.
 */
static void PacketHandleThread(void* pData)
{
  for (;;)
  {
    OS_SemaphoreWait(ByteReceived, 0);

    Packet_Get();                   // Gets a packet from RxFIFO

    LEDs_On(LED_BLUE);              // Turns on blue LED
    FTM_StartTimer(&FTMChLoad[0]);  // Starts FTM timer

    Handle_Packet();                // Responds accordingly to command and parameter bytes of received packet
  }
}

/*! @brief Runs the logic related to all calculations through utilising the necessary functions.
 *
 *  @param pData is not used but is required by the OS to create a thread.
  */
static void LogicThread(void* pData)
{
  for (;;)
  {
    OS_SemaphoreWait(SampleComplete, 0);

    // RMS voltages done on a per 16 sample basis, not sliding window.
    // Convert Phase A ints to voltages
    for (uint8_t i = 0; i < NB_SAMPLES; i++)
    {
      PhaseAVolt[i] = ADC_VOLTS(PhaseAInt[i]);
    }

    // Convert Phase B ints to voltages
    for (uint8_t i = 0; i < NB_SAMPLES; i++)
    {
      PhaseBVolt[i] = ADC_VOLTS(PhaseBInt[i]);
    }

    // Convert Phase C ints to voltages
    for (uint8_t i = 0; i < NB_SAMPLES; i++)
    {
      PhaseCVolt[i] = ADC_VOLTS(PhaseCInt[i]);
    }

    // Calculate RMS for three phases
    Logic_RMS(PhaseAVolt, NB_SAMPLES, &(PhaseA.RmsVal));
    Logic_RMS(PhaseBVolt, NB_SAMPLES, &(PhaseB.RmsVal));
    Logic_RMS(PhaseCVolt, NB_SAMPLES, &(PhaseC.RmsVal));

    // Calculate the voltage deviance for the three phases
    Logic_VoltageDev(PhaseA.RmsVal, &(PhaseA.VoltDev));
    Logic_VoltageDev(PhaseB.RmsVal, &(PhaseB.VoltDev));
    Logic_VoltageDev(PhaseC.RmsVal, &(PhaseC.VoltDev));

    // Frequency Tracking
    Logic_FrequencyTracking();

    // FIXME: Not currently working.
    // FFT
//    Logic_Fft((float complex*)PhaseAVolt, NB_SAMPLES);

    // Depending on the timing type, adjust the alarm based on RMS.
    switch (Logic_CheckRMS())
    {
      case (RMS_FINE):
          AlarmStopWatch.StopWatch = 0;
          Logic_OutputAlarm(OFF);
          Logic_OutputRaise(OFF);
          Logic_OutputLower(OFF);
          AlarmStopWatch.SetLevel = NOT_SET;
          break;

      // If RMS is high
      case (RMS_HIGH):
          Logic_OutputAlarm(ON);
          // If alarm is not already set, set an alarm for high
          if (AlarmStopWatch.SetLevel == NOT_SET)
          {
            // Different alarm approach depending on timing type.
            switch (AlarmStopWatch.TimingType)
            {
              case (DEF_TIMING):
                  Logic_DefiniteAlarm(SET_HIGH);
                  break;
              case (INV_TIMING):
                  Logic_InverseAlarm(SET_HIGH);
                  break;

              // For error catching, code should never reach here.
              default:
                  break;
            }
          }
          // If alarm is already set for a high rms, do nothing if
          // definite-timing, adjust timer if inverse-timing.
          else if (AlarmStopWatch.SetLevel == SET_HIGH)
          {
            // Different alarm approach depending on timing type.
            switch (AlarmStopWatch.TimingType)
            {
              case (DEF_TIMING):
                  // Do nothing
                  break;
              case (INV_TIMING):
                  // Passing in SET_LOW because it's currently SET_HIGH
                  Logic_InverseAlarm(SET_HIGH);
                  break;

              // For error catching, code shouldn't reach here.
              default:
                  break;
            }
            break;
          }
          // If alarm was set for low, set new alarm for high.
          else if (AlarmStopWatch.SetLevel == SET_LOW)
          {
            // Different alarm approach depending on timing type.
            switch (AlarmStopWatch.TimingType)
            {
              case (DEF_TIMING):
                  Logic_DefiniteAlarm(SET_HIGH);
                  break;
              case (INV_TIMING):
                  // Passing in SET_HIGH because it's currently SET_LOW
                  Logic_InverseAlarm(SET_HIGH);
                  break;

              // default: For error catching, code shouldn't reach here.
              default:
                  break;
            }
          }
          break;

      // If RMS is low
      case (RMS_LOW):
          Logic_OutputAlarm(ON);
          // If alarm is not already set, set an alarm for low
          if (AlarmStopWatch.SetLevel == NOT_SET)
          {
            // Different alarm approach depending on timing type.
            switch (AlarmStopWatch.TimingType)
            {
              case (DEF_TIMING):
                  Logic_DefiniteAlarm(SET_LOW);
                  break;
              case (INV_TIMING):
                  Logic_InverseAlarm(SET_LOW);
                  break;

              // default: For error catching, code should never reach here.
              default:
                  break;
              }
          }
          // If alarm is already set for a low rms,
          // Definite timing: do nothing
          // Inverse timing: adjust alarm
          else if (AlarmStopWatch.SetLevel == SET_LOW)
          {
            // Different alarm approach depending on timing type.
            switch (AlarmStopWatch.TimingType)
            {
              case (DEF_TIMING):
                  // Do nothing
                  break;
              case (INV_TIMING):
                  Logic_InverseAlarm(SET_LOW);
                  break;

              // default: For error catching, code shouldn't reach here.
              default:
                  break;
            }
            break;
          }
          // If alarm was set for high, set new alarm for low.
          else if (AlarmStopWatch.SetLevel == SET_HIGH)
          {
            // Different alarm approach depending on timing type.
            switch (AlarmStopWatch.TimingType)
            {
              case (DEF_TIMING):
                  Logic_DefiniteAlarm(SET_LOW);
                  break;
              case (INV_TIMING):
                  Logic_InverseAlarm(SET_LOW);
                  break;

              // For error catching, code shouldn't reach here.
              default:
                  break;
            }
          }
          break;

      // For error catching, code shouldn't reach here.
      default:
        break;
    }
  }
}

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /* Write your local variable definition here */

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/
  OS_ERROR error[NB_THREADS];

  // Initialise low-level clocks etc using Processor Expert code
  PE_low_level_init();

  // Initialize the RTOS
  OS_Init(CPU_CORE_CLK_HZ, true);

  // Threads
  error[0] = OS_ThreadCreate(InitModulesThread,
                          NULL,
                          &InitModulesThreadStack[THREAD_STACK_SIZE - 1],
                          INIT_MODULES_PRIORITY);

  error[1] = OS_ThreadCreate(PacketHandleThread,
                          NULL,
                          &PacketHandleThreadStack[THREAD_STACK_SIZE - 1],
                          PACKET_HANDLE_PRIORITY);

  error[2] = OS_ThreadCreate(RTCThread,
                          NULL,
                          &RTCThreadStack[THREAD_STACK_SIZE - 1],
                          RTC_PRIORITY);

  error[3] = OS_ThreadCreate(LogicThread,
                          NULL,
                          &LogicThreadStack[THREAD_STACK_SIZE - 1],
                          LOGIC_PRIORITY);

  // Check for errors in thread creation
  if (  (error[1] == OS_NO_ERROR) &&
        (error[2] == OS_NO_ERROR) &&
        (error[3] == OS_NO_ERROR) &&
        (error[4] == OS_NO_ERROR))
  {
    // Start multi-threading - never returns!
    OS_Start();
  }

  // Return 1 for error if code reaches here.
  return 1;
}

/*!
 ** @}
 */
