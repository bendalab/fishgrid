/*
  fishgridwidget.cc
  BaseWidget implementation for coordinating data acquisition threads and analyzers

  FishGrid
  Copyright (C) 2012 Jan Benda <benda@bio.lmu.de> & Joerg Henninger <henninger@bio.lmu.de>

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
#include <QApplication>
#include <QTime>
#include <QTimer>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QShortcut>
#include <relacs/optwidget.h>
#include <relacs/optdialog.h>
#include "datathread.h"
#include "simulationthread.h"
#ifdef HAVE_COMEDILIB_H
#include "comedithread.h"
#endif
#ifdef HAVE_NIDAQMXBASE_H
#include "nidaqmxthread.h"
#endif
#include "preprocessor.h"
#include "analyzer.h"
#include "fishgridwidget.h"

using namespace std;


FishGridWidget::FishGridWidget( double samplerate,
				double maxvolts, double gain,
				double buffertime,
				double datatime, double datainterval,
				bool dialog, bool saving, const string &stoptime,
				bool simulate )
  : BaseWidget( "fishgrid.cfg" ),
    FileSaver( this ),
    AutoSave( saving ),
    DataLoop( 0 ),
    MetadataDialog( 0 ),
    TimeStampDialog( 0 )
{
  MaxVolts = maxvolts;
  SampleRate = samplerate;
  Gain = gain;
  Unit = "mV";
  DataTime = datatime;
  DataInterval = datainterval;
  BufferTime = buffertime;

  // stop time:
  StopRecording = ( ! stoptime.empty() );
  if ( StopRecording ) {
    QTime a = QTime::fromString( stoptime.c_str(), "hh:mm:ss" );
    QDateTime b = QDateTime::currentDateTime();
    StopTime = QDateTime( QDate( b.date() ), a );
    if ( StopTime < b )
      StopTime = StopTime.addDays( 1 );
  }

  connect( qApp, SIGNAL( aboutToQuit() ), this, SLOT( finish() ) );

  // set up the data thread:
  DataThread *acq = 0;
#ifdef HAVE_COMEDILIB_H
  acq = new ComediThread( this );
#else
#ifdef HAVE_NIDAQMXBASE_H
  acq = new NIDAQmxThread( this );
#endif
#endif
  if ( acq == 0 || simulate ) {
    if ( acq != 0 )
      delete acq;
    printlog( "using simulation" );
    DataLoop = new SimulationThread( this );
  }
  else {
    printlog( "using data aquisition board" );
    DataLoop = acq;
  }
  FileSaver.setDataThread( DataLoop );

  // read configuration:
  CFG.read();

  for ( int g=0; g<MaxGrids; g++ ) {
    string ns = Str( g+1 );
    Used[g] = boolean( "Used"+ns );
    if ( Used[g] ) {
      Rows[g] = integer( "Rows"+ns, 0, 4 );
      Columns[g] = integer( "Columns"+ns, 0, 4 );
    }
  }

  if ( SampleRate <= 0 )
    SampleRate = number( "AISampleRate", 10000.0 );
  setNumber( "AISampleRate", SampleRate );

  if ( MaxVolts <= 0 )
    MaxVolts = number( "AIMaxVolt", 1.0 );
  setNumber( "AIMaxVolt", MaxVolts );

  if ( Gain == 0 )
    Gain = number( "Gain", 1.0 );
  setNumber( "Gain", Gain );

  if ( BufferTime <= 0 )
    BufferTime = number( "BufferTime", 60.0 );
  setNumber( "BufferTime", BufferTime );

  if ( DataTime <= 0 )
    DataTime = number( "DataTime", 0.1 );
  setNumber( "DataTime", DataTime );

  if ( DataInterval <= 0 )
    DataInterval = number( "DataInterval", 1.0 );
  setNumber( "DataInterval", DataInterval );

  // dialog:
  if ( dialog ) {
    OptDialog d;
    d.setCaption( "FishGrid Configuration" );
    d.addOptions( *this, 1, 2, OptWidget::TabSectionStyle | OptWidget::BoldSectionsStyle ); // select, readonly
    d.setVerticalSpacing( 4 );
    d.setHorizontalSpacing( 4 );
    d.setMargins( 10 );
    d.addButton( "&Ok", OptDialog::Accept, 1 );
    d.addButton( "&Cancel" );
    int r = d.exec();
    if ( r != 1 )
      exit( 0 );

    for ( int g=0; g<MaxGrids; g++ ) {
      string ns = Str( g+1 );
      Used[g] = boolean( "Used"+ns );
      if ( Used[g] ) {
	Rows[g] = integer( "Rows"+ns, 0, 4 );
	Columns[g] = integer( "Columns"+ns, 0, 4 );
      }
    }
    SampleRate = number( "AISampleRate", SampleRate );
    MaxVolts = number( "AIMaxVolt", MaxVolts );
    Gain = number( "Gain", Gain );
    BufferTime = number( "BufferTime", BufferTime );
    DataTime = number( "DataTime", DataTime );
    DataInterval = number( "DataInterval", DataInterval );
  }

  // disable grids that are not used:
  for ( int g=0; g<MaxGrids; g++ ) {
    string ns = Str( g+1 );
    if ( ! Used[g] ) {
      // not in meta data dialog:
      section( "Grid &"+ns ).delFlags( 4 );
      // not in configuration file:
      section( "Grid &"+ns ).delFlags( 16 );
    }
  }

  // write configuration to console:
  Channels = 0;
  Grid = -1;
  for ( int g=0; g<MaxGrids; g++ ) {
    if ( Used[g] ) {
      printlog( "Grid " + Str( g+1 ) );
      printlog( "  Rows=" +Str( Rows[g] ) );
      printlog( "  Columns=" +Str( Columns[g] ) );
      GridChannels[g] = Rows[g]*Columns[g];
      printlog( "  Channels=" +Str( GridChannels[g] ) );
      Channels += GridChannels[g];
      if ( Grid < 0 )
	Grid = g;
    }
  }
  setInteger( "AIUsedChannelCount", Channels );
  printlog( "Channels=" +Str( Channels ) );
  printlog( "SampleRate=" +Str( SampleRate ) + "Hz" );
  printlog( "MaxVolts=" +Str( MaxVolts, "%.3f" ) + "V" );
  printlog( "Gain=" +Str( Gain ) );
  printlog( "BufferTime=" +Str( BufferTime ) + "s" );
  printlog( "DataTime=" +Str( DataTime ) + "s" );
  printlog( "DataInterval=" +Str( DataInterval ) + "s" );

  setup();

  // start acquisition:
  QTimer::singleShot( 200, this, SLOT( start() ) );
}


FishGridWidget::~FishGridWidget( void )
{
}


void FishGridWidget::start( void )
{
  printlog( "start acquisition" );
  int r = DataLoop->start();
  if ( r != 0 ) {
    printlog( "error in starting data thread." );
    qApp->quit();
  }
  else {
    if ( AutoSave )
      startSaving( 1 );
    QTimer::singleShot( ProcessInterval, this, SLOT( processData() ) );
    if ( StopRecording ) {
      QDateTime ct = QDateTime::currentDateTime();
      qint64 rectime = ct.msecsTo( StopTime );
      if ( rectime > 0 ) {
	printlog( "Automatically stop recording in " + Str( rectime ) + "ms at "
		  + StopTime.toString().toStdString() );
	QTimer::singleShot( rectime, qApp, SLOT( quit() ) );
      }
    }
  }
}


void FishGridWidget::printlog( const string &message ) const
{
  FileSaver.printlog( message );
}


void FishGridWidget::lockAI( int g )
{
  DataLoop->lockAI( g );
}


void FishGridWidget::unlockAI( int g )
{
  DataLoop->unlockAI( g );
}


void FishGridWidget::processData( void )
{
  // error in acquisition:
  if ( ! DataLoop->running() ) {
    FileSaver.interruptionTimeStamp();
    int r = DataLoop->start();
    if ( r != 0 ) {
      printlog( "error in restarting data thread." );
      qApp->quit();
      return;
    }
  }

  // save and analyze data:   
  else {
    string wts = "FishGrid";
    string fsm = FileSaver.save();
    if ( ! fsm.empty() )
      wts += " @ " + fsm;

    if ( CurrentAnalyzer != 0 ) {

      // clear analysis buffer:
      for ( int g=0; g<MaxGrids; g++ ) {
	if ( Used[g] ) {
	  for ( int r=0; r<Rows[g]; r++ ) {
	    for ( int c=0; c<Columns[g]; c++ ) {
	      Data[g][r][c].resize( 0.0, DataTime, 1.0/SampleRate, 0.0F );
	      Data[g][r][c].clear();
	    }
	  }
	}
      }
      for ( int g=0; g<MaxGrids; g++ ) {
	if ( Used[g] ) {
	  // index of first data element to be analyzed:
	  lockAI( g );
	  long long size = inputBuffer( g ).size();
	  long long mininx = inputBuffer( g ).minIndex();
	  unlockAI( g );
	  long long inx = size - (int)::floor(DataTime*SampleRate)*GridChannels[g];
	  mininx += (int)::floor( GridChannels[g]*SampleRate );  // add 1 second for incoming new data
	  if ( inx < mininx )
	    inx = mininx;
	  inx = (inx/GridChannels[g])*GridChannels[g];
	  // copy data from buffer:
	  while ( inx<size-GridChannels[g] ) {
	    for ( int r=0; r<Rows[g]; r++ ) {
	      for ( int c=0; c<Columns[g]; c++ )
		Data[g][r][c].push( 1000.0*inputBuffer( g )[inx++] );  // convert to Millivolt
	    }
	  }
	}
      }

      // preprocessing:
      for ( deque< PreProcessor* >::iterator pp = PreProcessors.begin();
	    pp != PreProcessors.end();
	    ++pp ) {
	string s = (*pp)->process( Data );
	if ( ! s.empty() )
	  wts += " " + s;
      }

      // analyze and plot:
      AnalyzerWidgets[CurrentAnalyzer]->process( Data );
    }
    setWindowTitle( wts.c_str() );
  }
    
  QTimer::singleShot( ProcessInterval, this, SLOT( processData() ) );
}


void FishGridWidget::finish( void )
{
  if ( FileSaver.saving() ) {
    FileSaver.stop();
    setWindowTitle( "FishGrid" );
  }
  printlog( "quitting FishGrid" );
  CFG.save();
  DataLoop->stop();
}


void FishGridWidget::startSaving( int r )
{
  if ( MetadataDialog != 0 )
    delete MetadataDialog;
  MetadataDialog = 0;

  if ( r != 1 )
    return;

  string path = FileSaver.start();
  if ( ! path.empty() )
    setWindowTitle( string( "FishGrid @ 00:00:00 saving data to " + path ).c_str() );
}


void FishGridWidget::saveTimeStamp( int r )
{
  if ( TimeStampDialog != 0 )
    delete TimeStampDialog;
  TimeStampDialog = 0;

  if ( r != 1 )
    return;

  FileSaver.saveTimeStamp();
}


void FishGridWidget::keyPressEvent( QKeyEvent *event )
{
  BaseWidget::keyPressEvent( event );
  if ( event->isAccepted() )
    return;

  switch ( event->key() ) {

  case Qt::Key_Q :
    DataInterval /= 2.0;
    ProcessInterval /= 2;
    break;

  case Qt::Key_W :
    DataInterval *= 2.0;
    ProcessInterval *= 2;
    break;

  case Qt::Key_Enter :
  case Qt::Key_Return :
    if ( FileSaver.saving() ) {
      if ( event->modifiers() & Qt::AltModifier ) {
	FileSaver.stop();
	setWindowTitle( "FishGrid" );
      }
    }
    else if ( MetadataDialog == 0 ) {
      MetadataDialog = new OptDialog( false );
      MetadataDialog->setCaption( "MetaData" );
      MetadataDialog->addOptions( *this, 4, 8, OptWidget::TabSectionStyle | OptWidget::BoldSectionsStyle ); // select, readonly
      MetadataDialog->setHorizontalSpacing( 4 );
      MetadataDialog->setVerticalSpacing( 4 );
      MetadataDialog->setMargins( 10 );
      MetadataDialog->addButton( "&Ok", OptDialog::Accept, 1 );
      MetadataDialog->addButton( "&Cancel" );
      connect( MetadataDialog, SIGNAL( dialogClosed( int ) ),
	       this, SLOT( startSaving( int ) ) );
      MetadataDialog->exec();
    }
    break;

  case Qt::Key_Backspace :
    if ( TimeStampDialog != 0 )
      return;
    FileSaver.timeStamp();
    TimeStampDialog = new OptDialog( false );
    TimeStampDialog->setCaption( "TimeStamp" );
    TimeStampDialog->addOptions( FileSaver.timeStampOpts(), 1, 2 ); // select, readonly
    TimeStampDialog->setHorizontalSpacing( 4 );
    TimeStampDialog->setVerticalSpacing( 4 );
    TimeStampDialog->setMargins( 10 );
    TimeStampDialog->addButton( "&Ok", OptDialog::Accept, 1 );
    TimeStampDialog->addButton( "&Cancel" );
    connect( TimeStampDialog, SIGNAL( dialogClosed( int ) ),
	     this, SLOT( saveTimeStamp( int ) ) );
    TimeStampDialog->exec();
    break;

  default:
    event->ignore();
    return;
  }

  event->accept();
}


#include "moc_fishgridwidget.cc"

