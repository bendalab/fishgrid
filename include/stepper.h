/*
  stepper.h
  Repetitive recordings stepping through predefined fish positions.

  FishGrid
  Copyright (C) 2011 Jan Benda <benda@bio.lmu.de> & Joerg Henninger <henninger@bio.lmu.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.
  
  FishGrid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _STEPPER_H_
#define _STEPPER_H_ 1

#include <iostream>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QStackedLayout>
#include <relacs/optdialog.h>
#include <relacs/plot.h>
#include "recording.h"
#include "cyclicbuffer.h"
#include "datathread.h"
#include "configdata.h"

using namespace std;
using namespace relacs;

/*! 
\class Stepper
\brief Repetitive recordings stepping through predefined fish positions.
\author Jan Benda

\section keys Key shortcuts

- \c M : show window maximized/normalized
- \c Enter : Start recording at next raster position (that is marked red).
- \c Backspace : Redo recording at current raster position (that is marked yellow).

*/


class Stepper : public QWidget, public ConfigData
{
  Q_OBJECT

  friend class Recording;


public:

  /*! Constructs a %Stepper.
      \param[in] samplerate the sampling rate in hertz 
      \param[in] maxvolts the maximum expected voltage 
      \param[in] gain the gain of the amplifiers
      \param[in] buffertime the time in seconds for which the buffer should hold the data
      \param[in] datatime the size of the data segements in seconds on which analysis operates on
      \param[in] datainterval the time between calls to processData() in seconds
      \param[in] dialog open a configuration dialog
      \param[in] simulate start in simulation mode */
  Stepper( double samplerate,
	   double maxvolts, double gain,
	   double buffertime,
	   double datatime, double datainterval,
	   bool dialog, bool simulate );
  ~Stepper( void );


public slots:

    /*! Start data acquisition for next raster position. */
  void start( void );
    /*! Start data acquisition for the current rater position. */
  void startAgain( void );

    /*! Record data of next raster position. */
  void recordData( void );

    /*! Stops all Stepper activities and exits. */
  void finish( void );


protected:

  virtual void keyPressEvent( QKeyEvent *event );

    /*! Write current time and \a message to stderr and into a log file. */
  virtual void printlog( const string &message ) const;

    /*! Plot the current status of the raster. */
  void plot( void );

    /*! The analog input buffer of grid \a g. */
  inline CyclicBuffer< float > &inputBuffer( int g ) { return DataLoop->inputBuffer( g ); };
    /*! The analog input buffer of grid \a g. */
  inline const CyclicBuffer< float > &inputBuffer( int g ) const { return DataLoop->inputBuffer( g ); };
    /*! Lock the analog input mutex of grid \a g. */
  void lockAI( int g );
    /*! Unlock the analog input mutex of gid \a g. */
  void unlockAI( int g );


private:

    /*! Saves data to files. */
  Recording FileSaver;
    /*! Acquire the data. */
  DataThread *DataLoop;
    /*! The dialog for the meta data. */
  OptDialog *MetadataDialog;

  // Variables defining the raster:
  double RasterStepX;
  double RasterStepY;
  double RasterWidthX;
  double RasterWidthY;
  double RasterOffsX;
  double RasterOffsY;
  int RasterInxX;
  int RasterInxY;
  int RasterPrevX;
  int RasterPrevY;
  int RasterMaxX;
  int RasterMaxY;

    /*! The current status of the raster. */
  Plot P;

  QStackedLayout *SL;
  QWidget *ActionWidget;
  QLabel *CurrentLabel;
  QLabel *NextLabel;
  QPushButton *AgainButton;
  QPushButton *NextButton;
  QPushButton *QuitButton;
  QLabel *RecordingLabel;

};


#endif /* ! _STEPPER_H_ */

