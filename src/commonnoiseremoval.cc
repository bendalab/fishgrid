/*
  commonnoiseremoval.cc
  PreProcessor implementation that removes common noise from each trace.

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

#include "commonnoiseremoval.h"


CommonNoiseRemoval::CommonNoiseRemoval( int hotkey,
					BaseWidget *bw, QObject *parent )
  : PreProcessor( "CommonNoiseRemoval", hotkey, bw, parent ),
    RemoveCommonNoise( 0 ),
    FirstGrid( 0 )
{
  opts().addSelection( "CommonNoiseRemoval", "Remove common noise from", "none|each grid|all grids" );
}


CommonNoiseRemoval::~CommonNoiseRemoval( void )
{
}


void CommonNoiseRemoval::notify( void )
{
  RemoveCommonNoise = opts().index( "CommonNoiseRemoval" );
  printlog( "Remove common noise = " + Str( RemoveCommonNoise ) );
}


void CommonNoiseRemoval::initialize( void )
{
  for ( int g=0; g<maxGrids(); g++ ) {
    if ( used( g ) ) {
      FirstGrid = g;
      break;
    }
  }
}


string CommonNoiseRemoval::process( deque< deque< SampleDataF > > data[] )
{
  // remove common noise in each grid:
  if ( RemoveCommonNoise == 1 ) {
    for ( int g=0; g<maxGrids(); g++ ) {
      if ( used( g ) ) {
	for ( int i=0; i<data[g][0][0].size(); i++ ) {
	  double m = 0.0;
	  int k = 0;
	  for ( int r=0; r<rows( g ); r++ ) {
	    for ( int c=0; c<columns( g ); c++ )
	      m += ( data[g][r][c][i] -m )/(++k);
	  }
	  for ( int r=0; r<rows( g ); r++ ) {
	    for ( int c=0; c<columns( g ); c++ )
	      data[g][r][c][i] -= m;
	  }
	}
      }
    }
    return "com. noise rem.";
  }
  // remove common noise for all grids:
  else if ( RemoveCommonNoise == 2 ) {
    for ( int i=0; i<data[FirstGrid][0][0].size(); i++ ) {
      double m = 0.0;
      int k = 0;
      for ( int g=0; g<maxGrids(); g++ ) {
	if ( used( g ) ) {
	  for ( int r=0; r<rows( g ); r++ ) {
	    for ( int c=0; c<columns( g ); c++ )
	      m += ( data[g][r][c][i] -m )/(++k);
	  }
	}
      }
      for ( int g=0; g<maxGrids(); g++ ) {
	if ( used( g ) ) {
	  for ( int r=0; r<rows( g ); r++ ) {
	    for ( int c=0; c<columns( g ); c++ )
	      data[g][r][c][i] -= m;
	  }
	}
      }
    }
    return "com. gid-noise rem.";
  }
  return "";
}


#include "moc_commonnoiseremoval.cc"
