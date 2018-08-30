/*! @file
 *
 *  @brief Includes OS.h, defines, and enumerations used in OS implementation as
 *  well as global variables, which are accessed in multiple files.
 *
 *  @author 11850637 Alex Hiller
 *  @date 2018-06-27
 */

#ifndef SOURCES_BROS_H_
#define SOURCES_BROS_H_

#include "Cpu.h"
#include "Events.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "OS.h"
#include "types.h"
#include "Flash.h"
#include "FIFO.h"
#include "UART.h"
#include "PIT.h"
#include "RTC.h"
#include "FTM.h"
#include "LEDs.h"
#include "packet.h"
#include "handle.h"
#include "Analog.h"
#include "logic.h"
#include <math.h>
#include <complex.h>
#include <stdio.h>

// OS-related constants
#define BAUD_RATE           115200
#define THREAD_STACK_SIZE   800
#define NB_THREADS          7

// Thread priorities
enum
{
  INIT_MODULES_PRIORITY,
  UART_RX_PRIORITY,
  UART_TX_PRIORITY,
  LOGIC_PRIORITY,
  RTC_PRIORITY,
  PACKET_HANDLE_PRIORITY,
};

// Global Data Structures
TFTMChannel FTMChLoad[NB_CHANNELS];  // FTM Channel Structure
TData Samples[NB_PHASES];
TAlarmStopWatch AlarmStopWatch;

#define PhaseA        Samples[0]
#define PhaseB        Samples[1]
#define PhaseC        Samples[2]
#define NewestPhaseA  Samples[0].ADCBuffer[PhaseA.Counter]
#define NewestPhaseB  Samples[1].ADCBuffer[PhaseB.Counter]
#define NewestPhaseC  Samples[2].ADCBuffer[PhaseC.Counter]
#define PhaseAInt     Samples[0].ADCBuffer
#define PhaseBInt     Samples[1].ADCBuffer
#define PhaseCInt     Samples[2].ADCBuffer
#define PhaseAVolt    Samples[0].FloatBuffer
#define PhaseBVolt    Samples[1].FloatBuffer
#define PhaseCVolt    Samples[2].FloatBuffer

// Global semaphores
OS_ECB* ByteReceived;
OS_ECB* OneSecond;
OS_ECB* SampleComplete;

// Global Thread stacks
OS_THREAD_STACK (InitModulesThreadStack,  THREAD_STACK_SIZE);
OS_THREAD_STACK (PacketHandleThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK (RTCThreadStack,          THREAD_STACK_SIZE);
OS_THREAD_STACK (LogicThreadStack,        THREAD_STACK_SIZE);

#endif /* SOURCES_BROS_H_ */
