/*! @file
 *
 *  @brief I/O routines for the K70 I2C interface.
 *
 *  This contains the functions for operating the I2C (inter-integrated circuit) module.
 *
 *  @author 12551382 Samin Saif and 11850637 Alex Hiller
 *  @date 2018-05-10
 */
/*!
**  @addtogroup I2C_module I2C module documentation
**  @{
*/
/* MODULE I2C */

#include "I2C.h"
#include "Cpu.h"

// FSM for stages in I2C_IntRead
typedef enum
{
  STAGE_START,
  STAGE_REGISTER_ADDRESS,
  STAGE_REPEAT_START,
  STAGE_RECEIVE_MODE,
  STAGE_DUMMY_READ,
  STAGE_READ_DATA,
  STAGE_READ_BYTE,
  STAGE_READ_SECOND_LAST_BYTE,
  STAGE_READ_LAST_BYTE
} TI2CIntReadStage;

// Private Global Variables
static void (*ReadCompleteCallbackFunction)(void*);
static void* ReadCompleteCallbackArguments;
static uint8_t SlaveAddress;
static uint8_t RegisterAddress;
static uint8_t *Data;
static uint8_t NbBytes;
static uint8_t MaxNbBytes;

// FSM that keeps track of stage in ITC_IntRead
static TI2CIntReadStage I2CIntReadStage;

// All possible SCLdivider values
static const uint32_t SCLdivider[64] = {20, 22, 24, 26, 28, 30, 34, 40,
                                        28, 32, 36, 40, 44, 48, 56, 68,
                                        48, 56, 64, 72, 80, 88, 104, 128,
                                        80, 96, 112, 128, 144, 160, 192, 240,
                                        160, 192, 224, 256, 288, 320, 384, 480,
                                        320, 384, 448, 512, 576, 640, 768, 960,
                                        640, 768, 896, 1024, 1152, 1280, 1536, 1920,
                                        1280, 1536, 1792, 2048, 2304, 2560, 3072, 3840};


// PRIVATE FUNCTIONS

/*! @brief Finds the best multiplier and clockrate index for required baudrate in the I2Cx_F register (Frequency Divider Register).
 *
 *  @param multiplier I2Cx_F MULT.
 *  @param indexClockRate I2Cx_F ICR.
 *  @param desiredBaudRate The desired baud rate.
 *  @param clockSpeed The module clock speed.
 */
static void FrequencyDividerRegister(const uint32_t desiredBaudRate, const uint32_t clockSpeed)
{
  uint8_t indexMultiplier;      // MULT value to be entered into register
  uint8_t indexMultiplierTemp;  // Variable used to cycle through all possible MULT values
  uint32_t multiplier;          // Multiplier used in baud rate calculation
  uint8_t indexClockRate;       // ICR value to be entered into register
  uint8_t indexClockRateTemp;   // Variable used to cycle through all possible ICR values
  uint32_t error = 0xFFFFFFFF;  // Lowest calculated error out of all possible SCLdivider and multiplier combinations
  uint32_t calculatedBaudRate;  // Closest possible baud rate to desired baud rate

  // Cycle through all possible MULT values
  for (indexMultiplierTemp = 0, multiplier = 1 ; indexMultiplierTemp <= 0x02; indexMultiplierTemp++, multiplier *= 2)
  {
    // Cycle through all possible ICR values
    for (indexClockRateTemp = 0x00; indexClockRateTemp <= 0x3F; indexClockRateTemp++)
    {
      // Calculate baud rate with current MULT and ICR values
      calculatedBaudRate = clockSpeed/(multiplier * SCLdivider[indexClockRateTemp]);

      // If calculatedBaudRate > desiredBaudRate and error is less than previous error
      if ((calculatedBaudRate > desiredBaudRate) && (calculatedBaudRate - desiredBaudRate < error))
      {
        // Update error, MULT, and ICR
        error = calculatedBaudRate - desiredBaudRate;
        indexMultiplier = indexMultiplierTemp;
        indexClockRate = indexClockRateTemp;
      }

      // If desiredBaudRate > calculatedBaudRate and error is less than previous error
      else if ((calculatedBaudRate < desiredBaudRate) && (desiredBaudRate - calculatedBaudRate < error))
      {
        // Update error, MULT, and ICR
        error = desiredBaudRate - calculatedBaudRate;
        indexMultiplier = indexMultiplierTemp;
        indexClockRate = indexClockRateTemp;
      }

      // If error is greater than previous error
      else
      {
        continue;
      }
    }
  }

  // Write obtained ICR and MULT to Frequency Divider Register
  I2C0_F = indexClockRate;
  I2C0_F |= I2C_F_MULT(indexMultiplier);
}

/*! @brief Waits for STOP signal if the bus is busy.
 *
 */
static void WaitForIdleBus(void)
{
  while (I2C0_S & I2C_S_BUSY_MASK);
}

/*! @brief Waits for AK from slave.
 *
 */
static void WaitForAK(void)
{
  while (!(I2C0_S & I2C_S_IICIF_MASK));  // Wait for interrupt flag to be set
  I2C0_S |= I2C_S_IICIF_MASK;            // w1c interrupt flag
}

/*! @brief Sends START signal to the bus.
 *
 */
static void Start(void)
{
  I2C0_C1 |= I2C_C1_MST_MASK;  // Enable master mode to generate start signal
  I2C0_C1 |= I2C_C1_TX_MASK;   // Enable transmit mode
}

/*! @brief Sends STOP signal to the bus.
 *
 */
static void Stop(void)
{
  I2C0_C1 &= ~I2C_C1_MST_MASK;  // Enable slave mode to generate stop signal
  I2C0_C1 &= ~I2C_C1_TX_MASK;   // Enable receive mode
}


// PUBLIC FUNCTIONS

/*! @brief Sets up the I2C before first use.
 *
 *  @param aI2CModule is a structure containing the operating conditions for the module.
 *  @param moduleClk The module clock in Hz.
 *  @return BOOL - TRUE if the I2C module was successfully initialized.
 */
bool I2C_Init(const TI2CModule* const aI2CModule, const uint32_t moduleClk)
{
  // Assign callback function and arguments to private global variables
  ReadCompleteCallbackFunction = aI2CModule->readCompleteCallbackArguments;
  ReadCompleteCallbackArguments = aI2CModule->readCompleteCallbackArguments;

  // Enable I2C and PORT E clocks
  SIM_SCGC4 |= SIM_SCGC4_IIC0_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;

  // Multiplex PORT E pins 18 and 19 to I2C0_SDA and I2C0_SCL alts (PORT E alts on page 280 of K70 manual)
  PORTE_PCR18 |= PORT_PCR_MUX(4);
  PORTE_PCR19 |= PORT_PCR_MUX(4);

  // Enable Open Drain configuration for PTE18 and PTE19
  PORTE_PCR18 |= PORT_PCR_ODE_MASK;
  PORTE_PCR19 |= PORT_PCR_ODE_MASK;

  // Get best MULT and ICR values for desired baud rate and write to Frequency Divider Register with acquired values
  FrequencyDividerRegister(aI2CModule->baudRate, moduleClk);

  // Select slave device to be talked to by I2C module
  I2C_SelectSlaveDevice(aI2CModule->primarySlaveAddress);

  // NVIC I2C0: Vector=40   IRQ=24   non-IPR=0   IPR=6   24%32=24
  NVICICPR0 = (1 << 24);   // Clear pending interrupts
  NVICISER0 = (1 << 24);   // Enable interrupts

  // Enable I2C module
  I2C0_C1 |= I2C_C1_IICEN_MASK;

  return true;
}

/*! @brief Selects the current slave device
 *
 * @param slaveAddress The slave device address.
 */
void I2C_SelectSlaveDevice(const uint8_t slaveAddress)
{
  // Store slave address in private global variable
  SlaveAddress = slaveAddress;
}

/*! @brief Write a byte of data to a specified register
 *
 * @param registerAddress The register address.
 * @param data The 8-bit data to write.
 */
void I2C_Write(const uint8_t registerAddress, const uint8_t data)
{
  EnterCritical();

  // Follow Figure 11 in MMA8451Q data sheet

  WaitForIdleBus();                      // Wait for stop signal if bus is busy

  Start();                               // Send START signal

  I2C0_D = (SlaveAddress << 1) & 0xFE;   // Write slave address to data register with W bit
  WaitForAK();                           // Wait for AK from slave

  I2C0_D = registerAddress;              // Write register address to data register
  WaitForAK();                           // Wait for AK from slave

  I2C0_D = data;                         // Write data to data register
  WaitForAK();                           // Wait for AK from slave

  Stop();                                // Send STOP signal

  ExitCritical();
}

/*! @brief Reads data of a specified length starting from a specified register
 *
 * Uses polling as the method of data reception.
 * @param registerAddress The register address.
 * @param data A pointer to store the bytes that are read.
 * @param nbBytes The number of bytes to read.
 */
void I2C_PollRead(const uint8_t registerAddress, uint8_t* const data, const uint8_t nbBytes)
{
  uint8_t dummy, i;                     // Local variables

  I2C0_C1 &= ~I2C_C1_IICIE_MASK;        // Disable interrupts

  EnterCritical();

  // Follow Figure 11 in MMA8451Q data sheet

  WaitForIdleBus();                     // Wait for stop signal if bus is busy

  Start();                              // Send START signal

  I2C0_D = (SlaveAddress << 1) & 0xFE;  // Write slave address to data register with W bit
  WaitForAK();                          // Wait for AK from slave

  I2C0_D = registerAddress;             // Write register address to data register
  WaitForAK();                          // Wait for AK from slave

  I2C0_C1 |= I2C_C1_RSTA_MASK;          // Generate Repeat START signal
  I2C0_D = (SlaveAddress << 1) | 0x01;  // Write slave address to data register with R bit
  WaitForAK();                          // Wait for AK from slave
  I2C0_C1 &= ~I2C_C1_TX_MASK;           // Enable receive mode

  if (nbBytes == 1)
  {
    I2C0_C1 |= I2C_C1_TXAK_MASK;        // When in master receive mode and there is only 1 byte to be received, TXACK should be set before dummy read
    dummy = I2C0_D;                     // Store second last byte in data register into data
    WaitForAK();                        // Send AK to slave
    Stop();                             // Send STOP signal
    data[nbBytes - 1] = I2C0_D;         // Store last byte in data register in data
  }

  else if (nbBytes > 1)
  {
    I2C0_C1 &= ~I2C_C1_TXAK_MASK;       // Clear TXAK for AK
    dummy = I2C0_D;                     // Read from data register to initiate receiving of next byte
    WaitForAK();                        // Send AK to slave

    // Read each byte, except last 2
    for (i = 0; i < nbBytes - 2; i++)
    {
      data[i] = I2C0_D;                  // Store byte in data register into data
      WaitForAK();                       // Send AK to slave
    }

    I2C0_C1 |= I2C_C1_TXAK_MASK;         // Set TXAK bit to send NAK bit to slave after next read
    data[nbBytes - 2] = I2C0_D;          // Store second last byte in data register into data
    WaitForAK();                         // Send AK to slave
    Stop();                              // Send STOP signal
    data[nbBytes - 1] = I2C0_D;          // Store last byte in data register in data
  }

  ExitCritical();
}

/*! @brief Reads data of a specified length starting from a specified register
 *
 * Uses interrupts as the method of data reception.
 * @param registerAddress The register address.
 * @param data A pointer to store the bytes that are read.
 * @param nbBytes The number of bytes to read.
 */
void I2C_IntRead(const uint8_t registerAddress, uint8_t* const data, const uint8_t nbBytes)
{
  // Follow Figure 11 in MMA8451Q data sheet (Just do address cycling and have first byte ready to read)

  // Local variables
  RegisterAddress = registerAddress;        // Store registerAddress in private global variable
  NbBytes = nbBytes;                        // Store nbBytes in private global variable which keeps track of current byte being read
  MaxNbBytes = nbBytes;                     // Store nbBytes in private global variable which keeps track of total number of bytes being read
  Data = data;                              // Store data in private global variable
  uint8_t dummy;                            // Dummy variable for dummy read

  I2C0_C1 |= I2C_C1_IICIE_MASK;             // Enable interrupts

  EnterCritical();

  switch (I2CIntReadStage)
  {
    case (STAGE_START):
      WaitForIdleBus();                     // Wait for stop signal if bus is busy
      Start();                              // Send START signal
      I2C0_D = (SlaveAddress << 1) & 0xFE;  // Write slave address to data register with W bit
      break;

    case (STAGE_REGISTER_ADDRESS):
      I2C0_D = registerAddress;             // Write register address to data register
      break;

    case (STAGE_REPEAT_START):
      I2C0_C1 |= I2C_C1_RSTA_MASK;          // Generate Repeat START signal
      I2C0_D = (SlaveAddress << 1) | 0x01;  // Write slave address to data register with R bit
      break;

    case (STAGE_RECEIVE_MODE):
      I2C0_C1 &= ~I2C_C1_TX_MASK;           // Enable receive mode
      break;

    case (STAGE_DUMMY_READ):
      if (MaxNbBytes == 1)                  // If single byte read
      {
	I2C0_C1 |= I2C_C1_TXAK_MASK;        // When in master receive mode and there is only 1 byte to be received, TXACK should be set before dummy read
      }
      else if (MaxNbBytes > 1)
      {
        I2C0_C1 &= ~I2C_C1_TXAK_MASK;       // Clear TXAK for AK
      }
      dummy = I2C0_D;                       // Store second last byte in data register into data
      break;

    case (STAGE_READ_BYTE):
      Data[MaxNbBytes - NbBytes] = I2C0_D;  // Store byte in data register into data
      break;

    case (STAGE_READ_SECOND_LAST_BYTE):
      I2C0_C1 |= I2C_C1_TXAK_MASK;          // Set TXAK bit to send NAK bit to slave after next read
      Data[MaxNbBytes - 2] = I2C0_D;        // Store second last byte in data register into data

    case (STAGE_READ_LAST_BYTE):
      Stop();                               // Send STOP signal
      Data[MaxNbBytes - 1] = I2C0_D;        // Store last byte in data register in data
  }

  ExitCritical();
}

/*! @brief Interrupt service routine for the I2C.
 *
 *  Only used for reading data.
 *  At the end of reception, the user callback function will be called.
 *  @note Assumes the I2C module has been initialized.
 */
void __attribute__ ((interrupt)) I2C_ISR(void)
{
  I2C0_S |= I2C_S_IICIF_MASK;                         // w1c interrupt flag

  if (!(I2C0_S & I2C_S_TCF_MASK))                     // Return from ISR if TCF is cleared (transfer is in progress)
    return;

  switch (I2CIntReadStage)
  {
    case (STAGE_START):
      I2CIntReadStage++;                              // Next stage
      I2C_IntRead(RegisterAddress, Data, NbBytes);    // Call IntRead
      break;

    case (STAGE_REGISTER_ADDRESS):
      I2CIntReadStage++;                              // Next stage
      I2C_IntRead(RegisterAddress, Data, NbBytes);    // Call IntRead
      break;

    case (STAGE_REPEAT_START):
      I2CIntReadStage++;                              // Next stage
      I2C_IntRead(RegisterAddress, Data, NbBytes);    // Call IntRead
      break;

    case (STAGE_RECEIVE_MODE):
      I2CIntReadStage++;                              // Next stage
      I2C_IntRead(RegisterAddress, Data, NbBytes);    // Call IntRead
      break;

    case (STAGE_DUMMY_READ):
      if (MaxNbBytes == 1)                            // If single byte read
      {
        I2CIntReadStage = STAGE_READ_LAST_BYTE;       // Go to final stage
        I2C_IntRead(RegisterAddress, Data, NbBytes);  // Call IntRead
      }
      I2CIntReadStage++;                              // Next stage
      I2C_IntRead(RegisterAddress, Data, NbBytes);    // Call IntRead
      break;

    case (STAGE_READ_BYTE):
      NbBytes--;                                      // Decrement NbBytes
      if (NbBytes == 2)
      {
        I2CIntReadStage++;                            // Next stage
      }
      I2C_IntRead(RegisterAddress, Data, NbBytes);    // Call IntRead
      break;

    case (STAGE_READ_SECOND_LAST_BYTE):
      NbBytes--;                                      // Decrement NbBytes
      I2CIntReadStage++;                              // Next stage
      I2C_IntRead(RegisterAddress, Data, NbBytes);    // Call IntRead
      break;

    case (STAGE_READ_LAST_BYTE):
      I2CIntReadStage = 0;                            // Reset to first stage

      // Call ReadComplete callback function
      if (ReadCompleteCallbackFunction)
      {
	(*ReadCompleteCallbackFunction)(ReadCompleteCallbackArguments);
      }
      break;
  }
}

/*!
** @}
*/
