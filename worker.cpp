/*
  worker.cpp: Background raw waveform trace acquisition thread.

  Copyright (C) 2018 P G Duesbury: Based on Open6022-QT by G Carmix.

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


  06/01/18  First draft
*/


#include <stdbool.h>
#include "worker.h"
#include "HT6022.h"
#include "dso.h"

extern HT6022_DeviceTypeDef Device;                             // Hantek 'scope

void workerThread::run()
{
  int i, j;
  int Depth;                                     // size of raw interleaved data
  int tp;                         // temporary trigger point, zero if none found
  unsigned char level;           // offset from Trigger Level for noise immunity

  CH0 = CHA;                                        // initalise buffer pointers
  CHX = CHB;                                       // traces are double buffered

  while(alive)
  {
    Depth = Dso.MemDepth * 2;    // raw data is byte pairs of alternate channels

    if(Dso.MemDepth == HT6022_1KB) j = 32;  // aggressive search for trigger ...
    else j = 1;                            // ... not necesary with long buffers
    tp = 0;                                  // default if no trigger edge found

    for(;j;j--)
    {
      if
      (
        HT6022_ReadData
        (
          &Device,
          CHX,
          (HT6022_DataSizeTypeDef)Dso.MemDepth,
          0
        ) == HT6022_SUCCESS
      )
      {
        i = 16 + TriggerChannel;       // less than 10 leads to trigger problems
        if(TriggerEdge)                                           // rising edge
        {
          level = TriggerLevel > 4 ? TriggerLevel - 4 : 0;
          for(; i < Depth; i+=2) if(CHX[i] < level) break;
          for(; i < Depth; i+=2) if(CHX[i] >= TriggerLevel) break;
        }
        else                                                     // falling edge
        {
          level = TriggerLevel < 255-4 ? TriggerLevel + 4 : 255;
          for(; i < Depth; i+=2) if(CHX[i] > level) break;
          for(; i < Depth; i+=2) if(CHX[i] <= TriggerLevel) break;
        }

        if(i < Depth)                                      // trigger edge found
        {
          tp = i/2 - 1;                 // sample index just before trigger edge
          break;
        }
      }
    }
    if((tp && mode != HOLD) || mode == AUTO)            // free run in AUTO mode
    {
      if(CHX == CHA) CHX = CHB, CH0 = CHA;                       // swap buffers
      else CHX = CHA, CH0 = CHB;    // allows concurrent acquisition and display
      TriggerPoint = tp;       // keep trigger point with corresponding data set
      if(mode == SINGLE) mode = HOLD;
    }
    emit dataReady();                                   // signal display update
    msleep(holdoff);            // holdoff for display update on single core CPU
  }
}

