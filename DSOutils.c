/*
  DSOutils.c: functions to read and write files in the home directory and
  to format timing for display on delay and holdoff controls.

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


  06/01/18  First draft
*/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <math.h>
#include <stdbool.h>
#include "dso.h"
#include "DSOutils.h"


#ifdef __cplusplus
 extern "C" {
#endif


double Zero1[6] = {-1, -1, 0, 1, 2, 2}; //offset null corrections, -127 to + 127
double Zero2[6] = {9, 9, 10, 11, 12, 12};
double VScaleFactor = 1.00;                     // default correction for 'scope


static int get_home_path(char* path, const char* filename, int n)      // find ~
{
  int k;
  int m;
  const char *homedir;

  path[0] = 0;                                        // initialise empty string

  if((homedir = getenv("HOME")) == NULL)
    homedir = getpwuid(getuid())->pw_dir;                            // fallback

  k = strlen(filename);
  m = strlen(homedir) + k;
  if(m >= n) return 0;                                                   // fail

  strcpy(path, homedir);
  strncat(path, filename, k);
  return m;                                             // length of path string
}


int write_cal_file(void)                           // writes offsets to ~/.scope
{
  int i;
  FILE * datafile;
  char path[128];

  get_home_path(path, "/.scope", 128);
  datafile = fopen(path,"wt");
  if(datafile)
  {
    for(i = 0; i < 6; i++)
      fprintf(datafile,"%lf, %lf, ",Zero1[i], Zero2[i]);
    fprintf(datafile,"%lf\r\n", VScaleFactor);
    fclose(datafile);
    return 1;
  }
  return 0;
}


int read_cal_file(void)                          // reads offsets from ~/.scope
{
  int i;
  FILE * datafile;
  char path[128];

  get_home_path(path, "/.scope", 128);
  datafile = fopen(path,"rt");
  if(datafile)
  {
    for(i = 0; i < 6; i++)
      if(fscanf(datafile,"%lf, %lf, ",&Zero1[i], &Zero2[i]) != 2) return 0;
    if(fscanf(datafile,"%lf\r\n",&VScaleFactor) != 1) return 0;
    fclose(datafile);
    return 1;
  }
  return 0;
}


void do_cal(unsigned char* CH0, int Calibrate)   // modifies Zero1[] and Zero2[]
{
  static const HT6022_IRTypeDef VRange[6] =
    {HT6022_10V, HT6022_10V, HT6022_5V, HT6022_2V, HT6022_1V, HT6022_1V};

  static DSO_MODE_TypeDef Mode;

  int i;
  int offset1 = 0;
  int offset2 = 0;


  switch(Calibrate)
  {
    case 25:
      Mode = Dso.Mode;
      Dso.Mode = AUTO;                         // allow time for AUTO to kick in
      break;
    case 20:                                     // dummy scan: result discarded
    case 16:
    case 12:
    case 8:
    case 4:
      for(i = 8; i < 256+8; i++)
      {
        offset1 += CH0[i*2];
        offset2 += CH0[i*2+1];
      }
      i = Calibrate/4;
      Zero1[i] = (double)offset1 / 256.0 - 128.0;
      Zero2[i] = (double)offset2 / 256.0 - 128.0;
      i--;
      HT6022_SetCH1IR (&Device, VRange[i]);
      HT6022_SetCH2IR (&Device, VRange[i]);
      break;
    case 1:
      Zero1[5] = Zero1[4], Zero2[5] = Zero2[4]; // 2V and 1V same physical range
      Zero1[0] = Zero1[1], Zero2[0] = Zero2[1]; // 50mV and 100mV same
      write_cal_file();
      HT6022_SetCH1IR (&Device, Channel1.VRange);  // restore original range ...
      HT6022_SetCH2IR (&Device, Channel2.VRange);
      Channel1.Zero = Zero1[Channel1.index];         //... using new zero offset
      Channel2.Zero = Zero2[Channel2.index];
      Dso.Mode = Mode;                               // restore original setings
      break;
  }
}


void write_tracefile(unsigned char* CH0, int MemDepth)
{
  int i;
  FILE * datafile;
  char path[128];

  get_home_path(path, "/data.csv", 128);
  datafile = fopen(path,"wt");
  fprintf(datafile,"T(s),CH1(V),CH2(V)\r\n");
  for(i = 0; i < MemDepth; i++)
  {
    fprintf
    (
       datafile,"%lE,%5.4f,%5.4f\r\n",
       (double)i * Dso.Ts,
       Channel1.VScale * (CH0[2*i]-128.0-Channel1.Zero)/128.0,
       Channel2.VScale * (CH0[2*i+1]-128.0-Channel2.Zero)/128.0
    );
  }
  fclose(datafile);
}


void float2engStr(char* strout, double value)
{
  static const char* unit[8] = {"ps", "ns", "us", "ms", "s", "Ks", "Ms", "Gs"};
  int pos = 0;
  double neg = 1;

  if(value < 0) value = -value, neg = -1;

  while(value < 1 && pos >= -3) pos--, value *= 1000;
  while(value >= 1000 && pos <= 2) pos++, value /= 1000;
  sprintf(strout,"%3.2f%s",value * neg, unit[pos+4]);
}

#ifdef __cplusplus
    }
#endif
