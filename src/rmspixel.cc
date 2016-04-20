/*
  rmspixel.cc
  Analyzer implementation that plots for each electrode a box that is colored according to the rms signal strength

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
#include <QWidget>
#include <QPainter>
#include <QResizeEvent>
#include <relacs/stats.h>
#include "rmspixel.h"


RMSPixel::RMSPixel( BaseWidget *bw, QWidget *parent )
  : Analyzer( "RMSPixel", bw, parent ),
    PixMap( 0 ),
    PlotTrace( true )
{
  opts().addNumber( "TraceLength", 10.0, "s" );
  setAttribute( Qt::WA_OpaquePaintEvent );
}


RMSPixel::~RMSPixel( void )
{
  if ( PixMap != 0 )
    delete PixMap;
}


void RMSPixel::notify( void )
{
  TraceSize = (int)::ceil( opts().number( "TraceLength" )/dataInterval() );
  XPosition.reserve( TraceSize );
  YPosition.reserve( TraceSize );
  if ( CurrentTraceSize > TraceSize )
    CurrentTraceSize = TraceSize;
}


void RMSPixel::initialize( void )
{
  TraceSize = (int)::ceil( opts().number( "TraceLength" )/dataInterval() );
  XPosition.reserve( TraceSize );
  YPosition.reserve( TraceSize );
  CurrentTraceSize = 0;
}


void RMSPixel::display( int mode, int grid )
{
}


void RMSPixel::process( const deque< deque< SampleDataF > > data[] )
{
  // compute rms:
  double max = 0.0;
  int rmax = 0;
  int cmax = 0;
  double rms[data[grid()].size()][data[grid()][0].size()];
  for ( unsigned int r=0; r<data[grid()].size(); r++ ) {
    for ( unsigned int c=0; c<data[grid()][r].size(); c++ ) {
      rms[r][c] = relacs::stdev( data[grid()][r][c] );
      if ( rms[r][c] >= max ) {
	max = rms[r][c];
	rmax = r;
	cmax = c;
      }
    }
  }

  // draw pixels:
  QPainter paint( PixMap );
  int dx = PixMap->width()/data[grid()][0].size();
  int dy = PixMap->height()/data[grid()].size();
  int dxm = dx + PixMap->width() - data[grid()][0].size()*dx;
  int dym = dy + PixMap->height() - data[grid()].size()*dy;
  int wx = dx;
  int wy = dy;
  QFont font( paint.font() );
  font.setPixelSize( dy/3 );
  paint.setFont( font );
  for ( unsigned int r=0; r<data[grid()].size(); r++ ) {
    if ( r == data[grid()].size() - 1 )
      wy = dym;
    for ( unsigned int c=0; c<data[grid()][r].size(); c++ ) {
      if ( c < data[grid()].size() - 1 )
	wx = dx;
      else
	wx = dxm;
      paint.fillRect( c*dx, r*dy, wx, wy, QBrush( map( rms[r][c] ) ) );
      paint.drawText( c*dx+dx/10, r*dy+9*dy/10,
		      QString().setNum(r*columns()+c) );
    }
  }
  paint.drawText( dx/10, 3*dy/10,
		  QString( string( "Grid " + Str( grid()+1 ) ).c_str() ) );

  // draw maximum pixel:
  paint.setBrush( Qt::black );
  int r = dx < dy ? dx : dy;
  r /= 20;
  if ( r < 1 )
    r = 1;
  //  paint.drawEllipse( cmax*dx+dx/2-r, rmax*dy+dy/2-r, 2*r, 2*r );

  // draw average position:
  double s = 0.0;
  double x = 0.0;
  double y = 0.0;
  for ( int r=-1; r<=1; r++ ) {
    if ( rmax+r >= 0 && rmax+r < (int)data[grid()].size() ) {
      for ( int c=-1; c<=1; c++ ) {
	if ( cmax+c >= 0 && cmax+c < (int)data[grid()][r].size() ) {
	  x += rms[rmax+r][cmax+c]*(cmax+c);
	  y += rms[rmax+r][cmax+c]*(rmax+r);
	  s += rms[rmax+r][cmax+c];
	}
      }
    }
  }
  x /= s;
  y /= s;
  r *= 2;
  if ( PlotTrace )
    paint.drawEllipse( dx/2+dx*x-r, dy/2+dy*y-r, 2*r, 2*r );
    
  // draw trace:
  XPosition.push( dx/2+dx*x );
  YPosition.push( dy/2+dy*y );
  if ( CurrentTraceSize < TraceSize )
    CurrentTraceSize++;
  if ( PlotTrace ) {
    QPen pen = QPen( paint.brush(), 4 );
    int inx = XPosition.size() - CurrentTraceSize;
    if ( inx < XPosition.minIndex() )
      inx = XPosition.minIndex();
    for ( int k=inx, n=0; k<XPosition.size()-1; k++, n++ ) {
      QColor color( Qt::black );
      if ( XPosition.size() >= TraceSize && CurrentTraceSize >= TraceSize &&
	   n <= TraceSize/10 )
	color.setAlphaF( 10.0*n/TraceSize );
      pen.setColor( color );
      paint.setPen( pen );
      paint.drawLine( XPosition[k], YPosition[k],
		      XPosition[k+1], YPosition[k+1] );
    }
  }

  update();
}


QColor RMSPixel::map( double rms )
{
  QColor color;
  color.setHsv( 240+(int)floor(180.0*rms*sqrt(2.0)/maxVolts()), 255, 255 );
  return color;
}


void RMSPixel::resizeEvent( QResizeEvent *qre )
{
  Analyzer::resizeEvent( qre );

  if ( PixMap == 0 )
    delete PixMap;
  PixMap = new QPixmap( width(), height() );
  PixMap->fill( palette().color( QPalette::Window ) );
}


void RMSPixel::paintEvent( QPaintEvent *qpe )
{
  if ( PixMap != 0 ) {
    QPainter paint( this );
    paint.drawPixmap( 0, 0, *PixMap );
  }
}


void RMSPixel::keyPressEvent( QKeyEvent *event )
{
  switch ( event->key() ) {
  case Qt::Key_F :
    PlotTrace = ! PlotTrace;
    break;
  default:
    event->ignore();
    return;
  }

  event->accept();
}


#include "moc_rmspixel.cc"
