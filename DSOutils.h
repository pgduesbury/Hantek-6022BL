/*
  DSOutils.h: function prototypes for the 6022 'scope.

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
*/


#ifndef DSOUTILS_H
#define DSOUTILS_H


#include "HT6022.h"


#ifdef __cplusplus
 extern "C" {
#endif

extern double Zero1[6];                 //offset null corrections, -127 to + 127
extern double Zero2[6];
extern double VScaleFactor;

extern void do_cal(unsigned char* CH0, int Calibrate);
extern int write_cal_file(void);
extern int read_cal_file(void);
extern void write_tracefile(unsigned char* CH0, int MemDepth);
extern void float2engStr(char* strout, double value);

#ifdef __cplusplus
    }
#endif

#endif // DSOUTILS_H

