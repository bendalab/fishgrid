/*
  nidaqmxthread.cc
  DataThread implementation for acquisition of data using NIDAQmxBase

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

#include "nidaqmxthread.h"


NIDAQmxThread::NIDAQmxThread( ConfigData *cd )
  : DataThread( "Acquisition", cd ),
    Handle( 0 )
{
  addSelection( "reference", "RSE|DIFF|RSE|NRSE" );
}


int NIDAQmxThread::initialize( double duration )
{
  // XXX add multi-grid support
  // XXX add finite samples support
  int32 error = 0;
  char errstr[2048];

  //create an acquisition task:
  error = DAQmxBaseCreateTask( "AI", &Handle );
  if ( error != 0 ) {
    DAQmxBaseGetExtendedErrorInfo( errstr, 2048 );
    DAQmxBaseClearTask( Handle );
    Handle = 0;
    printlog( "NIDAQmxBase: ! error no." + Str( error ) + "[" + Str( errstr ) + "]" );
    return -1;
  }

  char chanlist[100];
  sprintf( chanlist, "Dev1/ai0:%d", channels()-1 );
  printlog( "NIDAQmxBase channels: " + Str( chanlist ) );
  int ref = index( "reference" );
  printlog( "NIDAQmxBase reference: " + Str( ref ) + " (0=DIFF, 1=RSE, 2=NRSE)" );
  int32 reference = DAQmx_Val_Diff;
  if ( ref == 1 )
    reference = DAQmx_Val_RSE;
  else if ( ref == 2 )
    reference = DAQmx_Val_NRSE;
  error = DAQmxBaseCreateAIVoltageChan( Handle, chanlist, "", reference,
					-maxVolts()*gain(), maxVolts()*gain(),
					DAQmx_Val_Volts, NULL );
  if ( error != 0 ) {
    DAQmxBaseGetExtendedErrorInfo( errstr, 2048 );
    DAQmxBaseClearTask( Handle );
    Handle = 0;
    printlog( "NIDAQmxBase: ! error no." + Str( error ) + "[" + Str( errstr ) + "]" );
    return -2;
  }

  // timing:
  uInt64 samplesperchan = (int)::floor( sampleRate()*interval() ); // XXX should be ignored in continues acquisition ????
  printlog( "NIDAQmxBase sampling rate: " + Str( sampleRate() ) + "Hz" );
  printlog( "NIDAQmxBase samples per channel: " + Str( (long)samplesperchan ) );
  error = DAQmxBaseCfgSampClkTiming( Handle, "OnboardClock", sampleRate(), DAQmx_Val_Rising,
				     DAQmx_Val_ContSamps, samplesperchan );
  if ( error != 0 ) {
    DAQmxBaseGetExtendedErrorInfo( errstr, 2048 );
    DAQmxBaseClearTask( Handle );
    Handle = 0;
    printlog( "NIDAQmxBase: ! error no." + Str( error ) + "[" + Str( errstr ) + "]" );
    return -3;
  }

  // DMA buffer size:
  int bufsize = samplesperchan*10*channels();
  printlog( "NIDAQmxBase buffer size: " + Str( bufsize ) );
  error = DAQmxBaseCfgInputBuffer( Handle, bufsize );
  if ( error != 0 ) {
    DAQmxBaseGetExtendedErrorInfo( errstr, 2048 );
    DAQmxBaseClearTask( Handle );
    Handle = 0;
    printlog( "NIDAQmxBase: ! error no." + Str( error ) + "[" + Str( errstr ) + "]" );
    return -4;
  }

  printlog( "NIDAQmxThread interval=" + Str( 1000.0*interval() ) + "ms" );

  // start acquisition:
  error = DAQmxBaseStartTask( Handle );
  if ( error != 0 ) {
    DAQmxBaseGetExtendedErrorInfo( errstr, 2048 );
    DAQmxBaseStopTask( Handle );
    DAQmxBaseClearTask( Handle );
    printlog( "NIDAQmxBase: ! error no." + Str( error ) + "[" + Str( errstr ) + "]" );
    return -5;
  }

  return 0;
}


void NIDAQmxThread::finish( void )
{
  DAQmxBaseStopTask( Handle );
  DAQmxBaseClearTask( Handle );
}


int NIDAQmxThread::read( void )
{
  // XXX add finite samples support

  // get data from daq driver:
  int32 pointstoread = (int32)::floor( interval()*sampleRate() );
  float64 timeout = 1.0;
  int buffersize = pointstoread*channels();
  float64 buffer[buffersize];
  int32 pointsread = 0;
  int32 error = DAQmxBaseReadAnalogF64( Handle, pointstoread, timeout,
					DAQmx_Val_GroupByScanNumber,
					buffer, buffersize, &pointsread, NULL );
  if ( error != 0 ) {
    char errstr[2048];
    DAQmxBaseGetExtendedErrorInfo( errstr, 2048 );
    DAQmxBaseStopTask( Handle );
    DAQmxBaseClearTask( Handle );
    printlog( "NIDAQmxBase: ! error no." + Str( error ) + "[" + Str( errstr ) + "]" );
    return -1;
  }

  // transfer data to buffer:
  /*
  lockAI();
  for ( int k=0; k<pointsread*channels(); k++ )
    inputBuffer().push( (float)(buffer[k]/gain()) );
  unlockAI();
  */

  // transfer data to input buffer:
  int bufferinx = 0;
  int buffern = pointsread*channels();
  do {
    int pn = 0;
    int m = inputBuffer().maxPush();
    float *fp = inputBuffer().pushBuffer();
    while ( m > 0 && bufferinx < buffern ) {
      *(fp++) = (float)(buffer[bufferinx++]/gain());
      pn++;
      m--;
    }
    lockAI();
    inputBuffer().push( pn );
    unlockAI();
  } while ( bufferinx < buffern );


  return 0;
}

