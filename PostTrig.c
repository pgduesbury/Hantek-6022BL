/*
  PostTrig.c: Read and format the waveform traces from the 6022 'scope.

  Copyright (C) 2018 P G Duesbury

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.


  06/01/2018  Display of post trigger data so limited pre-trigger support
*/

#include <stdbool.h>
#include "HT6022.h"
#include "dso.h"
#include "PostTrig.h"


extern DSO_SET Dso;

#ifdef __cplusplus
 extern "C" {
#endif

#define TRIG_WIN 50

static const int fir[10][5] = // convolution kernel for 5 fold sin(x)/x upsample
{
  {      0,     626,    1432,    1942,    1579 },
  {      0,   -2556,   -5136,   -6299,   -4726 },
  {      0,    6803,   13096,   15526,   11354 },
  {      0,  -15926,  -30697,  -36878,  -27753 },
  {      0,   44430,   98040,  149440,  186501 },
  { 200000,  186501,  149440,   98040,   44430 },
  {      0,  -27753,  -36878,  -30697,  -15926 },
  {      0,   11354,   15526,   13096,    6803 },
  {      0,   -4726,   -6299,   -5136,   -2556 },
  {      0,    1579,    1942,    1432,     626 }
};


static inline unsigned char minmax(unsigned char* ch, int j)
{
  // returns alternately the minimum or maximum value within two
  // consecutive sample intervals.

  static unsigned char prev;
  unsigned char min;
  unsigned char max;
  unsigned char c;
  int i;

  for(i = 0, min = 255, max = 0; i < 2 * Dso.SubSample; i += 2)
  {
    min = ch[i] < min ? ch[i] : min;
    max = ch[i] > max ? ch[i] : max;
  }
  if((j / (2 * Dso.SubSample)) & 1) c = min < prev ? min : prev, prev = max;
  else c = max > prev ? max : prev, prev = min;
  return c;
}


static int scan                // de-interleave the traces in the raw CH0 buffer
(
  unsigned char* CH,                                    // output waveform trace
  unsigned char* CH0,
  int triggerIdx,                               // initial trigger edge position
  int channel,                                                         // 0 or 1
  bool Glitch,   // invokes minmax mode to display short pulses on slow timebase
  int SzDispBuf                                            // Display bufer size
)
{
  int i;
  int j;
  int offset = 0;
  int SubSample = Dso.SubSample;   // number of actual samples per display point

  if(triggerIdx < 8)                      // triggerIdx is zero if no edge found
  {
    offset = 8+5 - triggerIdx;         // first 5 samples may be rubbish so skip
    triggerIdx = 8+5;
  }

  i = (triggerIdx - 8) * 2 + channel;
  j = offset / SubSample;                      // possible truncation error here

  if(j >= SzDispBuf) return 0;

  if(Dso.MemDepth - triggerIdx < SzDispBuf * SubSample)
    SzDispBuf = (Dso.MemDepth - triggerIdx) / SubSample;

  if(SubSample == 1)                    // special case for no decimation: speed
    for(; j < SzDispBuf; i += 2, j++) CH[j] = CH0[i];

  else if(Glitch)           // minmax mode to display sub sample interval pulses
    for(; j < SzDispBuf-1; i += 2*SubSample, j++) CH[j] = minmax(CH0+i, i);
  else                                                               // decimate
    for(; j < SzDispBuf; i += 2*SubSample, j++) CH[j] = CH0[i];
  return j;     // the number of samples read: varies with trigger edge position
}


static void vectorise                 // scale and if necesary upsample waveform
(
  double* y_vec,                                          // output scaled trace
  unsigned char* CH,                                         // imput trace data
  DSO_CHANNEL* Channel,                                    // scaling parameters
  int MemDepth                               // size of input and output buffers
)
{
  int i;
  int j;
  double VScale, VZero;

  VScale =  Channel->VScale / (128 * 4 * Channel->Vdiv);
  if(Channel->Inv && MemDepth != TRIG_WIN) VScale = -VScale; // positive trigger
  VZero = Channel->VOffset - (Channel->Zero + 128) * VScale;

  if(Dso.Tdiv <= 500e-9)       // Upsample using sin(x)/x as too few data points
  {
    VScale *= 5e-6;

    for(i = 0; i < (MemDepth - 5) / 5; i++)
    {
      for(j = 0; j < 5; j++)
      {
        y_vec[5 * i + 4 - j] = VZero + VScale *
        (
          CH[i+0] * fir[0][j] +
          CH[i+1] * fir[1][j] +
          CH[i+2] * fir[2][j] +
          CH[i+3] * fir[3][j] +
          CH[i+4] * fir[4][j] +
          CH[i+5] * fir[5][j] +
          CH[i+6] * fir[6][j] +
          CH[i+7] * fir[7][j] +
          CH[i+8] * fir[8][j] +
          CH[i+9] * fir[9][j]
        );
      }
    }
  }
  else
  {
    for(i = 0; i < MemDepth; i++)
      y_vec[i] = VZero + VScale * CH[i];
  }
}


static double refine_trigger      // fine trigger adjustment using interpolation
(
  double* y_vec,                                         // input waveform trace
  int TriggerEdge,                             // rising (1) or falling (0) edge
  DSO_CHANNEL* Channel                                          // scale factors
)
{
  int i;
  double tl;                                                     //trigger level

  tl = Dso.VTrigger/(4*Channel->Vdiv)+Channel->VOffset;

  if(Dso.Tdiv <= 500e-9) i = 16;                              // trace upsampled
  else i = 8 / Dso.SubSample;

  if(TriggerEdge)                                                 // rising edge
  {
    for(; i < TRIG_WIN; i++) if(y_vec[i] < tl) break;
    for(; i < TRIG_WIN; i++) if(y_vec[i] >= tl) break;
  }
  else                                                           // falling edge
  {
    for(; i < TRIG_WIN; i++) if(y_vec[i] > tl) break;
    for(; i < TRIG_WIN; i++) if(y_vec[i] <= tl) break;
  }

  if(i == TRIG_WIN) return 0;
  return i - (y_vec[i] - tl) / (y_vec[i] - y_vec[i-1]);
}


int get_post_trigger_waveforms    // Read and format waveform traces from worker
(
  double* y1_vec,                                     // QVector<double> &y1_vec
  double* y2_vec,                                     // QVector<double> &y2_vec
  double* x_vec,                                       // QVector<double> &x_vec
  unsigned char* CH0,                          // interleaved waveforms from USB
  DSO_CHANNEL* Channel1,
  DSO_CHANNEL* Channel2,
  int TriggerPoint,                             // initial trigger edge position
  int TriggerEdge                                           // rising or falling
)
{
  static unsigned char CH1[HT6022_1KB];
  static unsigned char CH2[HT6022_1KB];

  static unsigned char CH3[TRIG_WIN+8];
  static double t_vec[TRIG_WIN];        //static QVector<double>t_vec(TRIG_WIN);

  static double tp;                                             // trigger point

  int i;
  int triggerIdx;              // trigger edge position (may be offset by delay)
  int DataSize;             // number of samples actually read into trace buffer
  double Ts;

  triggerIdx = TriggerPoint;

  if(triggerIdx < 8) triggerIdx = 8;     // prevent out of bounds read in scan()

  if(Dso.TriggerDelay + triggerIdx > Dso.MemDepth) return -1;  // delay > buffer

  if(TriggerPoint)                // copy trigger edge for subsequent refinement
    scan(CH3,CH0,triggerIdx,Dso.ChTrigger==1?0:1,false,TRIG_WIN+8);

  triggerIdx += Dso.TriggerDelay;                              //delayed trigger

  DataSize = HT6022_1KB;                          // default value for AUTO mode

                            // Copy data from acquisition buffer to free this up
  if((Channel1->Enabled || Dso.ChAdd == 2) && (TriggerPoint||Dso.Mode==AUTO))
    DataSize = scan(CH1,CH0,triggerIdx,0,Channel1->Glitch,HT6022_1KB);

  if((Channel2->Enabled || Dso.ChAdd == 1) && (TriggerPoint||Dso.Mode==AUTO))
    DataSize = scan(CH2,CH0,triggerIdx,1,Channel2->Glitch,HT6022_1KB);

  // As CH1 and CH2 are not cleared between display updates, we see a composite
  // of a number of scans.  This is useful at 2us/div and below where the
  // actual trigger point may occur well into the 1K sample buffer if it occurs
  // at all.  Note that changing TriggerDelay invalidates the corespondence
  // between the timing vectors in x_vec and the contents of CH0 and CH1.

  if(Channel1->Enabled || Dso.ChAdd == 2)
    vectorise(y1_vec, CH1, Channel1, Dso.DisplayDepth);

  if(Channel2->Enabled || Dso.ChAdd == 1)
    vectorise(y2_vec, CH2, Channel2, Dso.DisplayDepth);

  if(Dso.ChAdd == 1)
    for(i = 0; i < HT6022_1KB; i++) y1_vec[i] += y2_vec[i] - Channel2->VOffset;

  if(TriggerPoint)
  {                                     // refine trigger; about 24 for sin(x)/x
    vectorise(t_vec, CH3, Dso.ChTrigger==1?Channel1:Channel2, TRIG_WIN);
    tp = refine_trigger(t_vec, TriggerEdge, Dso.ChTrigger==1?Channel1:Channel2);
  }
  else if((Dso.Mode == AUTO && Dso.Status == RUN))// || Dso.Status == STOP)
  {
    tp = 1 + 8 / Dso.SubSample;               // default position if no trigger
  }

  if(triggerIdx < 8) i = (8+5 - triggerIdx) / Dso.SubSample; //1st 5 samples bad
  else i = 0;

  if(Dso.Tdiv <= 500e-9)
    i *= 5, Ts = Dso.Ts * Dso.SubSample / 5, DataSize *= 5;
  else
    Ts = Dso.Ts * Dso.SubSample;

  DataSize = DataSize > HT6022_1KB ? HT6022_1KB : DataSize;

  for(; i < DataSize; i++) x_vec[i]=(i-tp)*Ts-Dso.TriggerOffset;//reposition
  // At 2us/div and below, x_vec can be a composite of several fractional
  // timing offsets.  This is necessary to allow the various sub traces to line
  // up correctly on screen.

  return TriggerPoint;                                                // Success
}


#ifdef __cplusplus
    }
#endif

