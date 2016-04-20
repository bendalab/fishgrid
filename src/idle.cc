/*
  idle.cc
  Analyzer implementation that does nothing

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

#include "idle.h"


Idle::Idle( BaseWidget *bw, QWidget *parent )
  : Analyzer( "Idle", bw, parent )
{
}


Idle::~Idle( void )
{
}


void Idle::initialize( void )
{
}


void Idle::display( int mode, int grid )
{
}


void Idle::process( const deque< deque< SampleDataF > > data[] )
{
}


#include "moc_idle.cc"
