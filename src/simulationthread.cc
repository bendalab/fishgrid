/*
  simulationthread.cc
  DataThread implementation for simulating data of a moving fish

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

#include <cmath>
#include <relacs/random.h>
#include "simulationthread.h"


SimulationThread::SimulationThread( ConfigData *cd )
  : DataThread( "Simulation", cd ),
    FixedPos( false )
{
  addBoolean( "fixedpos", FixedPos );
}


int SimulationThread::initialize( double duration )
{
  FixedPos = boolean( "fixedpos" );

  Amplitude = 0.0;

  // init sine:
  double freq = 900.0;
  for ( int g=0; g<ConfigData::MaxGrids; g++ ) {
    if ( used( g ) ) {
      int n = (int)::rint( sampleRate()/freq );
      Sine[g].reserve( n );
      Sine[g].clear();
      for ( int k=0; k<n; k++ )
	Sine[g].push_back( sin( 2.0*M_PI*k/n ) );
      X[g] = 0.5*columns( g );
      Y[g] = 0.5*rows( g );
      freq += 50.0;
    }
  }

  Samples = 0;
  MaxSamples = 0;
  if ( duration > 0.0 )
    MaxSamples = (int)ceil( duration*sampleRate() );

  printlog( "SimulationThread 10ms" );

  return 0;
}


void SimulationThread::finish( void )
{
  for ( int g=0; g<ConfigData::MaxGrids; g++ )
    Sine[g].clear();
}


int SimulationThread::read( void )
{
  if ( MaxSamples > 0 && Samples >= MaxSamples )
    return 1;

  bool firstgrid = true;
  for ( int g=0; g<ConfigData::MaxGrids; g++ ) {
    if ( used( g ) ) {

      int n = (int)::floor( 0.01*sampleRate() ) / Sine[g].size();
      double buffer[n*gridChannels( g )*Sine[g].size()];
      int buffern = 0;

      if ( FixedPos ) {
	for ( int k=0; k<n; k++ ) {
	  for ( unsigned int j = 0; j<Sine[g].size(); j++ ) {
	    for ( int r=0; r<rows( g ); r++ ) {
	      for ( int c=0; c<columns( g ); c++ ) {
		if ( r == rows( g )/2 && c == columns( g )/2 )
		  buffer[buffern++] = Amplitude*Sine[g][j];
		else
		  buffer[buffern++] = 0.0;
	      }
	    }
	  }
	  Amplitude += 0.0001;
	  if ( Amplitude > maxVolts() )
	    Amplitude = 0.0;
	  if ( firstgrid )
	    Samples++;
	}
      }

      else {
	for ( int k=0; k<n; k++ ) {
	  X[g] += (0.5*columns( g ) + D*columns( g )*rnd.gaussian() - X[g])/Tau;
	  if ( X[g] >= 1.0+columns( g ) )
	    X[g] = 1.0+columns( g );
	  else if ( X[g] < 0.0 )
	    X[g] = 0.0;
	  Y[g] += (0.5*rows( g ) + D*rows( g )*rnd.gaussian() - Y[g])/Tau;
	  if ( Y[g] >= 1.0+rows( g ) )
	    Y[g] = 1.0+rows( g );
	  else if ( Y[g] < 0.0 )
	    Y[g] = 0.0;
	  for ( unsigned int j = 0; j<Sine[g].size(); j++ ) {
	    for ( int r=0; r<rows( g ); r++ ) {
	      for ( int c=0; c<columns( g ); c++ ) {
		double dx = X[g]-c;
		double dy = Y[g]-r;
		double r = dx*dx + dy*dy;
		float a = maxVolts()*0.1/r;
		if ( a > maxVolts() )
		  a = maxVolts();
		buffer[buffern++] = a*Sine[g][j];
	      }
	    }
	  }
	}
      }

      // transfer data to input buffer:
      int bufferinx = 0;
      do {
	int pn = 0;
	int m = inputBuffer( g ).maxPush();
	float *fp = inputBuffer( g ).pushBuffer();
	while ( m > 0 && bufferinx < buffern ) {
	  *(fp++) = (float)buffer[bufferinx++];
	  pn++;
	  m--;
	}
	lockAI( g );
	inputBuffer( g ).push( pn );
	unlockAI( g );
      } while ( bufferinx < buffern );

      firstgrid = false;

    }
  }

  QThread::msleep( (int)::floor( 1000.0*0.01 ) );

  return 0;
}

