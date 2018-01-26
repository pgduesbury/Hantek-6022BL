/*
  dso.h: structuresand dinitions specific to the Hantek 6022 'scope.

  Copyright (C) 2018 P G Duesbury: based on Open6022-QT by G Carmix.

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

#ifndef DSO_H
#define DSO_H

#include "HT6022.h"


typedef enum DSO_TDIV
{
  TDIV_20NS,
  TDIV_50NS,
  TDIV_100NS,
  TDIV_200NS,
  TDIV_500NS,
  TDIV_1US,
  TDIV_2US,
  TDIV_5US,
  TDIV_10US,
  TDIV_20US,
  TDIV_50US,
  TDIV_100US,
  TDIV_200US,
  TDIV_500US,
  TDIV_1MS,
  TDIV_2MS,
  TDIV_5MS,
  TDIV_10MS,
  TDIV_20MS,
  TDIV_50MS,
  TDIV_100MS
} DSO_TDIV;

typedef enum
{
  AUTO,
  NORMAL,
  SINGLE,
  HOLD
} DSO_MODE_TypeDef;

typedef enum
{
  STOP,
  RUN
} DSO_STATUS_TypeDef;


typedef struct
{
  HT6022_SRTypeDef SR ;
  double Tdiv;
  double Ts;
  int SubSample;
  HT6022_DataSizeTypeDef MemDepth;
} ComboSampleTypeDef;

typedef struct DSO_SET
{
  DSO_STATUS_TypeDef Status;
  DSO_MODE_TypeDef Mode;
  int ChTrigger;
  int ChAdd;
  int TriggerDelay;
  int SubSample;
  int DisplayDepth;
  int MemDepth;
  int Pos;
  double Ts;                       // as displayed; takes account of subsampling
  double Tdiv;
  double VTrigger;
  double TriggerOffset; // offset between trigger delay display and sampled data
} DSO_SET;

typedef struct DSO_CHANNEL
{
  double VScale;
  double Vdiv;
  double VOffset;
  double Zero;
  int16_t index;
  HT6022_IRTypeDef VRange;
  bool Enabled;
  bool Inv;
  bool Glitch;
} DSO_CHANNEL;


extern HT6022_DeviceTypeDef Device;
extern DSO_CHANNEL Channel1, Channel2;
extern DSO_SET Dso;


#endif // DSO_H

