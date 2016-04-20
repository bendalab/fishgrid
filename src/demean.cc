/*
  demean.cc
  PreProcessor implementation that removes mean values of all traces.

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

#include "demean.h"


DeMean::DeMean( int hotkey, BaseWidget *bw, QObject *parent )
  : PreProcessor( "DeMean", hotkey, bw, parent )
{
}


DeMean::~DeMean( void )
{
}


void DeMean::initialize( void )
{
}


string DeMean::process( deque< deque< SampleDataF > > data[] )
{
  for ( int g=0; g<maxGrids(); g++ ) {
    if ( used( g ) ) {
      for ( int r=0; r<rows( g ); r++ ) {
	for ( int c=0; c<columns( g ); c++ )
	  data[g][r][c] -= mean( data[g][r][c] );
      }
    }
  }
  return "de-mean";
}


#include "moc_demean.cc"
