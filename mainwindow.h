/*
  mainwindow.h: QT user interface for the Hantek 6022BL 'scope.

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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qcustomplot.h"
#include "dso.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    void setupPlot(QCustomPlot *customPlot);
    ~MainWindow();

private slots:

    void on_btnGet_clicked();

    void updatePlot();

    void on_comboSampling_currentIndexChanged(int index);

    void on_dialTrigger_valueChanged(int value);

    void on_dialHoldoff_valueChanged(int value);

    void on_dialDelay_valueChanged(int value);

    void onYRangeChanged(const QCPRange &range);

    //void on_comboBoxPostTrig_currentIndexChanged(int index);

    void on_comboBoxV1div_currentIndexChanged(int index);

    void on_comboBoxV2div_currentIndexChanged(int index);

    void on_comboBoxMode1_currentIndexChanged(int index);

    void on_comboBoxMode2_currentIndexChanged(int index);

    void on_dialCursorV1_valueChanged(int value);

    void on_dialCursorV2_valueChanged(int value);

    void on_comboBoxCHSel_currentIndexChanged(int index);

    void on_comboBox_4_currentIndexChanged(int index);

    void on_checkBoxCH1ON_toggled(bool checked);

    void on_checkBoxCH2ON_toggled(bool checked);

    void on_checkBoxCH1Inv_toggled(bool checked);

    void on_checkBoxCH2Inv_toggled(bool checked);

    void on_checkBoxCH1Glitch_toggled(bool checked);

    void on_checkBoxCH2Glitch_toggled(bool checked);

    void on_checkBoxCH1Add_toggled(bool checked);

    void on_comboBox_rise_currentIndexChanged(int index);

    void on_actionSave_to_file_triggered();

    void on_actionExit_triggered();

    void on_actionOffset_Null_triggered();

    void SetTriggerLine(DSO_CHANNEL* Channel);

    void on_actionSetScaleFactor_triggered(void);

private:
    Ui::MainWindow *ui;
};

#endif                                                           // MAINWINDOW_H
