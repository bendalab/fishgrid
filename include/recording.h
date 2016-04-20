/*
  recording.h
  Records data to disc

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

#ifndef _RECORDING_H_
#define _RECORDING_H_ 1

#include <fstream>
#include <relacs/configclass.h>
#include "configdata.h"
#include "datathread.h"

using namespace std;
using namespace relacs;


/*! 
\class Recording
\brief Records data to disc
\author Jan Benda
*/

class Recording : public ConfigClass
{

public:

    /*! Constructs a Recording. */
  Recording( ConfigData *cd, DataThread *dt=0 );
    /*! Destructs a Recording. */
  ~Recording( void );

    /*! Pass the DataThread \a dt to \a this. */
  void setDataThread( DataThread *dt );

    /*! Start a recording. Returns the path name on success.
        Immediatley opens the files for the raw data and for time stamps
	if \a tracefiles or \a timestamps, respectively, are \c true. */
  string start( bool tracefiles=true, bool timestamps=true );
  /*! Open files for storing raw data of the traces.
      \a name is used for creating the names of the files:
      \c traces-grid1NAME.raw */
  void openTraceFiles( const string &name="" );
    /*! Close all open trace files. */
  void closeTraceFiles( void );
    /*! Write data to disc and returns a meaningful message. */
  string save( void );
    /*! Stop a recording. */
  void stop( void );

    /*! \return \c true if we are currently saving data to disc. */
  bool saving( void ) const;

    /*! Records a time stamp. */
  void timeStamp( void );
    /*! Saves the previously recorded time stamp. */
  void saveTimeStamp( void );
    /*! Save a time stamp with comment \a comment. */
  void saveTimeStamp( const string &comment );
    /*! Adds a time stamp saying that the acquisition was interrupted. */
  void interruptionTimeStamp( void );
    /*! Returns the options used for a time stamp. */
  Options &timeStampOpts( void );

    /*! Write current time and \a message to stderr and into a log file. */
  void printlog( const string &message ) const;


private:

  ConfigData *CD;
  DataThread *DT;

    /*! Data are saved to disc. */
  bool Save;
    /*! The path (directory or common basename)
        where all data of the current session are stored. */
  string Path;
    /*! The template from which \a Path is generated. */
  string PathTemplate;
    /*! Identification number for pathes used to create a base path
        from \a PathFormat. */
  int PathNumber;
    /*! Binary files for the voltage traces of each grid. */
  ofstream TraceFile[ConfigData::MaxGrids];
    /*! Indicates, whether trace files are open. */
  bool TraceFilesOpen;
    /*! Index of the first saved data points for each grid. */
  long long FirstTraceIndex[ConfigData::MaxGrids];
    /*! So far saved data points for each grids. */
  long long TraceIndex[ConfigData::MaxGrids];
    /*! Time and date of the start of the recording. */
  QDateTime StartRecTime;

    /*! Options for the time stamp dialog. */
  Options TimeStampOpts;
    /*! File for time stamps. */
  ofstream TimeStampFile;
    /*! Indicates, whether time-stamp file is open. */
  bool TimeStampsOpen;
    /*! Number of the time stamp. */
  int TimeStampNum;

    /*! The log-file. */
  ofstream *LogFile;

};


#endif /* ! _RECORDING_H_ */

