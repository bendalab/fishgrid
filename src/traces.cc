/*
  traces.cc
  Analyzer implementation that plots voltage traces

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
#include "traces.h"


Traces::Traces( BaseWidget *bw, QWidget *parent )
  : Analyzer( "Traces", bw, parent )
{
  setLayout( new QHBoxLayout );
  layout()->addWidget( &AP );
  layout()->addWidget( &SP );
  AP.show();
  SP.hide();
}


Traces::~Traces( void )
{
}


void Traces::initialize( void )
{
  TimeWindow = dataTime();
  LinkTimeWindow = true;
  VoltRange = maxVolts();

  // all traces:
  AP.resize( gridChannels(), rows(), true );
  AP.disableMouse();

  // single trace:
  SP.setXLabel( "Time [ms]" );
  SP.setYLabel( "Voltage [" + unit() + "]" );
}


void Traces::display( int mode, int grid )
{
  if ( mode == 1 ) {
    AP.hide();
    AP.resize( gridChannels(), rows(), true );
    int p=0;
    for ( int r=0; r<rows(); r++ ) {
      for ( int c=0; c<columns(); c++ ) {
	AP[p].clear();
	AP[p].setLMarg( 0.5 );
	AP[p].setRMarg( 0.0 );
	AP[p].setTMarg( 0.0 );
	AP[p].setBMarg( 0.5 );
	AP[p].setXTics();
	AP[p].setYTics();
	AP[p].setXLabel( "" );
	AP[p].setYLabel( "" );
	if ( (int)r == rows()-1 && c == 0 ) {
	}
	else if ( (int)r == rows()-1 ) {
	  AP[p].noYTics();
	  if ( (int)c == columns()-1 )
	    AP[p].setXLabel( "Time [ms]" );
	}
	else if ( c == 0 ) {
	  AP[p].noXTics();
	  if ( r == 0 ) {
	    AP[p].setYLabel( "Voltage [" + unit() + "]" );
	    AP[p].setYLabelPos( -3.5, Plot::FirstMargin, 0.0, Plot::SecondMargin,
				Plot::Right, -90.0 );
	  }
	}
	else {
	  AP[p].noXTics();
	  AP[p].noYTics();
	}
	AP[p].setXRange( 0.0, 1000.0*TimeWindow );
	AP[p].setYRange( -VoltRange, VoltRange );
	p++;
      }
    }
    SP.hide();
    AP.show();
  }
  else {
    AP.hide();
    SP.show();
  }
}


void Traces::process( const deque< deque< SampleDataF > > data[] )
{
  if ( LinkTimeWindow || TimeWindow > dataTime() )
    TimeWindow = dataTime();

  if ( displayAll() ) {
    double fw = fontMetrics().width( "00" ) - fontMetrics().width( "0" );
    double xo = 5.0*fw/width();
    double yo = 4.0*fw/height();
    double dx = (1.0 - xo)/columns();
    double dy = (1.0 - yo)/rows();
    int p=0;
    for ( unsigned int r=0; r<data[grid()].size(); r++ ) {
      for ( unsigned int c=0; c<data[grid()][r].size(); c++ ) {
	AP[p].setSize( dx, dy );
	AP[p].setOrigin( xo + c*dx, yo + (rows()-r-1)*dy );
	if ( (int)r == row() && (int)c == column() )
	  AP[p].setBackgroundColor( Plot::Red );
	else
	  AP[p].setBackgroundColor( Plot::WidgetBackground );
	AP[p].clear();
	AP[p].setLabel( Str(r*columns()+c+1), 0.05, Plot::GraphX,
			0.05, Plot::GraphY, Plot::Left,
			0.0, Plot::White, 0.015*height()/rows() );
	AP[p].setXRange( 0.0, 1000.0*TimeWindow );
	AP[p].setYRange( -VoltRange, VoltRange );
	AP[p].plot( data[grid()][r][c], 1000.0, Plot::Green, 2 );
	if ( r == data[grid()].size()-1 && c == 0 )
	  AP[p].setLabel( "Grid " + Str(grid()+1), -5.0, Plot::FirstMargin,
			  -2.45, Plot::FirstMargin, Plot::Left,
			  0.0, Plot::Black, 1.8 );
	p++;
      }
    }
    AP.draw();
  }
  else {
    SP.setFontSize( 0.05*height() );
    SP.setLMarg( 5.0 );
    SP.clear();
    SP.setLabel( Str(row()*columns()+column()+1), 0.05, Plot::GraphX,
		 0.05, Plot::GraphY, Plot::Left,
		 0.0, Plot::White, 1.8 );
    SP.setLabel( "(" + Str(row()+1) + "|" + Str(column()+1) + ")", 0.05, Plot::GraphX,
		 0.85, Plot::GraphY, Plot::Left,
		 0.0, Plot::White, 1.8 );
    SP.setLabel( "Grid " + Str(grid()+1), 0.0, Plot::Screen,
		 0.0, Plot::Screen, Plot::Left,
		 0.0, Plot::Black, 1.8 );
    SP.setXRange( 0.0, 1000.0*TimeWindow );
    SP.setYRange( -VoltRange, VoltRange );
    SP.plot( data[grid()][row()][column()], 1000.0, Plot::Green, 2, Plot::Solid );
    //	  Plot::Circle, 10, Plot::Green, Plot::Green );
    SP.draw();
  }
}


void Traces::zoomTimeIn( void )
{
  LinkTimeWindow = false;
  TimeWindow *= 0.5;
}


void Traces::zoomTimeOut( void )
{
  TimeWindow *= 2.0;
  if ( TimeWindow > dataTime() )
    TimeWindow = dataTime();
}


void Traces::zoomVoltIn( void )
{
  VoltRange *= 0.5;
}


void Traces::zoomVoltOut( void )
{
  VoltRange *= 2.0;
  if ( VoltRange > maxVolts() )
    VoltRange = maxVolts();
}


void Traces::keyPressEvent( QKeyEvent *event )
{
  switch ( event->key() ) {

  case Qt::Key_V :
  case Qt::Key_Y :
    if ( event->modifiers() & Qt::ShiftModifier )
      zoomVoltIn();
    else
      zoomVoltOut();
    break;

  case Qt::Key_Plus :
  case Qt::Key_Equal :
    if ( LinkTimeWindow ) {
      event->ignore();
      return;
    }
    else
      zoomTimeIn();
    break;
  case Qt::Key_Minus :
    if ( LinkTimeWindow ) {
      event->ignore();
      return;
    }
    else
      zoomTimeOut();
    break;
  case Qt::Key_X :
    if ( event->modifiers() & Qt::ShiftModifier )
      zoomTimeIn();
    else
      zoomTimeOut();
    break;

  case Qt::Key_D :
    LinkTimeWindow = ! LinkTimeWindow;
    break;

  default:
    event->ignore();
    return;
  }

  event->accept();
}


#include "moc_traces.cc"
