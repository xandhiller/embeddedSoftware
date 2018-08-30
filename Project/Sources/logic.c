/*! @file
 *
 *  @brief Functions for required logical calculations, deductions or carrying out any resulting operations that are dependent on these calculations/deductions.
 *
 *  @author 11850637 Alex Hiller
 *  @date 2018-06-27
 */
/*
 * logic.c
 *
 *  Created on: 20 Jun 2018
 *      Author: 11850637
 */

#include "brOS.h"

void Logic_RMS(const float sample[], uint8_t size, float* rms)
{
  float calc[size]; // Used to preserve sample.
  // Assign and square
  for (uint8_t i = 0; i < size; i++)
    {
      calc[i] = sample[i];
      calc[i] = pow(calc[i], 2);
    }
  // Take the mean
  for (uint8_t i = 0; i < size; i++)
    {
      *rms += calc[i];
    }
  *rms = (*rms) / ((float)size);
  //  Sqrt value
  *rms = sqrt(*rms);
}

void Logic_IntRMS(const uint16_t sample[], uint8_t size, uint16_t* rms)
{
  uint16_t calc[size]; // Used to preserve sample.
  // Assign and square
  for (uint8_t i = 0; i < size; i++)
    {
      calc[i] = sample[i];
      calc[i] = pow(calc[i], 2);
    }
  // Take the mean
  for (uint8_t i = 0; i < size; i++)
    {
      *rms += calc[i];
    }
  *rms = (uint16_t)((*rms)/((float)size));
  //  Sqrt value
  *rms = (uint16_t)(sqrt((float)(*rms)));
}

bool Logic_SampleInit(TData * const aData)
{
  aData->Counter          = 0;
  aData->RmsVal           = 0;
  aData->VoltDev          = 0;
  aData->ZeroCrossingIndex[0] = 0;
  aData->ZeroCrossingIndex[1] = 0;
  return true;
}

bool Logic_ZeroCrossings(const float sample[], uint8_t size, uint8_t crossing[])
{
  // Zero the crossing indices to see if any crossings were found.
  crossing[0] = 0;
  crossing[1] = 0;

  // Get the index before the first zero crossing
  for (uint8_t i = 0; i < size; i++)
  {
    if ( ((sample[i]>0) && (sample[i+1]<0)) ||  ((sample[i]<0) && (sample[i+1]>0)) )
    {
      crossing[0] = i;
      break;
    }
  }
  // Get the index before the second zero crossing.
  for (uint8_t i = crossing[0]+2; i < size; i++)
  {
    if ( ((sample[i]>0) && (sample[i+1]<0)) ||  ((sample[i]<0) && (sample[i+1]>0)) )
    {
      crossing[1] = i;
      break;
    }
  }
  if ((crossing[0] == 0) && (crossing[1] == 0))
    return false;
  else
    return true;
}

void Logic_FrequencyTracking(void)
{
  // Get zero crossing indices
  if (!Logic_ZeroCrossings(PhaseAVolt, NB_SAMPLES, PhaseA.ZeroCrossingIndex))
    {
      return;
    }
  float a1 = PhaseAVolt[PhaseA.ZeroCrossingIndex[0]];
  float b1 = PhaseAVolt[PhaseA.ZeroCrossingIndex[0]+1];
  float a2 = PhaseAVolt[PhaseA.ZeroCrossingIndex[1]];
  float b2 = PhaseAVolt[PhaseA.ZeroCrossingIndex[1]+1];
  float Ts = AlarmStopWatch.WaveformSampleNs * 1e-9; // Sample time in seconds
  float t1 = -a1*Ts/(b1-a1);
  float t2 = -a2*Ts/(b2-a2);
  uint8_t n = PhaseA.ZeroCrossingIndex[1]-PhaseA.ZeroCrossingIndex[0]; // a2's index - a1's index
  float WavePeriod = (n*Ts-t1+t2)*2;
  // If the period of the wave indicates it's within 45hz-55Hz, then adjust the sample time and saved freq.
  if ( ((uint32_t)(WavePeriod/(16.0)*1e9) > 1136400 ) && ((uint32_t)(WavePeriod/(16.0)*1e9) < 1388900 ))
  {
    // Save sampling time in ns into global struct for accessibility
    AlarmStopWatch.WaveformSampleNs = (uint32_t)(WavePeriod/(16.0)*1e9);
    // Save frequency for communication with PC
    AlarmStopWatch.WaveformFrequency = (float)1.0/(float)WavePeriod;
    // Set new Waveform sampling rate
  }
  PIT_Set(AlarmStopWatch.WaveformSampleNs, true, SAMPLING_CHANNEL);
}

void Logic_OutputRaise(bool status)
{
  if (status == ON)
    {
      Analog_Put(RAISE_CHANNEL, VOLT_SIGNAL);
      Flash_NbRaises();
    }
  if (status == OFF)
    {
      Analog_Put(RAISE_CHANNEL, VOLT_IDLE);
    }
}

void Logic_OutputLower(bool status)
{
  if (status == ON)
    {
      Analog_Put(LOWER_CHANNEL, VOLT_SIGNAL);
      Flash_NbLowers();
    }
  if (status == OFF)
    {
      Analog_Put(LOWER_CHANNEL, VOLT_IDLE);
    }
}

void Logic_OutputAlarm(bool status)
{
  if (status == ON)
    {
      Analog_Put(ALARM_CHANNEL, VOLT_SIGNAL);
    }
  if (status == OFF)
    {
      Analog_Put(ALARM_CHANNEL, VOLT_IDLE);
    }
  }

bool Logic_AlarmStopWatchInit(TAlarmStopWatch * const aAlarmStopWatch)
{
  aAlarmStopWatch->SetLevel           = 0;
  aAlarmStopWatch->StopWatch          = 0;
  aAlarmStopWatch->StopWatchStop      = 0;
  aAlarmStopWatch->TimingType         = DEF_TIMING; // Sets up definite timing to begin with.
  aAlarmStopWatch->WaveformSampleNs   = INITIAL_SAMPLE_T;
  aAlarmStopWatch->WaveformFrequency  = 50;
  return true;
}

void Logic_DefiniteAlarm(uint8_t setting)
{
  AlarmStopWatch.SetLevel       = setting;
  AlarmStopWatch.StopWatch      = 0;
  AlarmStopWatch.StopWatchStop  = MILLISEC_TO_INT(5000);
}

void Logic_InverseAlarm(uint8_t setting)
{
  AlarmStopWatch.SetLevel = setting;
  // FIXME: Just using Phase A for this calculation for now. This correct?

  // If no alarm going
  if (AlarmStopWatch.SetLevel != NOT_SET)
  {
      AlarmStopWatch.StopWatchStop = MILLISEC_TO_INT(1000*0.5*5.0/(PhaseA.VoltDev));
//      AlarmStopWatch.StopWatch = RESET;
  }
  // Else must be some time on stopwatch
  else
  {
    AlarmStopWatch.StopWatchStop = MILLISEC_TO_INT(1000*(0.5*5.0/(PhaseA.VoltDev))*(1.0 - (float)((AlarmStopWatch.StopWatch)/(AlarmStopWatch.StopWatchStop))));
    // Reset the stopwatch
//    AlarmStopWatch.StopWatch = RESET;
  }
}

uint8_t Logic_CheckRMS(void)
{
  // Check if above
  if (PhaseA.RmsVal > RMS_UPPER_LIMIT  ||
      PhaseB.RmsVal > RMS_UPPER_LIMIT  ||
      PhaseC.RmsVal > RMS_UPPER_LIMIT)
  {
    return RMS_HIGH;
  }
  // Check if below
  else if ( PhaseA.RmsVal < RMS_LOWER_LIMIT  ||
            PhaseB.RmsVal < RMS_LOWER_LIMIT  ||
            PhaseC.RmsVal < RMS_LOWER_LIMIT )
  {
    return RMS_LOW;
  }
  // Else return fine
  else
  {
    return RMS_FINE;
  }
}

void Logic_VoltageDev(float rms, float* vDev)
{
  if (rms >= (float)RMS_UPPER_LIMIT)
  {
    *vDev = (float)RMS_NOMINAL - rms;
    if ( (*vDev) < 0)
    {
      *vDev = (*vDev)*(-1);
    }
  }

  if (rms <= (float)RMS_LOWER_LIMIT)
  {
    *vDev = (float)RMS_NOMINAL - rms;
    if ( (*vDev) < 0)
    {
      *vDev = (*vDev)*(-1);
    }
  }
}

void Logic_Separate(float complex * a, uint8_t n)
{
  float complex b[n/2];
  for(int i=0; i<n/2; i++)    // copy all odd elements to heap storage
      b[i] = a[i*2+1];
  for(int i=0; i<n/2; i++)    // copy all even elements to lower-half of a[]
      a[i] = a[i*2];
  for(int i=0; i<n/2; i++)    // copy all odd (from heap) to upper-half of a[]
      a[i+n/2] = b[i];
}

void Logic_Fft(float complex * X, uint8_t N)

{
  if (N < 2)
  {
      // Do nothing
  }
  else
  {
    // Separates X into even elements on the lower half, odd elements on the upper half.
    Logic_Separate(X,N);
    // Recursive use of the function
    Logic_Fft(X, N/2);
    // Add N/2 to each element of what will become the frequency array
    for (uint8_t i = 0; i<N; i++)
    {
      X[i] += N/2;
    }
    // Run fft recursively again with new values.
    Logic_Fft(X, N/2);
    // Combine the results of the two half recursions
    for (uint8_t i = 0; i<N/2; i++)
      {
        float complex e = X[i];
        float complex o = X[i+N/2];
        float complex w = cexpf((float complex)(0, -2.0*PI*i/N));
        X[i] = e + w * o;
        X[i+N/2] = e - w * o;
      }
  }
}
