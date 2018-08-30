/*! @file
 *
 *  @brief HAL for the accelerometer.
 *
 *  This contains the functions for interfacing to the MMA8451Q accelerometer.
 *
 *  @author 12551382 Samin Saif and 11850637 Alex Hiller
 *  @date 2018-05-10
 */
/*!
 *  @addtogroup accel_module accel module documentation
 *  @{
*/
/* MODULE accel */

// Accelerometer functions
#include "accel.h"

// Inter-Integrated Circuit
#include "I2C.h"

// Median filter
#include "median.h"

// K70 module registers
#include "MK70F12.h"

// CPU and PE_types are needed for critical section variables and the definition of NULL pointer
#include "CPU.h"
#include "PE_types.h"

// Accelerometer registers -- only need MSB in 8-bit mode, ignore LSB
#define ADDRESS_OUT_X_MSB 0x01

#define ADDRESS_INT_SOURCE 0x0C

static union
{
  uint8_t byte;			/*!< The INT_SOURCE bits accessed as a byte. */
  struct
  {
    uint8_t SRC_DRDY   : 1;	/*!< Data ready interrupt status. */
    uint8_t              : 1;
    uint8_t SRC_FF_MT  : 1;	/*!< Freefall/motion interrupt status. */
    uint8_t SRC_PULSE  : 1;	/*!< Pulse detection interrupt status. */
    uint8_t SRC_LNDPRT : 1;	/*!< Orientation interrupt status. */
    uint8_t SRC_TRANS  : 1;	/*!< Transient interrupt status. */
    uint8_t SRC_FIFO   : 1;	/*!< FIFO interrupt status. */
    uint8_t SRC_ASLP   : 1;	/*!< Auto-SLEEP/WAKE interrupt status. */
  } bits;			/*!< The INT_SOURCE bits accessed individually. */
} INT_SOURCE_Union;

#define INT_SOURCE     		INT_SOURCE_Union.byte
#define INT_SOURCE_SRC_DRDY	INT_SOURCE_Union.bits.SRC_DRDY
#define INT_SOURCE_SRC_FF_MT	CTRL_REG4_Union.bits.SRC_FF_MT
#define INT_SOURCE_SRC_PULSE	CTRL_REG4_Union.bits.SRC_PULSE
#define INT_SOURCE_SRC_LNDPRT	CTRL_REG4_Union.bits.SRC_LNDPRT
#define INT_SOURCE_SRC_TRANS	CTRL_REG4_Union.bits.SRC_TRANS
#define INT_SOURCE_SRC_FIFO	CTRL_REG4_Union.bits.SRC_FIFO
#define INT_SOURCE_SRC_ASLP	CTRL_REG4_Union.bits.SRC_ASLP

#define ADDRESS_CTRL_REG1 0x2A

typedef enum
{
  DATE_RATE_800_HZ,
  DATE_RATE_400_HZ,
  DATE_RATE_200_HZ,
  DATE_RATE_100_HZ,
  DATE_RATE_50_HZ,
  DATE_RATE_12_5_HZ,
  DATE_RATE_6_25_HZ,
  DATE_RATE_1_56_HZ
} TOutputDataRate;

typedef enum
{
  SLEEP_MODE_RATE_50_HZ,
  SLEEP_MODE_RATE_12_5_HZ,
  SLEEP_MODE_RATE_6_25_HZ,
  SLEEP_MODE_RATE_1_56_HZ
} TSLEEPModeRate;

static union
{
  uint8_t byte;			/*!< The CTRL_REG1 bits accessed as a byte. */
  struct
  {
    uint8_t ACTIVE    : 1;	/*!< Mode selection. */
    uint8_t F_READ    : 1;	/*!< Fast read mode. */
    uint8_t LNOISE    : 1;	/*!< Reduced noise mode. */
    uint8_t DR        : 3;	/*!< Data rate selection. */
    uint8_t ASLP_RATE : 2;	/*!< Auto-WAKE sample frequency. */
  } bits;			/*!< The CTRL_REG1 bits accessed individually. */
} CTRL_REG1_Union;

#define CTRL_REG1     		  CTRL_REG1_Union.byte
#define CTRL_REG1_ACTIVE	  CTRL_REG1_Union.bits.ACTIVE
#define CTRL_REG1_F_READ  	  CTRL_REG1_Union.bits.F_READ
#define CTRL_REG1_LNOISE  	  CTRL_REG1_Union.bits.LNOISE
#define CTRL_REG1_DR	    	  CTRL_REG1_Union.bits.DR
#define CTRL_REG1_ASLP_RATE	  CTRL_REG1_Union.bits.ASLP_RATE

#define ADDRESS_CTRL_REG2 0x2B

#define ADDRESS_CTRL_REG3 0x2C

static union
{
  uint8_t byte;			/*!< The CTRL_REG3 bits accessed as a byte. */
  struct
  {
    uint8_t PP_OD       : 1;	/*!< Push-pull/open drain selection. */
    uint8_t IPOL        : 1;	/*!< Interrupt polarity. */
    uint8_t WAKE_FF_MT  : 1;	/*!< Freefall/motion function in SLEEP mode. */
    uint8_t WAKE_PULSE  : 1;	/*!< Pulse function in SLEEP mode. */
    uint8_t WAKE_LNDPRT : 1;	/*!< Orientation function in SLEEP mode. */
    uint8_t WAKE_TRANS  : 1;	/*!< Transient function in SLEEP mode. */
    uint8_t FIFO_GATE   : 1;	/*!< FIFO gate bypass. */
  } bits;			/*!< The CTRL_REG3 bits accessed individually. */
} CTRL_REG3_Union;

#define CTRL_REG3     		    CTRL_REG3_Union.byte
#define CTRL_REG3_PP_OD		    CTRL_REG3_Union.bits.PP_OD
#define CTRL_REG3_IPOL		    CTRL_REG3_Union.bits.IPOL
#define CTRL_REG3_WAKE_FF_MT	    CTRL_REG3_Union.bits.WAKE_FF_MT
#define CTRL_REG3_WAKE_PULSE	    CTRL_REG3_Union.bits.WAKE_PULSE
#define CTRL_REG3_WAKE_LNDPRT	    CTRL_REG3_Union.bits.WAKE_LNDPRT
#define CTRL_REG3_WAKE_TRANS	    CTRL_REG3_Union.bits.WAKE_TRANS
#define CTRL_REG3_FIFO_GATE	    CTRL_REG3_Union.bits.FIFO_GATE

#define ADDRESS_CTRL_REG4 0x2D

static union
{
  uint8_t byte;			/*!< The CTRL_REG4 bits accessed as a byte. */
  struct
  {
    uint8_t INT_EN_DRDY   : 1;	/*!< Data ready interrupt enable. */
    uint8_t               : 1;
    uint8_t INT_EN_FF_MT  : 1;	/*!< Freefall/motion interrupt enable. */
    uint8_t INT_EN_PULSE  : 1;	/*!< Pulse detection interrupt enable. */
    uint8_t INT_EN_LNDPRT : 1;	/*!< Orientation interrupt enable. */
    uint8_t INT_EN_TRANS  : 1;	/*!< Transient interrupt enable. */
    uint8_t INT_EN_FIFO   : 1;	/*!< FIFO interrupt enable. */
    uint8_t INT_EN_ASLP   : 1;	/*!< Auto-SLEEP/WAKE interrupt enable. */
  } bits;			/*!< The CTRL_REG4 bits accessed individually. */
} CTRL_REG4_Union;

#define CTRL_REG4            		CTRL_REG4_Union.byte
#define CTRL_REG4_INT_EN_DRDY	  CTRL_REG4_Union.bits.INT_EN_DRDY
#define CTRL_REG4_INT_EN_FF_MT	CTRL_REG4_Union.bits.INT_EN_FF_MT
#define CTRL_REG4_INT_EN_PULSE	CTRL_REG4_Union.bits.INT_EN_PULSE
#define CTRL_REG4_INT_EN_LNDPRT	CTRL_REG4_Union.bits.INT_EN_LNDPRT
#define CTRL_REG4_INT_EN_TRANS	CTRL_REG4_Union.bits.INT_EN_TRANS
#define CTRL_REG4_INT_EN_FIFO	  CTRL_REG4_Union.bits.INT_EN_FIFO
#define CTRL_REG4_INT_EN_ASLP	  CTRL_REG4_Union.bits.INT_EN_ASLP

#define ADDRESS_CTRL_REG5 0x2E

static union
{
  uint8_t byte;			/*!< The CTRL_REG5 bits accessed as a byte. */
  struct
  {
    uint8_t INT_CFG_DRDY   : 1;	/*!< Data ready interrupt enable. */
    uint8_t                : 1;
    uint8_t INT_CFG_FF_MT  : 1;	/*!< Freefall/motion interrupt enable. */
    uint8_t INT_CFG_PULSE  : 1;	/*!< Pulse detection interrupt enable. */
    uint8_t INT_CFG_LNDPRT : 1;	/*!< Orientation interrupt enable. */
    uint8_t INT_CFG_TRANS  : 1;	/*!< Transient interrupt enable. */
    uint8_t INT_CFG_FIFO   : 1;	/*!< FIFO interrupt enable. */
    uint8_t INT_CFG_ASLP   : 1;	/*!< Auto-SLEEP/WAKE interrupt enable. */
  } bits;			/*!< The CTRL_REG5 bits accessed individually. */
} CTRL_REG5_Union;

#define CTRL_REG5     		      	CTRL_REG5_Union.byte
#define CTRL_REG5_INT_CFG_DRDY		CTRL_REG5_Union.bits.INT_CFG_DRDY
#define CTRL_REG5_INT_CFG_FF_MT		CTRL_REG5_Union.bits.INT_CFG_FF_MT
#define CTRL_REG5_INT_CFG_PULSE		CTRL_REG5_Union.bits.INT_CFG_PULSE
#define CTRL_REG5_INT_CFG_LNDPRT	CTRL_REG5_Union.bits.INT_CFG_LNDPRT
#define CTRL_REG5_INT_CFG_TRANS		CTRL_REG5_Union.bits.INT_CFG_TRANS
#define CTRL_REG5_INT_CFG_FIFO		CTRL_REG5_Union.bits.INT_CFG_FIFO
#define CTRL_REG5_INT_CFG_ASLP		CTRL_REG5_Union.bits.INT_CFG_ASLP


// Public Global Variables

// Accelerometer circular overwriting data buffer
extern struct
{
  TAccelData Buffer[3];
  TAccelData FilteredAccelData;
  uint8_t Index;
} AccelDataBuffer;

// Private global variable
static void (*DataReadyCallbackFunction)(void*);
static void* DataReadyCallbackArguments;
static void (*ReadCompleteCallbackFunction)(void*);
static void* ReadCompleteCallbackArguments;
static TAccelMode AccelMode;
static TI2CModule I2CSetupStruct;


// PRIVATE FUNCTIONS

static void setI2CStruct(TI2CModule* aI2CSetupStruct)
{
  aI2CSetupStruct->primarySlaveAddress            = 0x1D;
  aI2CSetupStruct->baudRate                       = 100000;
  aI2CSetupStruct->readCompleteCallbackFunction   = ReadCompleteCallbackFunction;
  aI2CSetupStruct->readCompleteCallbackArguments  = NULL;
}

static void ActivateAccelerometer(bool activate)
{
  switch (activate)
  {
    case (true):
      CTRL_REG1_ACTIVE = true;
      // Put settings onto the register(s)
      I2C_Write(ADDRESS_CTRL_REG1, CTRL_REG1);
      break;

    case (false):
      CTRL_REG1_ACTIVE = false;
      // Put settings onto the register(s)
      I2C_Write(ADDRESS_CTRL_REG1, CTRL_REG1);
      break;
  }
}

/*! @brief Increment the index of Public Global Data buffer in a circular fashion.
 *
 */
static void AccelBufferIncrementIndex(void)
{
  if ((AccelDataBuffer.Index == 0)  || (AccelDataBuffer.Index == 1))
  {
    AccelDataBuffer.Index++;
  }
  else
  {
    AccelDataBuffer.Index = 0;
  }
}


// PUBLIC FUNCTIONS

/*! @brief Initializes the accelerometer by calling the initialization routines of the supporting software modules.
 *
 *  @param accelSetup is a pointer to an accelerometer setup structure.
 *  @return BOOL - TRUE if the accelerometer module was successfully initialized.
 */
bool Accel_Init(const TAccelSetup* const accelSetup)
{
  // Save callback functions
  DataReadyCallbackFunction     = accelSetup->dataReadyCallbackFunction;
  DataReadyCallbackArguments    = accelSetup->dataReadyCallbackArguments;
  ReadCompleteCallbackFunction  = accelSetup->dataReadyCallbackFunction;
  ReadCompleteCallbackArguments = accelSetup->readCompleteCallbackArguments;

  // TODO:
  setI2CStruct(&I2CSetupStruct);
  I2C_Init(&I2CSetupStruct, accelSetup->moduleClk);

  // Change frequency of sampling to (1/56)Hz
  CTRL_REG1_DR = DATE_RATE_1_56_HZ;
  CTRL_REG1_ASLP_RATE = SLEEP_MODE_RATE_1_56_HZ;

  // Set clock gate control for PortB
  SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK;

  // Choose the GPIO alternative for pin 4 on PortB for the INT1 accelerometer to input to K70.
  PORTB_PCR4 |= PORT_PCR_MUX(1);

  // Default data direction is input for GPIO -- no need to set.

  // Set device's active bit - turns device to wake mode.
  CTRL_REG1_ACTIVE = true;

  // Setup as 8-bit data resolution
  CTRL_REG1_F_READ = true;

  // Sampling rate when asleep -- leave as default of 50Hz

  // Put settings onto the register(s)
  EnterCritical();
  ActivateAccelerometer(false);
  I2C_Write(ADDRESS_CTRL_REG1, CTRL_REG1);
  ActivateAccelerometer(true);
  ExitCritical();
  return true;
}

/*! @brief Reads X, Y and Z accelerations.
 *  @param data is a an array of 3 bytes where the X, Y and Z data are stored.
 */
void Accel_ReadXYZ(uint8_t data[3])
{
  // If there are issues, look into the "auto-increment" addresses on pg20 of the accelerometer manual
  switch (AccelMode)
  {
    case (ACCEL_POLL):
      I2C_PollRead(ADDRESS_OUT_X_MSB, data, 3);
      break;

    // Handling for interrupt mode necessary?

    case (ACCEL_INT):
      I2C_IntRead(ADDRESS_OUT_X_MSB, data, 3);
      break;
  }
  AccelBufferIncrementIndex();
}

/*! @brief Set the mode of the accelerometer.
 *  @param mode specifies either polled or interrupt driven operation.
 */
void Accel_SetMode(const TAccelMode mode)
{
  AccelMode = mode;

  switch(mode)
  {
    case (ACCEL_POLL):
      // Setting AccelMode changes the RTC callback function to now push packets of accelData
      AccelMode = ACCEL_POLL;
      // Sampling rate doesn't matter, leave it as default 800hz
      // Sleep mode default rate doesn't matter, leave it as 50Hz
      // Clear the interrupt bit in case modes are switched intra-program

      // TODO: Undo the effects set in mode = ACCEL_INT in case of toggling of protocol mode. i.e. undo interrupt setting.


      CTRL_REG4_INT_EN_DRDY = false;
      EnterCritical();
      ActivateAccelerometer(false);
      I2C_Write(ADDRESS_CTRL_REG4, CTRL_REG4);
      ActivateAccelerometer(true);
      ExitCritical();
      break;

    case (ACCEL_INT):
      AccelMode = ACCEL_INT;
      // Set asynchronous process on, send only if new data
      // Set 'data ready' interrupt for Accelerometer
      CTRL_REG4_INT_EN_DRDY = true;

      // Pipe the 'data ready' interrupt through the Accelerometer's INT1 pin
      CTRL_REG5_INT_CFG_DRDY = true;

//      CTRL_REG3_IPOL = true;

      // Set arm bit for PortB interrupts -- Flag and interrupt on falling edge.
      PORTB_PCR4 |= PORT_PCR_IRQC(0b1010);
      // This is because on CTRL_REG3[IPOL] is by default set to signal interrupts by Active Low

      // IRQ = 88
      // Non-IPR = 2
      // NVIC Setup for PortB
      NVICICPR2 = (1 << 24);  // Clear any pending interrupts
      NVICISER2 = (1 << 24);  // Interrupt enable

      EnterCritical();
      ActivateAccelerometer(false);
      I2C_Write(ADDRESS_CTRL_REG1, CTRL_REG1);
      I2C_Write(ADDRESS_CTRL_REG4, CTRL_REG4);
      I2C_Write(ADDRESS_CTRL_REG3, CTRL_REG3);
      ActivateAccelerometer(true);
      ExitCritical();
      break;
  }
}

/*! @brief Interrupt service routine for the accelerometer.
 *
 *  The accelerometer has data ready.
 *  The user callback function will be called.
 *  @note Assumes the accelerometer has been initialized.
 */
void __attribute__ ((interrupt)) AccelDataReady_ISR(void)
{
  // FYI: Connected to PortB pin 4 -- has been enabled in the vectors.c table.
  // Clear the PortB interrupt flag (connected to accelerometer's INT1 pin) (w1c)
  PORTB_PCR4 |= PORT_PCR_ISF_MASK;

  // Prepare the data for sending with median filter.
  Accel_FilterData();

  if (DataReadyCallbackFunction)
  {
    (*DataReadyCallbackFunction)(DataReadyCallbackArguments);
  }

//  //Not sure what this should be? Turn off interrupts, filter data and packet push?
//  if (ReadCompleteCallbackFunction)
//  {
//    (*ReadCompleteCallbackFunction)(ReadCompleteCallbackArguments);
//  }
}


/*! @brief Filters the data in the buffer with the median filter.
 *
 */
void Accel_FilterData(void)
{
  FILTERED_DATA_X = Median_Filter3(ACCEL_X1, ACCEL_X2, ACCEL_X3);
  FILTERED_DATA_Y = Median_Filter3(ACCEL_Y1, ACCEL_Y2, ACCEL_Y3);
  FILTERED_DATA_Z = Median_Filter3(ACCEL_Z1, ACCEL_Z2, ACCEL_Z3);
}


/*!
 * @}
*/
