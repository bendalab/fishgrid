/*
  recorder.h
  Records data from the DAQ boards.

  FishGridRecorder
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

#ifndef _RECORDER_H_
#define _RECORDER_H_ 1

#include <QThread>
#include <QMutex>
#include "configdata.h"
//#include "tcpsocket.h"
#include "recording.h"
#include "datathread.h"
#include "simulationthread.h"
#ifdef HAVE_COMEDILIB_H
#include "comedithread.h"
#endif
#ifdef HAVE_NIDAQMXBASE_H
#include "nidaqmxthread.h"
#endif

using namespace std;
using namespace relacs;


/*! 
\class Recorder
\brief Records data from the DAQ boards.
\author Jan Benda
*/


//class Recorder : public QThread, public TCPSocket, public ConfigData
class Recorder : public QThread, public ConfigData
{

public:

  /*! Constructs a Recorder.
      \param[in] samplerate the sampling rate in hertz 
      \param[in] maxvolts the maximum expected voltage 
      \param[in] gain the gain of the amplifiers
      \param[in] buffertime the time in seconds for which the buffer should hold the data
      \param[in] simulate start in simulation mode */
  Recorder( double samplerate,
	    double maxvolts, double gain,
	    double buffertime, bool simulate );
  ~Recorder( void );

    /*! Start data acquisition. */
  int start( void );
    /*! Processes data (save to disk, analyse, and plot). */
  int processData( void );
    /*! Stops all FishGridWidget activities and exits. */
  void finish( void );

    /*! Sleep for the data process interval. */
  void sleep( void );

    /*! The main server loop for establishing and handling of connections. */
  //  int serverLoop( void );
    /*! Reads the commands and executes the requested tasks. */
  //  int readCommand( int sock );


protected:

    /*! Write current time and \a message to stderr. */
  void printlog( const string &message ) const;
    /*! Dummy for QThread. */
  void run( void );


private:

    /*! Saves data to files. */
  Recording FileSaver;

  DataThread *DataLoop;

  bool FileSaving;
  QMutex FileSavingMutex;

};


#endif /* ! _RECORDER_H_ */

