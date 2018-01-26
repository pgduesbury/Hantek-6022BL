/*
  mainwindow.cpp: QT user interface for the Hantek 6022BL 'scope.

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


  06/01/2018  First draft.
*/


#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "DSOutils.h"
#include "HT6022.h"
#include "worker.h"
#include "dso.h"
#include "PostTrig.h"
#include <stdio.h>
#include <unistd.h>
#include <float.h>
#include <QDebug>
#include <QElapsedTimer>



typedef enum                       // operating modes for vertical dial controls
{
  POSITION,
  GAIN
} VARMODE_TypeDef;


workerThread worker;                    // backgound waveform acquisition thread
DSO_SET Dso = {STOP,AUTO,1,0,0,1,HT6022_1KB,HT6022_1KB,0,1/16e6,1e-3,0,0};
                                                              // timing and mode


HT6022_DeviceTypeDef Device;                 // USB identifier for Hantek 'scope
DSO_CHANNEL Channel1, Channel2;          // current vertical deflection settings
VARMODE_TypeDef VarCH1 = POSITION;       // vertical dial mode: POSITION or GAIN
VARMODE_TypeDef VarCH2 = POSITION;
QElapsedTimer timer;                  // only needed during test and development
double CursorX1 = 0;                 // timing position of vertical trigger line
QCPItemLine* vCursorX1;                                 // vertical trigger line
QCPItemLine* vCursorTrigger;                          // horizontal trigger line
QVector<double> x_vec(HT6022_1KB);              // timings for each y_vec sample

QVector<double>y1_vec(HT6022_1KB);
QVector<double>y2_vec(HT6022_1KB);
int withhold = 0;              // delay switching to AUTO mode as for CRT 'scope
int Calibrate = 0;               // set to 25 to initiate ofset null calibration


ComboSampleTypeDef ComboSample[22] = //max buffer sizes for fast display refresh
{
  {HT6022_48MSa,  20e-9,  1/48e6,    1,   HT6022_1KB},
  {HT6022_48MSa,  50e-9,  1/48e6,    1,   HT6022_1KB},
  {HT6022_48MSa, 100e-9,  1/48e6,    1,   HT6022_1KB},
  {HT6022_48MSa, 200e-9,  1/48e6,    1,   HT6022_1KB},
  {HT6022_48MSa, 400e-9,  1/48e6,    1,   HT6022_1KB},  //  kludge for 1K buffer
  {HT6022_48MSa,   1e-6,  1/48e6,    1,   HT6022_1KB},
  {HT6022_48MSa,   2e-6,  1/48e6,    1,   HT6022_1KB},
  {HT6022_16MSa,   5e-6,  1/16e6,    1, HT6022_256KB},
  {HT6022_16MSa,  10e-6,  1/16e6,    2, HT6022_256KB},
  {HT6022_16MSa,  20e-6,  1/16e6,    4, HT6022_256KB},
  {HT6022_16MSa,  50e-6,  1/16e6,   10, HT6022_256KB},
  {HT6022_16MSa, 100e-6,  1/16e6,   20, HT6022_256KB},
  {HT6022_16MSa, 200e-6,  1/16e6,   40, HT6022_256KB},
  {HT6022_16MSa, 500e-6,  1/16e6,  100, HT6022_256KB}, //  16ms ack, 5ms display
  {HT6022_16MSa,   1e-3,  1/16e6,  200, HT6022_512KB}, //  32ms ack,10ms display
  {HT6022_16MSa,   2e-3,  1/16e6,  512, HT6022_512KB}, //  32ms ack,20ms display
  {HT6022_16MSa,   5e-3,  1/16e6, 1024,   HT6022_1MB}, //  64ms ack,50ms display
  {HT6022_8MSa,   10e-3,0.125e-6, 1024,   HT6022_1MB}, // 128ms ack 0.1s display
  {HT6022_4MSa,   20e-3, 0.25e-6, 1024,   HT6022_1MB}, // 256ms ack,0.2s display
  {HT6022_1MSa,   50e-3,    1e-6,  625,   HT6022_1MB}, //    1s ack,0.5s display
  {HT6022_1MSa,  100e-3,    1e-6, 1024,   HT6022_1MB}  //    1s ack,1.0s display
};


DSO_CHANNEL Channel[6] =                         // vertical deflection settings
{
  {10.0/2, 2.0, 0, 0, 0, HT6022_10V, true, false, false}, // 2V
  {10.0/2, 1.0, 0, 0, 1, HT6022_10V, true, false, false}, // 1V
  { 5.0/2, 0.5, 0, 0, 2, HT6022_5V,  true, false, false}, // 0.5V
  { 2.0/2, 0.2, 0, 0, 3, HT6022_2V,  true, false, false}, // 0.2V
  { 1.0/2, 0.1, 0, 0, 4, HT6022_1V,  true, false, false}, // 0.1V
  { 1.0/2,0.05, 0, 0, 5, HT6022_1V,  true, false, false}  // 0.05V
};



MainWindow::MainWindow(QWidget *parent):
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  int res;

  if(!HT6022_Init())
  {
    QPalette* palette = new QPalette();
    palette->setColor(QPalette::WindowText,Qt::yellow);
    res= HT6022_FirmwareUpload();
    if (res == HT6022_SUCCESS || res == HT6022_LOADED)
    {
      while((res = HT6022_DeviceOpen (&Device)) != 0)
      {
        sleep(1);
        qDebug() << "Waiting for device..." << endl;
        ui->statusBar->showMessage("Loading Firmware...",0);
        QApplication::processEvents();
      }

      ui->checkBoxCH1ON->setChecked(true);
      ui->checkBoxCH2ON->setChecked(true);

      Channel1 = Channel[1];
      Channel2 = Channel[1];
      Channel1.VScale *= VScaleFactor;
      Channel2.VScale *= VScaleFactor;
      Channel1.Zero = Zero1[1];
      Channel2.Zero = Zero2[1];

      HT6022_SetCH1IR (&Device, HT6022_10V);
      HT6022_SetCH2IR (&Device, HT6022_10V);
      on_comboSampling_currentIndexChanged(TDIV_1MS);
      read_cal_file();

      worker.alive = 1;
      worker.TriggerEdge = 1;
      worker.TriggerChannel = 0;
      worker.TriggerLevel = 128;                // equivalen to Dso.VTrigger = 0
      worker.holdoff = 40;                                        // delay in ms

      // ui->lblholdoff->setText("40.00ms");
      ui->comboSampling->setCurrentIndex(TDIV_1MS);
      ui->statusBar->showMessage("Device initialized.",0);

      worker.start();
      worker.blockSignals(1);
      ui->actionSave_to_file->setEnabled(false);
      //y1_vec.reserve(HT6022_1KB);  // 'c' code will write directly to std::vec
      //y2_vec.reserve(HT6022_1KB);   // might want this if dynamicaly allocated
      //x_vec.reserve(HT6022_1KB);           // allows re-sizing for 500ns range
    }
    else
    {
      exit(res);
    }
  }
  else
  {
    exit(-1);
  }
  setupPlot(ui->customPlot);
  connect(&worker, SIGNAL(dataReady()), this, SLOT(updatePlot()));
}

void MainWindow::setupPlot(QCustomPlot *customPlot)
{
  customPlot->setBackground(Qt::black);
  // create graph and assign data to it:
  customPlot->addGraph();
  customPlot->addGraph();

  vCursorX1 = new QCPItemLine(customPlot);
  vCursorX1->setPen(QColor(Qt::white));
  vCursorX1->start->setCoords(CursorX1,-Channel1.VScale);
  vCursorX1->end->setCoords(CursorX1,Channel1.VScale);
  vCursorTrigger = new QCPItemLine(customPlot);
  vCursorTrigger->setPen(QColor(Qt::darkYellow));

  customPlot->xAxis->setTickLabels(0);
  customPlot->yAxis->setTickLabels(0);
  customPlot->xAxis->setRange(0, 10 * Dso.Tdiv);
  customPlot->yAxis->setRange(-1,1);
  customPlot->xAxis->setAutoTickStep(false);
  customPlot->xAxis->setTickStep(Dso.Tdiv);
  customPlot->yAxis->setAutoTickStep(false);
  customPlot->yAxis->setTickStep(0.25);
  customPlot->graph(0)->setPen(QPen(Qt::yellow));
  customPlot->axisRect()->setBackground(Qt::black);
  customPlot->graph(1)->setPen(QPen(Qt::cyan));
  customPlot->setInteractions(QCP::iRangeDrag);
  connect
  (
    customPlot->yAxis,
    SIGNAL(rangeChanged(QCPRange)),
    this,
    SLOT(onYRangeChanged(QCPRange))
  );
}


void MainWindow::SetTriggerLine(DSO_CHANNEL* Channel)  // pos of horiz trig line
{
  double VTrigger;

  VTrigger = Channel->Inv ? -Dso.VTrigger : Dso.VTrigger;

  vCursorTrigger->start->setCoords
  (
    -1, VTrigger/(4*Channel->Vdiv)+Channel->VOffset
  );
  vCursorTrigger->end->setCoords
  (
    1, VTrigger/(4*Channel->Vdiv)+Channel->VOffset
  );

  worker.TriggerLevel =
    (unsigned char)(Dso.VTrigger * 128 /Channel->VScale + 128 + Channel->Zero);
}


void MainWindow::onYRangeChanged(const QCPRange &range)
{
  QCPRange boundedRange = range;
  boundedRange.lower = -1;
  boundedRange.upper = 1;

  ui->customPlot->yAxis->setRange(boundedRange);
}


MainWindow::~MainWindow()
{
  delete ui;
  on_actionExit_triggered();
}


void MainWindow::updatePlot()            // invoked by signal from worker thread
{
  // timer.start();

  if(Dso.Mode == SINGLE)                                          // Single shot
  {
    if(worker.mode == HOLD)                                     // triggered ...
    {
      worker.blockSignals(1);                 // ... so stop further updates ...
      Dso.Status = STOP;
      ui->btnGet->setText("ARM");                    // ... and invite re-arming
      ui->actionSave_to_file->setEnabled(true);
    }
  }
  else
  {
    if(Dso.Status == RUN) worker.mode = Dso.Mode;
    else worker.mode = HOLD;
  }

  if(worker.TriggerPoint == 0)
  {             // In AUTO, wait ~200ms before resuming scan without trigger ...
    if(withhold++ < 5) return;                       // ... like analoge 'scope
  }                 // makes display more stable in AUTO when timebase < 2us/div
  else withhold = 0;

  if
  (
    get_post_trigger_waveforms            // consider adding pre-trigger version
    (
      (double*)&y1_vec[0],
      (double*)&y2_vec[0],
      (double*)&x_vec[0],
      worker.CH0,
      &Channel1,
      &Channel2,
      worker.TriggerPoint,
      worker.TriggerEdge
    ) < 0                                         // nothing to plot if negative
  ) return;

  if(Calibrate)
  {
    do_cal(worker.CH0, Calibrate);
    Calibrate--;
    if(Calibrate == 0) ui->statusBar->showMessage("Offset Null Completed",0);
  }

  ui->customPlot->graph(0)->clearData();
  ui->customPlot->graph(1)->clearData();

  if(Channel1.Enabled)
    ui->customPlot->graph(0)->setData(x_vec,y1_vec);

  if(Channel2.Enabled)
    ui->customPlot->graph(1)->setData(x_vec,y2_vec);

  ui->customPlot->replot();

  // qDebug() << "Time: " << timer.nsecsElapsed() << "ns";
}


void MainWindow::on_btnGet_clicked()                    // Toggle capture / hold
{
  if(Dso.Status == STOP)
  {
    ui->actionSave_to_file->setEnabled(false);
    worker.blockSignals(0);
    Dso.Status = RUN;    
    if(Dso.Mode == SINGLE)
    {
       worker.mode = SINGLE;
       ui->btnGet->setText("READY");
    }
    else ui->btnGet->setText("STOP");
  }
  else if(Dso.Status == RUN)
  {
     ui->actionSave_to_file->setEnabled(true);
     worker.blockSignals(1);
     Dso.Status = STOP;
     ui->btnGet->setText("ARM");
  }
}


void MainWindow::on_comboBox_4_currentIndexChanged(int index)
{                                                        // AUTO, NORMAL, SINGLE
  if(index < 0 || index > 2) return;
  Dso.Mode = (DSO_MODE_TypeDef)index;

  Dso.Status = STOP;
  on_btnGet_clicked();         // automatically run when AUTO or NORMAL selected
}


// Trigger

void MainWindow::on_comboBox_rise_currentIndexChanged(int index)        // slope
{
  worker.TriggerEdge = index?0:1;
}


void MainWindow::on_comboBoxCHSel_currentIndexChanged(int index)
{                                                   // Trigger channel selection
  Dso.ChTrigger = index + 1;
  if(Dso.ChTrigger == 1)
  {
    worker.TriggerChannel = 0;
    Channel1.Enabled = true;
    ui->checkBoxCH1ON->setChecked(true);
    SetTriggerLine(&Channel1);
  }
  else
  {
    worker.TriggerChannel = 1;
    Channel2.Enabled = true;
    ui->checkBoxCH2ON->setChecked(true);
    SetTriggerLine(&Channel2);
  }
}


void MainWindow::on_dialTrigger_valueChanged(int value)  // Trigger level: 0-100
{
  if(Dso.ChTrigger == 1)
  {
    Dso.VTrigger = Channel1.VScale*((double)value/100.0);
    SetTriggerLine(&Channel1);
  }
  else
  {
    Dso.VTrigger = Channel2.VScale*((double)value/100.0);
    SetTriggerLine(&Channel2);
  }
  ui->labelTrigger->setText(QString::number(Dso.VTrigger,'g',3)+"V");
  ui->customPlot->replot();
}


void MainWindow::on_dialHoldoff_valueChanged(int value)      // 0-200ms hold off
{
  char valueStr[10];

  worker.holdoff = value;
  float2engStr(valueStr,(double)value/1000);
  ui->lblholdoff->setText(valueStr);
}


void MainWindow::on_dialDelay_valueChanged(int pos) // Delayed Trig: 0-2,000,000
{
  static int turns = 0;                                    // multi turn control
  static int oldpos = 0;
  static int base = 0;                 // centre point of expanded control range
  static int vernier = 10;

  int i;
  double value;                                    // cumulative control setting
  double delay;                                      // trigger delay in seconds
  double delta;                           // delay integer truncation correction
  char valueStr[10];

  Dso.Pos = pos;                // for on_comboSampling_currentIndexChanged(pos)
  if(oldpos - pos > 9000) turns++;
  else if(pos - oldpos > 9000) turns--;
  oldpos = pos;
  pos += turns*10000;

  if(pos - base > 1000) base = pos - 1000;
  else if(pos - base < -1000) base = pos + 1000;                   // drag range
  pos -= base;
  value = base + pos / vernier;                    // vernier over limited range
  if(value < 50 || value > 50) vernier = 20;             // extra slow near zero
  else vernier = 10;                       // wider fine control range elsewhere

  value /= 200000.0;
             // as far as posssible, preserve delay setting when changing ranges
  if(Dso.MemDepth > HT6022_256KB) value *= HT6022_256KB;
  else value *= Dso.MemDepth;
  if(Dso.Ts > 1/16e6) value /= Dso.Ts*16e6;

  Dso.TriggerDelay = (int)value;
  delta = value - (double)Dso.TriggerDelay;
  delay = value * Dso.Ts;
  delta *= Dso.Ts;
  Dso.TriggerOffset = delta;

    // Changing delay invalidates the timings in x_vec so we need a way to clear
    // it.  This is a horrible way to suppress partial traces on the display ...
  for(i = 0; i < HT6022_1KB; i++) x_vec[i] = DBL_MAX;       // ... but it works!

  float2engStr(valueStr, delay);
  ui->lblfreq->setText(valueStr);
  if(Dso.Status == STOP || Dso.Mode == SINGLE) updatePlot();
  else ui->customPlot->replot();    // already running in AUTO or NORMAL trigger
}

/*
void MainWindow::on_comboBoxPostTrig_currentIndexChanged(int index)
{                                  // Post or Pre trigger display mode selection
  TriggerType = index;
}
*/


// Timebase

void MainWindow::on_comboSampling_currentIndexChanged(int index)     // Timebase
{
  static int d_size[6] =
  {
    HT6022_1KB/20, // 20ns
    HT6022_1KB/8,  // 50ns
    HT6022_1KB/4,  // 100ns
    HT6022_1KB/2,  // 200ns
    HT6022_1KB-32, // 400ns    22 is minimum acceptable reduction: see +32 below
    HT6022_1KB/2   // 1us
  };

  static const char* msg[21] =              // sample rate, t/div, buffer length
  {
    "48Ms/s, 20ns/div, 20us",
    "48Ms/s, 50ns/div, 20us",
    "48Ms/s, 100ns/div, 20us",
    "48Ms/s, 200ns/div, 20us",
    "48Ms/s, 400ns/div, 20us",
    "48Ms/s, 1us/div, 20us",
    "48Ms/s, 2us/div, 20us",
    "16Ms/s, 5us/div, 16ms",
    "16Ms/s, 10us/div, 16ms",
    "16Ms/s, 20us/div, 16ms",
    "16Ms/s, 50us/div, 16ms",
    "16Ms/s, 100us/div, 16ms",
    "16Ms/s, 200us/div, 16ms",
    "16Ms/s, 500us/div, 16ms",
    "16Ms/s, 1ms/div, 16ms",
    "16Ms/s, 2ms/div, 32ms",
    "16Ms/s, 5ms/div, 64ms",
    "8Ms/s, 10ms/div, 128ms",
    "4Ms/s, 20ms/div, 256ms",
    "1Ms/s, 50ms/div, 1s",
    "1Ms/s, 100ms/div, 1s"
  };

  HT6022_SRTypeDef SR;
  int i;

  if(index < 0 || index > 21) return;
  if
  (
    (Dso.Status == STOP || Dso.Mode == SINGLE) &&        // stored trace display
    (Dso.Ts != ComboSample[index].Ts)         // moving outside current SR range
  ) return;                    // won't trap NORMAL trigger case when no trigger

  SR = ComboSample[index].SR;
  Dso.Tdiv = ComboSample[index].Tdiv;
  Dso.MemDepth = ComboSample[index].MemDepth;
  if
  (
    (Dso.Ts != ComboSample[index].Ts) ||             // crossing SR boundary ...
    (Dso.SubSample != ComboSample[index].SubSample) || // ... affecting display
    (index == 5) || (index == 6)       // switching away from five fold upsample
  )
  {
    Dso.SubSample = ComboSample[index].SubSample;
    Dso.Ts = ComboSample[index].Ts;
    on_dialDelay_valueChanged(Dso.Pos);               // invalidate trace buffer
  }
  if(index < 6) Dso.DisplayDepth = d_size[index] + 32;   // allow for trig delay
  else Dso.DisplayDepth = (int)HT6022_1KB;

  for(i = 0; i < HT6022_1KB; i++) y1_vec[i] = 0, y2_vec[i] = 0;

  ui->customPlot->xAxis->setRange(0, 10 * Dso.Tdiv);
  ui->customPlot->xAxis->setAutoTickStep(false);
  ui->customPlot->xAxis->setTickStep(Dso.Tdiv);

  HT6022_SetSR (&Device,SR);

  if(Dso.Status == STOP || Dso.Mode == SINGLE) updatePlot();
  ui->statusBar->showMessage(msg[index],0);
}


// Vertical: CH1

void MainWindow::on_checkBoxCH1ON_toggled(bool checked)      // Channel 1 enable
{
  Channel1.Enabled = checked;
}


void MainWindow::on_checkBoxCH1Add_toggled(bool checked)
{  // often used in conjunction with inversion and variable vertical sensitivity
  Dso.ChAdd = checked ? 1 : 0;
}


void MainWindow::on_checkBoxCH1Inv_toggled(bool checked)
{
  Channel1.Inv = checked;
  if(Dso.ChTrigger == 1) SetTriggerLine(&Channel1);
}


void MainWindow::on_checkBoxCH1Glitch_toggled(bool checked)
{
  Channel1.Glitch = checked;
}


void MainWindow::on_comboBoxMode1_currentIndexChanged(int index)
{                           // select trace position or variable gain, Channel 1
  char valueStr[10];

  VarCH1 = (VARMODE_TypeDef)index;
  if(VarCH1 == POSITION)
  {
    sprintf(valueStr,"%4.3fV",Channel1.VOffset*Channel1.Vdiv*4.0);
    ui->lblCH1->setText(valueStr);
  }
  else
  {
    sprintf(valueStr,"%4.3fV",Channel1.Vdiv);
    ui->lblCH1->setText(valueStr);
  }
}


void MainWindow::on_dialCursorV1_valueChanged(int value)//trace position or gain
{
  char valueStr[10];

  if(VarCH1 == POSITION)
  {
    Channel1.VOffset = (double)value/100.0;
    sprintf(valueStr,"%4.3fV",Channel1.VOffset*Channel1.Vdiv*4.0);
    ui->lblCH1->setText(valueStr);
    if(Dso.Status == STOP || Dso.Mode == SINGLE) updatePlot();
    else ui->customPlot->replot();          // running in AUTO or NORMAL trigger
  }
  else
  {
    if(value < 0) value /= 2;
    Channel1.Vdiv = Channel[Channel1.index].Vdiv / ((value + 100) / 100.0);
    sprintf(valueStr,"%4.3fV",Channel1.Vdiv);
    ui->lblCH1->setText(valueStr);
  }
  if(Dso.ChTrigger == 1) SetTriggerLine(&Channel1);
}


void MainWindow::on_comboBoxV1div_currentIndexChanged(int index)     // Volt/Div
{
  char valueStr[10];

  if(index < 0 || index > 6) return;

  Channel1.VRange = Channel[index].VRange;
  Channel1.Vdiv = Channel[index].Vdiv;
  Channel1.VScale = Channel[index].VScale * VScaleFactor;
  Channel1.Zero = Zero1[index];                   // correct amp offset by range
  Channel1.index = index;
  HT6022_SetCH1IR (&Device, Channel1.VRange);
  if(Dso.ChTrigger == 1) SetTriggerLine(&Channel1);
  if(VarCH1 == POSITION)
    sprintf(valueStr,"%4.3fV",Channel1.VOffset*Channel1.Vdiv*4.0);
  else
    sprintf(valueStr,"%4.3fV",Channel1.Vdiv);
  ui->lblCH1->setText(valueStr);
}


// Vertical: CH2

void MainWindow::on_checkBoxCH2ON_toggled(bool checked)
{
  Channel2.Enabled = checked;
}

/*
void MainWindow::on_checkBoxCH2Add_toggled(bool checked)
{
  Dso.ChAdd = checked ? 2 : 0;
}
*/

void MainWindow::on_checkBoxCH2Inv_toggled(bool checked)
{
  Channel2.Inv = checked;
  if(Dso.ChTrigger == 2) SetTriggerLine(&Channel2);
}


void MainWindow::on_checkBoxCH2Glitch_toggled(bool checked)
{
  Channel2.Glitch = checked;
}


void MainWindow::on_comboBoxMode2_currentIndexChanged(int index)
{                           // select trace position or variable gain, Channel 2
  char valueStr[10];

  VarCH2 = (VARMODE_TypeDef)index;
  if(VarCH2 == POSITION)
  {
    sprintf(valueStr,"%4.3fV",Channel2.VOffset*Channel2.Vdiv*4.0);
    ui->lblCH2->setText(valueStr);
  }
  else
  {
    sprintf(valueStr,"%4.3fV",Channel2.Vdiv);
    ui->lblCH2->setText(valueStr);
  }
}


void MainWindow::on_dialCursorV2_valueChanged(int value)//trace position or gain
{
  char valueStr[10];

  if(VarCH2 == POSITION)
  {
    Channel2.VOffset = (double)value/100.0;
    sprintf(valueStr,"%4.3fV",Channel2.VOffset*Channel2.Vdiv*4.0);
    ui->lblCH2->setText(valueStr);
    if(Dso.Status == STOP || Dso.Mode == SINGLE) updatePlot();
    else ui->customPlot->replot();          // running in AUTO or NORMAL trigger
  }
  else
  {
    if(value < 0) value /= 2;
    Channel2.Vdiv = Channel[Channel2.index].Vdiv / ((value + 100) / 100.0);
    sprintf(valueStr,"%4.3fV",Channel2.Vdiv);
    ui->lblCH2->setText(valueStr);
  }
  if(Dso.ChTrigger == 2) SetTriggerLine(&Channel2);
}


void MainWindow::on_comboBoxV2div_currentIndexChanged(int index)     // Volt/Div
{
  char valueStr[10];

  if(index < 0 || index > 6) return;

  Channel2.VRange = Channel[index].VRange;
  Channel2.Vdiv = Channel[index].Vdiv;
  Channel2.VScale = Channel[index].VScale * VScaleFactor;
  Channel2.Zero = Zero2[index];                   // correct amp offset by range
  Channel2.index = index;
  HT6022_SetCH2IR (&Device, Channel2.VRange);
  if(Dso.ChTrigger == 2) SetTriggerLine(&Channel2);
  if(VarCH2 == POSITION)
    sprintf(valueStr,"%4.3fV",Channel2.VOffset*Channel2.Vdiv*4.0);
  else
    sprintf(valueStr,"%4.3fV",Channel2.Vdiv);
  ui->lblCH2->setText(valueStr);
}


void MainWindow::on_actionSave_to_file_triggered()
{
  write_tracefile(worker.CH0, Dso.MemDepth);
}


void MainWindow::on_actionExit_triggered()
{
  worker.alive = 0;                                          // terminate thread
  sleep(1);                 // allow time for worker thread to terminate cleanly
  HT6022_DeviceClose(&Device);                               // shut down 'scope
  HT6022_Exit();
  qDebug() << "Exiting\r\n";
  exit(0);
}


void MainWindow::on_actionOffset_Null_triggered()        // offset null by range
{
  QMessageBox msgBox;

  msgBox.setText("Connect both probes to gound    ");
  msgBox.exec();

  Calibrate = 25;                // magic number: state variable for calibration
  Dso.Status = RUN;                             // make sure we are getting data
  ui->btnGet->setText("STOP");
  worker.blockSignals(0);
}


void MainWindow::on_actionSetScaleFactor_triggered()
{
  // Insert code to set scale factor here.  Not sure whether calibrator output
  // is reliably 2.00V p-p.  Currently use 1.5V cell, DVM and save trace.
  write_cal_file();
}

