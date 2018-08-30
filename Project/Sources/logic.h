/*! @file
 *
 *  @brief Functions for required logical calculations or deductions.
 *
 *  @author 11850637 Alex Hiller
 *  @date 2018-06-27
 */
/*
 * logic.h
 *
 *  Created on: 20 Jun 2018
 *      Author: 11850637
 */

#ifndef SOURCES_LOGIC_H_
#define SOURCES_LOGIC_H_

#include "brOS.h"
#include <complex.h> // Included to stop errors by use of 'complex' type.

#define NB_SAMPLES          16
#define ON                  true
#define OFF                 false
#define MILLISEC_TO_INT(X)  (uint32_t)((X)/10)
#define HIGH_RMS            (float)3.0
#define LOW_RMS             (float)2.0
#define RESET               0
#define PI                  3.14159265358979323846
// For DAC placement
#define DAC_BITS_PER_VOLT         3276.7
#define DAC_VOLTS(X)              (int16_t)( ((float)DAC_BITS_PER_VOLT) * ((float)X) )
#define ADC_VOLTS(X)              (float)X/(float)DAC_BITS_PER_VOLT
#define VOLT_SIGNAL               DAC_VOLTS(5)
#define VOLT_IDLE                 DAC_VOLTS(0)
#define RMS_UPPER_LIMIT           3
#define RMS_LOWER_LIMIT           2
#define RMS_NOMINAL               2.5
#define INITIAL_SAMPLE_T          1250000 // Nanoseconds per sample period to acquire 16 samples for 50Hz signal
#define NB_PHASES                 3 // On input side
#define RAISE_CHANNEL             1 // On output side
#define LOWER_CHANNEL             2 // On output side
#define ALARM_CHANNEL             3 // On output side
#define DEBUGGING_CHANNEL         0
#define CHANNEL_A                 1 // On input side
#define CHANNEL_B                 2 // On input side
#define CHANNEL_C                 3 // On input side

enum
{
  NOT_SET,
  SET_LOW,
  SET_HIGH,
};

enum
{
  DEF_TIMING,
  INV_TIMING,
};

enum
{
  RMS_FINE,
  RMS_LOW,
  RMS_HIGH,
};

/*!
 * @struct TSample
 */
typedef struct
{
  int16_t   ADCBuffer[NB_SAMPLES];    /*!< The actual array of bytes to store the data */
  uint8_t   Counter;                  /*!< Counter to keep track of how many samples are new */
  float     FloatBuffer[NB_SAMPLES];
  float     RmsVal;
  float     VoltDev;
  uint8_t   ZeroCrossingIndex[2];
  volatile uint16union_t *NvNbLowers;  // Non-volatile number of lower taps
  volatile uint16union_t *NvNbRaises;  // Non-volatile number of raise taps
} TData;

typedef struct
{
  uint32_t StopWatch;
  uint32_t StopWatchStop;
  uint8_t SetLevel;
  uint8_t TimingType;
  uint32_t WaveformSampleNs;
  float WaveformFrequency;
} TAlarmStopWatch;

/*! @brief Takes an array of samples and returns a calculated float RMS
 *
 *  @param sample[] is an array of ints taken from the ADC
 *  @param size is the size of sample[].
 *  @param rms is a pointer to a float to hold the calculated rms value
 */
void Logic_RMS(const float sample[], uint8_t size, float* rms);

/*! @brief Takes in a waveform sample buffer struct and initialises its variables and semaphores
 *
 *  @param aSample is the struct to be initialised.
 */
bool Logic_SampleInit(TData * const aData);


/*! @brief Takes an array of samples and returns the period of that waveform
 *
 *  @param sample[] is an array of ints taken from the ADC
 *  @param size is the size of sample[].
 *  @param currentPeriod is the current sample period in seconds.
 *  @param calcPeriod is the calculated period in seconds.
 *  @note Assumes that between two points on the zero crossing that there is a straight
 *  line.
 */
void Logic_FrequencyTracking(void);


/*! @brief Takes an array of y-values and returns the indices right before the first two zero-crossings.
 *
 *  Assumes that between two points on the zero crossing that there is a straight
 *  line.
 *  @param sample[] is an array of floats converted from the ADC.
 *  @param size is the size of sample[].
 *  @param crossing[] is an array you want to place the two indices into.
 *  @return true if crossings were found, false if not.
 *  @note Because we are regulating a transformer, there is no DC component that can
 *  get through, so finding the 'crossing of the average' was not performed. If there was
 *  reason to expect a DC component to the signal, this function would need to be changed.
 */
bool Logic_ZeroCrossings(const float sample[], uint8_t size, uint8_t crossing[]);

/*! @brief Changes the status of the 'Raise' tap based on function input.
 *
 *  @param status indicates to either turn on or off the 'Raise' tap.
 */
void Logic_OutputRaise(bool status);

/*! @brief Changes the status of the 'Lower' tap based on function input.
 *
 *  @param status indicates to either turn on or off the 'Lower' tap.
 */
void Logic_OutputLower(bool status);

/*! @brief Changes the status of the 'Alarm' output based on function input.
 *
 *  @param status indicates to either turn on or off the 'Alarm' tap.
 */
void Logic_OutputAlarm(bool status);

/*! @brief Initialises the AlarmStopWatch object for alarm timing.
 *
 *  @param aAlarmStopWatch a TAlarmStopWatch object to be initialised.
 */
bool Logic_AlarmStopWatchInit(TAlarmStopWatch * const aAlarmStopWatch);

/*! @brief Checks the RMSvals in the global data struct if they exceed limits
 *
 *  @return RMS_FINE = 0, RMS_LOW = 1, RMS_HIGH = 2
 *  @note Limits are specified by RMS_UPPER_LIMIT and RMS_LOWER_LIMIT in logic.h
 */
uint8_t Logic_CheckRMS(void);

/*! @brief Sets an alarm in Definite Timing mode.
 *
 *  @param setting specifies whether it will turn the output on or off.
 *  @note Outputs put to DAC stay that way until re-initialised.
 */
void Logic_DefiniteAlarm(uint8_t setting);

/*! @brief Sets an alarm in Inverse Timing mode.
 *
 *  @param setting specifies whether it will turn the output on or off.
 *  @note Outputs put to DAC stay that way until re-initialised.
*/
void Logic_InverseAlarm(uint8_t setting);

/*! @brief Calculates the voltage deviation from the specified nominal voltage
 *
 *  @param rms is a value of a phase who's voltage deviation is to be calculated.
 *  @param vDev is a pointer to a place to store the calculated value.
 *  @note RMS is always a positive number.
 */
void Logic_VoltageDev(float rms, float* vDev);

/*! @brief Takes in a complex array and sorts the upper and lower half into even, upper half is odd.
 *
 *  @param a is a complex float array pointer.
 *  @param n is the size of the array, a.
 */
void Logic_Separate(float complex* a, uint8_t n);

/*! @brief Performs an fft on a complex float array.
 *
 *  @param X is an array of copied time-domain voltage data.
 *  @param N is the number of elements in X. Has to be a power of 2.
 *  @note The result of the calculation will be stored in the given 'float complex' array, X.
 *  @note N should be a power of 2 or "bad things will happen".
 */
void Logic_Fft(float complex* X, uint8_t N);

#endif /* SOURCES_LOGIC_H_ */
