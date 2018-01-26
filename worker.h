/*
  worker.h

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


#ifndef WORKER_H
#define WORKER_H
#include <QThread>
#include "HT6022.h"
#include "dso.h"

class workerThread : public QThread
{
    Q_OBJECT
public:
    unsigned char* CH0;
    int TriggerPoint;                       // in terms of sample index position
    int TriggerEdge;                        // 0 (falling edge), 1 (rising edge)
    int TriggerChannel;                          // 0 (Channel 1), 1 (Channel 2)
    int holdoff;               // delay in ms between sucessive data acquisition
    int alive;                                         // for thread termination
    DSO_MODE_TypeDef mode;                         // AUTO, NORMAL, SINGLE, HOLD
    unsigned char TriggerLevel;                                       // 0 - 255
signals:
    void dataReady();
private:
    unsigned char CHA[1024*1024*2];              // double buffer wavefom traces
    unsigned char CHB[1024*1024*2];
    unsigned char* CHX;           // last buffer filled and available to be read
    void run();
};

#endif                                                               // WORKER_H
