/*
  datathread.h
  Base class for acquiring data

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

#ifndef _DATATHREAD_H_
#define _DATATHREAD_H_ 1

#include <QThread>
#include <QMutex>
#include <relacs/configclass.h>
#include "cyclicbuffer.h"
#include "configdata.h"

using namespace std;
using namespace relacs;


/*! 
\class DataThread
\brief Base class for acquiring data
\author Jan Benda
*/

class DataThread : public QThread, public ConfigClass
{

public:

  DataThread( const string &name, ConfigData *cd );
  int start( double duration=0.0 );
  void stop( void );
  bool running( void ) const;

    /*! \return the maximum number of grids. */
  int maxGrids( void ) const;
    /*! \return \c true if grid \a g is used. */
  bool used( int g ) const;
    /*! \return the number of electrodes in a row for grid \a g. */
  int rows( int g ) const;
    /*! \return the number of electrodes in a column for grid \a g. */
  int columns( int g ) const;
    /*! \return the total number of electrodes for grid \a g. */
  int gridChannels( int g ) const;
    /*! \return the total number of electrodes for all grids. */
  int channels( void ) const;
    /*! The maximum voltage to be expected to measure in volts. */
  double maxVolts( void ) const;
    /*! The sampling rate per channel to be used in hertz. */
  double sampleRate( void ) const;
    /*! The gain factor of the amplifiers. */
  double gain( void ) const;

    /*! The analog input buffer. */
  inline CyclicBuffer< float > &inputBuffer( int g ) { return AIBuffer[g]; };
    /*! The analog input buffer. */
  inline const CyclicBuffer< float > &inputBuffer( int g ) const { return AIBuffer[g]; };
    /*! Lock the analog input mutex for grid \a g. */
  void lockAI( int g );
    /*! Unlock the analog input mutex for grid \a g. */
  void unlockAI( int g );

    /*! The time in seconds for which the buffer should hold the data. */
  double bufferTime( void ) const;


protected:

  virtual void run( void );
  virtual int initialize( double duration=0.0 )=0;
  virtual void finish( void )=0;
  virtual int read( void )=0;

    /*! Write current time and \a message to stderr and into a log file
        and set error flag to \c true. */
  void printlog( const string &message ) const;
    /*! Returns the error flag. */
  bool error( void ) const;
    /*! Clear the error flag. */
  void clearError( void );


private:

    /*! The analog input buffer. */
  CyclicBuffer< float > AIBuffer[ConfigData::MaxGrids];
  QMutex AIMutex[ConfigData::MaxGrids];

  bool Run;
  mutable QMutex RunMutex;
  ConfigData *CD;
  mutable bool Error;

};


#endif /* ! _DATATHREAD_H_ */

