/*
  spectra.cc
  Analyzer implementation that plots power spectra

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

#include <deque>
#include <QKeyEvent>
#include <QFont>
#include <QHBoxLayout>
#include <relacs/str.h>
#include <relacs/sampledata.h>
#include <relacs/eventdata.h>
#include <relacs/map.h>
#include "spectra.h"


Spectra::Spectra( BaseWidget *bw, QWidget *parent )
  : Analyzer( "Spectra", bw, parent )
{
  setLayout( new QHBoxLayout );
  layout()->addWidget( &AP );
  layout()->addWidget( &SP );
  AP.show();
  SP.hide();

  opts().addSelection( "Size", "Number of data points for FFT", "1024|64|128|256|512|1024|2048|4096|8192|16384|32768|65536|131072|262144|524288|1048576" );
  opts().addBoolean( "Overlap", "Overlap FFT windows", true );
  opts().addSelection( "Window", "FFT window function", "Hanning|Bartlett|Blackman|Blackman-Harris|Hamming|Hanning|Parzen|Square|Welch" );
  opts().addBoolean( "Decibel", "Plot decibel relative to maximum", true );
  opts().addNumber( "Decay", "Decay time constant for maximum power", 100.0, "iterations" );
  opts().addNumber( "FMin", "Minimum frequency shown", 0.0, 0.0, 1000000.0, 50.0, "Hz", "Hz", "%.0f" );
  opts().addNumber( "FMax", "Maximum frequency shown", 2000.0, 0.0, 1000000.0, 50.0, "Hz", "Hz", "%.0f" );
  opts().addBoolean( "Clip", "Remove clipped traces from merged spectrum", true );
}


Spectra::~Spectra( void )
{
}


void Spectra::notify( void )
{
  SpecSize = opts().integer( "Size" );
  Overlap = opts().boolean( "Overlap" );
  int win = opts().index( "Window" );
  switch ( win ) {
  case 0: Window = bartlett; break;
  case 1: Window = blackman; break;
  case 2: Window = blackmanHarris; break;
  case 3: Window = hamming; break;
  case 4: Window = hanning; break;
  case 5: Window = parzen; break;
  case 6: Window = square; break;
  case 7: Window = welch; break;
  default: Window = hanning;
  }
  Decibel = opts().boolean( "Decibel" );
  Decay = opts().number( "Decay" );
  FreqRangeMin = opts().number( "FMin" );
  FreqRangeMax = opts().number( "FMax" );
  Clip = opts().boolean( "Clip" );
}


void Spectra::initialize( void )
{
  // all spectra:
  AP.resize( gridChannels(), rows(), true );
  AP.disableMouse();

  // single spectrum:
  SP.setXLabel( "Frequency [Hz]" );

  MaxFreq = -1.0;
  MaxGridPower = 0.0;
  MaxPower = 1e-8;
  PowerRange = MaxPower;
  ZoomedPower = false;
  MinDecibel = -10.0;
  DecibelRange = MinDecibel;
  ZoomedDecibel = false;
}


void Spectra::display( int mode, int grid )
{
  if ( mode == 1 ) {
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
	    AP[p].setXLabel( "Frequency [Hz]" );
	}
	else if ( c == 0 ) {
	  AP[p].noXTics();
	  if ( r == 0 ) {
	    AP[p].setYLabelPos( -3.5, Plot::FirstMargin, 0.0, Plot::SecondMargin,
				Plot::Right, -90.0 );
	  }
	}
	else {
	  AP[p].noXTics();
	  AP[p].noYTics();
	}
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
  MaxGridPower = 0.0;
  MaxPower = 1e-8;
  PowerRange = MaxPower;
}


void Spectra::process( const deque< deque< SampleDataF > > data[] )
{
  double mmax = 0.0;
  double mmin = 0.0;

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
	SampleDataD d( data[grid()][r][c] );
	d -= mean( d );
	SampleDataD spec( SpecSize );
	rPSD( d, spec, Overlap, Window );
	double smax = max( spec );
	if ( smax > mmax )
	  mmax = smax;
	if ( Decibel )
	  spec.decibel( maxVolts()*maxVolts() );
	//	  spec.decibel( MaxGridPower );
	if ( MaxFreq < 0.0 || MaxFreq < spec.length() )
	  MaxFreq = spec.length();
	if ( FreqRangeMax > MaxFreq || FreqRangeMax < 0.0 )
	  FreqRangeMax = MaxFreq;
	if ( FreqRangeMax < FreqRangeMin )
	  FreqRangeMin = 0.0;
	AP[p].setXRange( FreqRangeMin, FreqRangeMax );
	if ( Decibel ) {
	  double smin = min( spec );
	  if ( smin < mmin )
	    mmin = smin;
	  AP[p].setYRange( DecibelRange, 0.0 );
	  if ( r == 0 && c == 0 )
	    AP[p].setYLabel( "Power [dB]" );
	}
	else {
	  AP[p].setYRange( 0.0, PowerRange );
	  if ( r == 0 && c == 0 )
	    AP[p].setYLabel( "Power [" + unit() + "^2/Hz]" );
	}
	AP[p].plot( spec, 1.0, Plot::Yellow, 2, Plot::Solid );
	if ( r == data[grid()].size()-1 && c == 0 )
	  AP[p].setLabel( "Grid " + Str(grid()+1), -5.0, Plot::FirstMargin,
			  -2.45, Plot::FirstMargin, Plot::Left,
			  0.0, Plot::Black, 1.8 );
	p++;
      }
    }
    AP.draw();
    MaxGridPower = mmax;
  }
  else if ( displayMerged() ) {
    SP.setFontSize( 0.05*height() );
    SP.setLMarg( 5.0 );
    SP.clear();
    SP.setLabel( "Grid " + Str(grid()+1), 0.0, Plot::Screen,
		 0.0, Plot::Screen, Plot::Left,
		 0.0, Plot::Black, 1.8 );
    SampleDataD sumspec( SpecSize );
    sumspec = 0.0;
    int p = 0;
    for ( unsigned int r=0; r<data[grid()].size(); r++ ) {
      for ( unsigned int c=0; c<data[grid()][r].size(); c++ ) {
	if ( Clip ) {
	  bool skip = false;
	  int nc = 0;
	  for ( int k=0; k<data[grid()][r][c].size(); k++ ) {
	    // 0.999: we have 16 bit, that makes 32000 data points between 0 and maxVolts
	    // so clipped data are above 1-1/32000 = 0.99996875
	    if ( fabs( data[grid()][r][c][k] ) > 0.999*maxVolts() ) {
	      nc++;
	      if ( nc > 1 ) {
		skip = true;
		break;
	      }
	    }
	    else
	      nc = 0;
	  }
	  if ( skip )
	    continue;
	}
	SampleDataD d( data[grid()][r][c] );
	d -= mean( d );
	SampleDataD spec( SpecSize );
	rPSD( d, spec, Overlap, Window );
	sumspec += spec;
	p++;
      }
    }
    if ( p > 0 )
      sumspec /= p;
    if ( Decibel )
      sumspec.decibel( maxVolts()*maxVolts() );
    if ( MaxFreq < 0.0 || MaxFreq < sumspec.length() )
      MaxFreq = sumspec.length();
    if ( FreqRangeMax > MaxFreq || FreqRangeMax < 0 )
      FreqRangeMax = MaxFreq;
    if ( FreqRangeMax < FreqRangeMin )
      FreqRangeMin = 0.0;
    if ( ! SP.zoomedXRange() )
      SP.setXRange( FreqRangeMin, FreqRangeMax );
    if ( Decibel ) {
      double smin = min( sumspec );
      if ( smin < mmin )
	mmin = smin;
      SP.setYRange( DecibelRange, 0.0 );
      SP.setYLabel( "Power [dB]" );
    }
    else {
      double smax = max( sumspec );
      if ( smax > mmax )
	mmax = smax;
      SP.setYRange( 0.0, PowerRange );
      SP.setYLabel( "Power [" + unit() + "^2/Hz]" );
    }
    fishDetector( sumspec, SP );
    SP.plot( sumspec, 1.0, Plot::Orange, 2, Plot::Solid );
    SP.draw();
  }
  else {
    // single trace:
    SP.setFontSize( 0.05*height() );
    SP.setLMarg( 5.0 );
    SP.clear();
    SP.setLabel( Str(row()*columns()+column()+1), 0.05, Plot::GraphX,
		 0.05, Plot::GraphY, Plot::Left,
		 0.0, Plot::White, 2.0 );
    SP.setLabel( "(" + Str(row()+1) + "|" + Str(column()+1) + ")", 0.05, Plot::GraphX,
		 0.85, Plot::GraphY, Plot::Left,
		 0.0, Plot::White, 2.0 );
    SP.setLabel( "Grid " + Str(grid()+1), 0.0, Plot::Screen,
		 0.0, Plot::Screen, Plot::Left,
		 0.0, Plot::Black, 1.8 );
    SampleDataD d( data[grid()][row()][column()] );
    d -= mean( d );
    SampleDataD spec( SpecSize );
    rPSD( d, spec, Overlap, Window );
    // ranges:
    if ( Decibel )
      spec.decibel( maxVolts()*maxVolts() );
    if ( MaxFreq < 0.0 || MaxFreq < spec.length() )
      MaxFreq = spec.length();
    if ( FreqRangeMax > MaxFreq || FreqRangeMax < 0 )
      FreqRangeMax = MaxFreq;
    if ( FreqRangeMax < FreqRangeMin )
      FreqRangeMin = 0.0;
    if ( ! SP.zoomedXRange() )
      SP.setXRange( FreqRangeMin, FreqRangeMax );
    if ( Decibel ) {
      double smin = min( spec );
      if ( smin < mmin )
	mmin = smin;
      SP.setYRange( DecibelRange, 0.0 );
      SP.setYLabel( "Power [dB]" );
    }
    else {
      double smax = max( spec );
      if ( smax > mmax )
	mmax = smax;
      SP.setYRange( 0.0, PowerRange );
      SP.setYLabel( "Power [" + unit() + "^2/Hz]" );
    }
    fishDetector( spec, SP );
    // plot:
    SP.plot( spec, 1.0, Plot::Yellow, 2, Plot::Solid );
    SP.draw();
  }

  if ( Decibel ) {
    if ( mmin < MinDecibel + 0.5 )
      MinDecibel = mmin;
    else
      MinDecibel += -MinDecibel/Decay;
    if ( ! ZoomedDecibel || DecibelRange < MinDecibel )
      DecibelRange = MinDecibel;
  }
  else {
    if ( mmax > 0.98*MaxPower )
      MaxPower = mmax;
    else
      MaxPower += -MaxPower/Decay;
    if ( ! ZoomedPower || PowerRange > MaxPower )
      PowerRange = MaxPower;
  }
}


void Spectra::fishDetector( const SampleDataD &spec, Plot &p )
{
  if ( Decibel ) {
    double threshold = 8.0; // decibel
    double maxfreqdiff = 1.5*spec.stepsize(); // Hz
    EventData freqevents( 1000, true );
    peaks( spec, freqevents, threshold );
    MapD freqs( freqevents );
    deque< MapD > fishfreqs;
    // remove frequencies below 20 Hz:
    while ( freqs.size() > 0 && freqs.x( 0 ) < 20.0 )
      freqs.erase( 0 );
    while ( freqs.size() > 0 ) {
      int peakinx = maxIndex( freqs.y() );
      double basefreq = freqs.x( peakinx );
      MapD fish;
      fish.push( freqs.x( peakinx ), freqs.y( peakinx ) );
      freqs.erase( peakinx );
      for ( int k=peakinx; k<freqs.size(); k++ ) {
	double nf = ::round( freqs.x(k)/basefreq );
	if ( ::fabs( freqs.x(k)/nf - basefreq ) < maxfreqdiff ) {
	  fish.push( freqs.x( k ), freqs.y( k ) );
	  freqs.erase( k );
	  if ( fish.size() > 10 )
	    break;
	}
      }
      fishfreqs.push_back( fish );
    }
    Plot::Color colors[3] = { Plot::White, Plot::OrangeRed, Plot::Green };
    for ( unsigned int k=0; k<3 && k<fishfreqs.size(); k++ ) {
      p.plot( fishfreqs[k], 1.0, Plot::Transparent, 0, Plot::Solid, 
	      Plot::Circle, 10, colors[k], colors[k] );
      double freq = fishfreqs[k].x( 0 );
      double peak = fishfreqs[k].y( 0 );
      p.setLabel( Str( freq, 0, 0, 'f' ) + "Hz", freq+10.0, Plot::First, 
		  peak, Plot::First, Plot::Left, 0.0, colors[k] );
    }
  }
  else {
    double peak=0.0;
    double peakinx = spec.maxIndex( peak, 70.0, 2000.0 ); // peak within 70 to 2000 Hz
    double peakpos = spec.pos( peakinx );
    p.setLabel( Str( peakpos, 0, 0, 'f' ) + "Hz", peakpos+20.0, peak - (Decibel ? 10.0 : 0.0) );
  }
}


void Spectra::zoomFreqIn( void )
{
  double df = FreqRangeMax - FreqRangeMin;
  df *= 0.5;
  FreqRangeMax = FreqRangeMin + df;
}


void Spectra::zoomFreqOut( void )
{
  double df = FreqRangeMax - FreqRangeMin;
  df *= 2.0;
  FreqRangeMax = FreqRangeMin + df;
  if ( FreqRangeMax > MaxFreq )
    FreqRangeMax = MaxFreq;
}


void Spectra::zoomPowerIn( void )
{
  if ( Decibel ) {
    DecibelRange *= 0.9;
    ZoomedDecibel = true;
  }
  else {
    PowerRange *= 0.5;
    ZoomedPower = true;
  }
}


void Spectra::zoomPowerOut( void )
{
  if ( Decibel ) {
    DecibelRange *= 2.0;
    if ( DecibelRange < MinDecibel ) {
      DecibelRange = MinDecibel;
      ZoomedDecibel = false;
    }
  }
  else {
    PowerRange *= 2.0;
    if ( PowerRange > MaxPower ) {
      PowerRange = MaxPower;
      ZoomedPower = false;
    }
  }
}


void Spectra::keyPressEvent( QKeyEvent *event )
{
  switch ( event->key() ) {

  case Qt::Key_D :
    Decibel = ! Decibel;
    break;

  case Qt::Key_V :
  case Qt::Key_Y :
    if ( event->modifiers() & Qt::ShiftModifier )
      zoomPowerIn();
    else
      zoomPowerOut();
    break;

  case Qt::Key_Plus :
  case Qt::Key_Equal :
    zoomFreqIn();
    break;
  case Qt::Key_Minus :
    zoomFreqOut();
    break;
  case Qt::Key_X :
  case Qt::Key_F :
    if ( event->modifiers() & Qt::ShiftModifier )
      zoomFreqIn();
    else
      zoomFreqOut();
    break;

  default:
    event->ignore();
    return;
  }

  event->accept();
}


#include "moc_spectra.cc"
