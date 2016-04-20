/*
  rmsplot.cc
  Analyzer implementation that plots the RMS voltage as a function of column number

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

#include <QKeyEvent>
#include <QFont>
#include <QHBoxLayout>
#include <relacs/str.h>
#include <relacs/map.h>
#include "rmsplot.h"


RMSPlot::RMSPlot( BaseWidget *bw, QWidget *parent )
  : Analyzer( "RMSPlot", bw, parent )
{
  setLayout( new QHBoxLayout );
  layout()->addWidget( &AP );
  layout()->addWidget( &SP );
  AP.show();
  SP.hide();
}


RMSPlot::~RMSPlot( void )
{
}


void RMSPlot::initialize( void )
{
  VoltRange = maxVolts();

  // all rmsplot:
  AP.resize( rows() );
  AP.disableMouse();

  // single trace:
  SP.setXTics( 1.0 );
  SP.setXRange( 0.5, columns()+0.5 );
  SP.setXLabel( "Electrode" );
  SP.setYLabel( "RMS Voltage [" + unit() + "]" );
}


void RMSPlot::display( int mode, int grid )
{
  if ( mode == 1 ) {
    AP.resize( rows() );
    for ( int r=0; r<rows(); r++ ) {
      AP[r].clear();
      AP[r].setLMarg( 5.0 );
      AP[r].setRMarg( 0.5 );
      AP[r].setTMarg( 0.5 );
      AP[r].setBMarg( 0.5 );
      AP[r].setXTics();
      AP[r].setYTics();
      AP[r].setXLabel( "" );
      AP[r].setYLabel( "" );
      if ( (int)r == rows()-1 )
	AP[r].setXLabel( "Electrode" );
      if ( r == 0 ) {
	AP[r].setYLabel( "RMS Voltage [" + unit() + "]" );
	AP[r].setYLabelPos( 1.0, Plot::FirstMargin, 0.0, Plot::SecondMargin,
			    Plot::Right, -90.0 );
      }
      AP[r].setXTics( 1.0 );
      AP[r].setXRange( 0.5, columns()+0.5 );
      AP[r].setYRange( -VoltRange, VoltRange );
    }
    SP.hide();
    AP.show();
  }
  else {
    AP.hide();
    SP.show();
  }
}


void RMSPlot::process( const deque< deque< SampleDataF > > data[] )
{
  if ( displayAll() ) {
    double fw = fontMetrics().width( "00" ) - fontMetrics().width( "0" );
    double yo = 4.0*fw/height();
    double dy = (1.0 - yo)/rows();
    for ( unsigned int r=0; r<data[grid()].size(); r++ ) {
      AP[r].clear();
      AP[r].setSize( 1.0, dy );
      AP[r].setOrigin( 0.0, yo + (rows()-r-1)*dy );
      if ( (int)r == row() )
	AP[r].setBackgroundColor( Plot::Red );
      else
	AP[r].setBackgroundColor( Plot::WidgetBackground );
      AP[r].setLabel( Str(r*columns())+"-"+Str((r+1)*columns()-1), 0.05, Plot::GraphX,
		      0.05, Plot::GraphY, Plot::Left,
		      0.0, Plot::White, 0.005*height() );
      AP[r].setYRange( 0.0, VoltRange );
      MapF rms;
      rms.reserve( columns() );
      for ( int c=0; c<columns(); c++ )
	rms.push( c+1.0, relacs::stdev( data[grid()][r][c] ) );
      AP[r].plot( rms, 1.0, Plot::Orange, 2, Plot::Solid,
		  Plot::Circle, 10, Plot::Orange, Plot::Orange );
      if ( r == data[grid()].size()-1 )
	AP[r].setLabel( "Grid " + Str(grid()+1), 0.0, Plot::FirstMargin,
			-2.45, Plot::FirstMargin, Plot::Left,
			0.0, Plot::Black, 1.8 );
    }
    AP.draw();
  }
  else {
    SP.setFontSize( 0.05*height() );
    SP.setLMarg( 5.0 );
    SP.clear();
    SP.setLabel( Str(row()*columns())+"-"+Str((row()+1)*columns()-1), 0.05, Plot::GraphX,
		 0.05, Plot::GraphY, Plot::Left,
		 0.0, Plot::White, 2.0 );
    SP.setLabel( "(" + Str(row()+1) + "|*)", 0.05, Plot::GraphX,
		 0.85, Plot::GraphY, Plot::Left,
		 0.0, Plot::White, 2.0 );
    SP.setLabel( "Grid " + Str(grid()+1), 0.0, Plot::Screen,
		 0.0, Plot::Screen, Plot::Left,
		 0.0, Plot::Black, 1.8 );
    SP.setYRange( 0.0, VoltRange );
    MapF rms;
    rms.reserve( columns() );
    for ( int c=0; c<columns(); c++ )
      rms.push( c+1.0, relacs::stdev( data[grid()][row()][c] ) );
    SP.plot( rms, 1.0, Plot::Orange, 2, Plot::Solid,
	     Plot::Circle, 10, Plot::Orange, Plot::Orange );
    SP.draw();
  }
}


void RMSPlot::zoomVoltIn( void )
{
  VoltRange *= 0.5;
}


void RMSPlot::zoomVoltOut( void )
{
  VoltRange *= 2.0;
  if ( VoltRange > maxVolts() )
    VoltRange = maxVolts();
}


void RMSPlot::keyPressEvent( QKeyEvent *event )
{
  switch ( event->key() ) {

  case Qt::Key_V :
  case Qt::Key_Y :
    if ( event->modifiers() & Qt::ShiftModifier )
      zoomVoltIn();
    else
      zoomVoltOut();
    break;

  default:
    event->ignore();
    return;
  }

  event->accept();
}


#include "moc_rmsplot.cc"
