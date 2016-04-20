/*
  analyzer.cc
  Base class for different analyzer and plot methods

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

#include "basewidget.h"
#include "analyzer.h"


Analyzer::Analyzer( const string &name, BaseWidget *bw, QWidget *parent )
  : QWidget( parent ),
    BW( bw ),
    Cfg( name )
{
  setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
  setFocusPolicy( Qt::StrongFocus );
  Cfg.setDialogHeader( false );
  connect( &Cfg, SIGNAL( dialogAccepted() ), this, SLOT( callNotify() ) );
}


Analyzer::~Analyzer( void )
{
}


Options &Analyzer::opts( void )
{
  return Cfg;
}


const Options &Analyzer::opts( void ) const
{
  return Cfg;
}


void Analyzer::notify( void )
{
}


void Analyzer::callNotify( void )
{
  notify();
}


void Analyzer::dialog( void )
{
  Cfg.dialog();
}


int Analyzer::grid( void ) const
{
  return BW->Grid;
}


bool Analyzer::used( int g ) const
{
  return BW->Used[g];
}


int Analyzer::rows( int g ) const
{
  return BW->Rows[g];
}


int Analyzer::columns( int g ) const
{
  return BW->Columns[g];
}


int Analyzer::gridChannels( int g ) const
{
  return BW->GridChannels[g];
}


int Analyzer::rows( void ) const
{
  return BW->Rows[BW->Grid];
}


int Analyzer::columns( void ) const
{
  return BW->Columns[BW->Grid];
}


int Analyzer::gridChannels( void ) const
{
  return BW->GridChannels[BW->Grid];
}


int Analyzer::channels( void ) const
{
  return BW->Channels;
}


int Analyzer::row( int g ) const
{
  return BW->Row[g];
}


int Analyzer::column( int g ) const
{
  return BW->Column[g];
}


int Analyzer::row( void ) const
{
  return BW->Row[BW->Grid];
}


int Analyzer::column( void ) const
{
  return BW->Column[BW->Grid];
}


double Analyzer::maxVolts( void ) const
{
  return 1000.0*BW->MaxVolts;
}


string Analyzer::unit( void ) const
{
  return BW->Unit;
}


double Analyzer::sampleRate( void ) const
{
  return BW->SampleRate;
}


double Analyzer::dataTime( void ) const
{
  return BW->DataTime;
}


double Analyzer::dataInterval( void ) const
{
  return BW->DataInterval;
}


int Analyzer::displayMode( void ) const
{
  return BW->DisplayMode;
}


bool Analyzer::displayAll( void ) const
{
  return ( BW->DisplayMode == 1 );
}


bool Analyzer::displaySingle( void ) const
{
  return ( BW->DisplayMode == 0 );
}


bool Analyzer::displayMerged( void ) const
{
  return ( BW->DisplayMode == 2 );
}


void Analyzer::printlog( const string &message ) const
{
  BW->printlog( message );
}


#include "moc_analyzer.cc"
