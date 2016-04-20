/*
  fishgridwidget.h
  BaseWidget implementation for coordinating data acquisition threads and analyzers

  FishGrid
  Copyright (C) 2009 Jan Benda <benda@bio.lmu.de> & Joerg Henninger <henninger@bio.lmu.de>

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

#ifndef _FISHGRIDWIDGET_H_
#define _FISHGRIDWIDGET_H_ 1

#include <iostream>
#include <QDateTime>
#include <relacs/optdialog.h>
#include "recording.h"
#include "cyclicbuffer.h"
#include "datathread.h"
#include "basewidget.h"

using namespace std;
using namespace relacs;

/*! 
\class FishGridWidget
\brief BaseWidget implementation for coordinating data acquisition threads and analyzers
\author Jan Benda

\section keys Key shortcuts

Plots:
- \c Q : increase frequency of analysis and plotting (Quicker)
- \c W : decrease frequency of analysis and plotting (sloWer)

File saving:
- \c Return : start/stop saving recording to a file
- \c Backspace : make a time stamp

*/


class FishGridWidget : public BaseWidget
{
  Q_OBJECT

  friend class Analyzer;
  friend class Recording;


public:

  /*! Constructs a FishGrid.
      \param[in] samplerate the sampling rate in hertz 
      \param[in] maxvolts the maximum expected voltage 
      \param[in] gain the gain of the amplifiers
      \param[in] buffertime the time in seconds for which the buffer should hold the data
      \param[in] datatime the size of the data segements in seconds on which analysis operates on
      \param[in] datainterval the time between calls to processData() in seconds
      \param[in] dialog open a configuration dialog
      \param[in] saving start saving right away
      \param[in] stoptime stop saving and quit at this time
      \param[in] simulate start in simulation mode */
  FishGridWidget( double samplerate,
		  double maxvolts, double gain,
		  double buffertime,
		  double datatime, double datainterval,
		  bool dialog, bool saving, const string &stoptime, bool simulate );
  ~FishGridWidget( void );


public slots:

    /*! Start data acquisition. */
  void start( void );

    /*! Processes data (save to disk, analyse, and plot). */
  void processData( void );

    /*! Stops all FishGridWidget activities and exits. */
  void finish( void );


protected:

  virtual void keyPressEvent( QKeyEvent *event );

    /*! Write current time and \a message to stderr and into a log file. */
  virtual void printlog( const string &message ) const;

    /*! The analog input buffer of grid \a g. */
  inline CyclicBuffer< float > &inputBuffer( int g ) { return DataLoop->inputBuffer( g ); };
    /*! The analog input buffer of grid \a g. */
  inline const CyclicBuffer< float > &inputBuffer( int g ) const { return DataLoop->inputBuffer( g ); };
    /*! Lock the analog input mutex of grid \a g. */
  void lockAI( int g );
    /*! Unlock the analog input mutex of gid \a g. */
  void unlockAI( int g );


protected slots:

    /*! Initializes the files for saving. */
  void startSaving( int r );
    /*! Save the time stamp dialog. */
  void saveTimeStamp( int r );


private:

    /*! Saves data to files. */
  Recording FileSaver;
    /*! Start saving right away. */
  bool AutoSave;
    /*! Automatically stop recording at \a StopTime. */
  bool StopRecording;
    /*! Stop recording at this time. */
  QDateTime StopTime;

    /*! Acquire the data. */
  DataThread *DataLoop;

    /*! The dialog for the meta data. */
  OptDialog *MetadataDialog;
    /*! The dialog for the time stamps. */
  OptDialog *TimeStampDialog;

};


#endif /* ! _FISHGRIDWIDGET_H_ */

