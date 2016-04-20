/*
  preprocessor.h
  Base class for preprocessor modules

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
#include "preprocessor.h"


PreProcessor::PreProcessor( const string &name, int hotkey,
			    BaseWidget *bw, QObject *parent )
  : QObject( parent ),
    BW( bw ),
    Cfg( name ),
    HotKey( hotkey )
{
  Cfg.setDialogHeader( false );
  connect( &Cfg, SIGNAL( dialogAccepted() ), this, SLOT( callNotify() ) );
}


PreProcessor::~PreProcessor( void )
{
}


string PreProcessor::name( void ) const
{
  return Cfg.name();
}


int PreProcessor::hotkey( void ) const
{
  return HotKey;
}


Options &PreProcessor::opts( void )
{
  return Cfg;
}


const Options &PreProcessor::opts( void ) const
{
  return Cfg;
}


void PreProcessor::notify( void )
{
}


void PreProcessor::callNotify( void )
{
  notify();
}


void PreProcessor::dialog( void )
{
  Cfg.dialog();
}


int PreProcessor::maxGrids( void ) const
{
  return BW->MaxGrids;
}


int PreProcessor::grid( void ) const
{
  return BW->Grid;
}


bool PreProcessor::used( int g ) const
{
  return BW->Used[g];
}


int PreProcessor::rows( int g ) const
{
  return BW->Rows[g];
}


int PreProcessor::columns( int g ) const
{
  return BW->Columns[g];
}


int PreProcessor::gridChannels( int g ) const
{
  return BW->GridChannels[g];
}


int PreProcessor::rows( void ) const
{
  return BW->Rows[BW->Grid];
}


int PreProcessor::columns( void ) const
{
  return BW->Columns[BW->Grid];
}


int PreProcessor::gridChannels( void ) const
{
  return BW->GridChannels[BW->Grid];
}


int PreProcessor::channels( void ) const
{
  return BW->Channels;
}


int PreProcessor::row( int g ) const
{
  return BW->Row[g];
}


int PreProcessor::column( int g ) const
{
  return BW->Column[g];
}


int PreProcessor::row( void ) const
{
  return BW->Row[BW->Grid];
}


int PreProcessor::column( void ) const
{
  return BW->Column[BW->Grid];
}


double PreProcessor::maxVolts( void ) const
{
  return 1000.0*BW->MaxVolts;
}


string PreProcessor::unit( void ) const
{
  return BW->Unit;
}


double PreProcessor::sampleRate( void ) const
{
  return BW->SampleRate;
}


double PreProcessor::dataTime( void ) const
{
  return BW->DataTime;
}


double PreProcessor::dataInterval( void ) const
{
  return BW->DataInterval;
}


void PreProcessor::printlog( const string &message ) const
{
  BW->printlog( message );
}


#include "moc_preprocessor.cc"
