/*
  datathread.cc
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

#include "datathread.h"


DataThread::DataThread( const string &name, ConfigData *cd )
  : ConfigClass( name ),
    CD( cd ),
    Error( false )
{
}


int DataThread::start( double duration )
{
  // analog input buffers:
  for ( int g=0; g<ConfigData::MaxGrids; g++ ) {
    if ( used( g ) ) {
      int nbuffer = (int)::floor( sampleRate()*bufferTime()*gridChannels( g ) );
      AIBuffer[g].reserve( nbuffer );
      printlog( "buffer size of grid " + Str( g+1 ) +
		" is " + Str( AIBuffer[g].capacity() ) );
    }
  }

  RunMutex.lock();
  Run = true;
  RunMutex.unlock();
  
  int r = initialize( duration );

  if ( r == 0 )
    QThread::start( HighestPriority );
  
  return r;
}


void DataThread::stop( void )
{
  RunMutex.lock();
  bool rd = Run;
  RunMutex.unlock();
  if ( rd ) {
    RunMutex.lock();
    Run = false;
    RunMutex.unlock();
    QThread::wait();
  }
}


bool DataThread::running( void ) const
{
  RunMutex.lock();
  bool rd = Run;
  RunMutex.unlock();
  return rd;
}


int DataThread::maxGrids( void ) const
{
  return CD->MaxGrids;
}


bool DataThread::used( int g ) const
{
  return CD->Used[g];
}


int DataThread::rows( int g ) const
{
  return CD->Rows[g];
}


int DataThread::columns( int g ) const
{
  return CD->Columns[g];
}


int DataThread::gridChannels( int g ) const
{
  return CD->GridChannels[g];
}


int DataThread::channels( void ) const
{
  return CD->Channels;
}


double DataThread::maxVolts( void ) const
{
  return CD->MaxVolts;
}


double DataThread::sampleRate( void ) const
{
  return CD->SampleRate;
}


double DataThread::gain( void ) const
{
  return CD->Gain;
}


void DataThread::lockAI( int g )
{
  AIMutex[g].lock();
}


void DataThread::unlockAI( int g )
{
  AIMutex[g].unlock();
}


double DataThread::bufferTime( void ) const
{
  return CD->BufferTime;
}


void DataThread::run( void )
{
  int r = 0;
  bool rd = true;

  do {
    r = read();
    RunMutex.lock();
    rd = Run;
    RunMutex.unlock();
  } while ( rd && r == 0 );
  RunMutex.lock();
  Run = false;
  RunMutex.unlock();
  finish();
}


void DataThread::printlog( const string &message ) const
{
  CD->printlog( message );
  Error = true;
}


bool DataThread::error( void ) const
{
  return Error;
}


void DataThread::clearError( void )
{
  Error = false;
}

