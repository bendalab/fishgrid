/*
  stepper.cc
  Repetitive recordings stepping through predefined fish positions.

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
#include <QApplication>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
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
#include "stepper.h"

using namespace std;


Stepper::Stepper( double samplerate,
		  double maxvolts, double gain,
		  double buffertime,
		  double datatime, double datainterval,
		  bool dialog, bool simulate )
  : QWidget( 0 ),
    ConfigData( "fishgridstepper.cfg" ),
    FileSaver( this ),
    DataLoop( 0 ),
    MetadataDialog( 0 ),
    P( this )
{
  // layout:
  setWindowTitle( "FishGridStepper" );
  resize( 450, 400 );
  QVBoxLayout *vb = new QVBoxLayout;
  setLayout( vb );
  vb->addWidget( &P );

  SL = new QStackedLayout( this );
  vb->addLayout( SL );
  QFont nf( font() );
  nf.setPointSize( 2 * fontInfo().pointSize() );
  nf.setBold( true );

  ActionWidget = new QWidget;
  QVBoxLayout *awb = new QVBoxLayout;
  ActionWidget->setLayout( awb );
  SL->addWidget( ActionWidget );
  CurrentLabel = new QLabel( "Current" );
  CurrentLabel->setFont( nf );
  CurrentLabel->setAlignment( Qt::AlignHCenter );
  CurrentLabel->hide();
  awb->addWidget( CurrentLabel );
  NextLabel = new QLabel( "Next" );
  NextLabel->setFont( nf );
  NextLabel->setAlignment( Qt::AlignHCenter );
  awb->addWidget( NextLabel );
  QHBoxLayout *bb = new QHBoxLayout;
  awb->addLayout( bb );
  AgainButton = new QPushButton( "&Again" );
  AgainButton->setEnabled( false );
  connect( AgainButton, SIGNAL( clicked() ), this, SLOT( startAgain() ) );
  bb->addWidget( AgainButton );
  NextButton = new QPushButton( "&Next" );
  connect( NextButton, SIGNAL( clicked() ), this, SLOT( start() ) );
  bb->addWidget( NextButton );
  QuitButton = new QPushButton( "&Quit" );
  connect( QuitButton, SIGNAL( clicked() ), qApp, SLOT( quit() ) );
  bb->addWidget( QuitButton );

  RecordingLabel = new QLabel( "Recording..." );
  RecordingLabel->setFont( nf );
  RecordingLabel->setAlignment( Qt::AlignHCenter );
  awb->addWidget( RecordingLabel );
  SL->addWidget( RecordingLabel );

  //  connect( qApp, SIGNAL( lastWindowClosed() ), qApp, SLOT( quit() ) );

  MaxVolts = maxvolts;
  SampleRate = samplerate;
  Gain = gain;
  Unit = "mV";
  DataTime = datatime;
  DataInterval = datainterval;
  BufferTime = buffertime;

  erase( "Used1" );
  Used[0] = true;
  for ( int g=1; g<MaxGrids; g++ ) {
    string ns = Str( g+1 );
    erase( "Grid &"+ns );
    erase( "Name"+ns );
    erase( "Configuration"+ns );
    erase( "Used"+ns );
    erase( "Columns"+ns );
    erase( "Rows"+ns );
    erase( "ColumnDistance"+ns );
    erase( "RowDistance"+ns );
    erase( "ElectrodeType"+ns );
    erase( "GridPosX"+ns );
    erase( "GridPosY"+ns );
    erase( "GridOrientation"+ns );
    erase( "RefElectrodeType"+ns );
    erase( "RefElectrodePosX"+ns );
    erase( "RefElectrodePosY"+ns );
    erase( "GroundElectrodeType"+ns );
    erase( "GroundElectrodePosX"+ns );
    erase( "GroundElectrodePosY"+ns );
    erase( "WaterDepth"+ns );
  }
  delFlags( "DataTime", 1+4 );
  delFlags( "DataInterval", 1+4 );
  setRequest( "BufferTime", "Length of each recording" );
  // from OptWidget:
  Options &opt = insertSection( "R&aster", "", 1+4+16 );
  opt.addNumber( "RasterStepX", "X increments",  5.0, 0.0, 100000.0, 5.0, "cm", "cm", "%.1f" ).setFlags( 1+4+16+32 );
  opt.addNumber( "RasterStepY", "Y increments",  5.0, 0.0, 100000.0, 5.0, "cm", "cm", "%.1f" ).setFlags( 1+4+16+32 );
  opt.addNumber( "RasterWidthX", "X-width of raster",  50.0, 0.0, 100000.0, 5.0, "cm", "cm", "%.1f" ).setFlags( 1+4+16+32 );
  opt.addNumber( "RasterWidthY", "Y-width of raster",  50.0, 0.0, 100000.0, 5.0, "cm", "cm", "%.1f" ).setFlags( 1+4+16+32 );
  opt.addNumber( "RasterOriginX", "X-coordinate of raster origin relative to grid",  50.0, 0.0, 100000.0, 5.0, "cm", "cm", "%.1f" ).setFlags( 1+4+16+32 );
  opt.addNumber( "RasterOriginY", "X-coordinate of raster origin relative to grid",  50.0, 0.0, 100000.0, 5.0, "cm", "cm", "%.1f" ).setFlags( 1+4+16+32 );
  opt.addInteger( "RasterStartX", "Start at X index", 0, 0, 10000, 1 ).setFlags( 1+4+8+16+32 );
  opt.addInteger( "RasterStartY", "Start at Y index", 0, 0, 10000, 1 ).setFlags( 1+4+8+16+32 );
  opt.addNumber( "FishYaw", "Horizontal orientation of the fish (yaw)",  0.0, -360.0, 360.0, 5.0, "Degree", "Degree", "%.0f" ).setFlags( 1+4+16+32 );
  opt.addNumber( "FishPitch", "Vertical orientation of the fish (pitch)",  0.0, -360.0, 360.0, 5.0, "Degree", "Degree", "%.0f" ).setFlags( 1+4+16+32 );

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

  Rows[0] = integer( "Rows1", 0, 4 );
  Columns[0] = integer( "Columns1", 0, 4 );

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

    Rows[0] = integer( "Rows1", 0, 4 );
    Columns[0] = integer( "Columns1", 0, 4 );
    SampleRate = number( "AISampleRate", SampleRate );
    MaxVolts = number( "AIMaxVolt", MaxVolts );
    Gain = number( "Gain", Gain );
    BufferTime = number( "BufferTime", BufferTime );
  }

  RasterStepX = number( "RasterStepX" );
  RasterStepY = number( "RasterStepY" );
  double rasteroriginx = number( "RasterOriginX" );
  double rasteroriginy = number( "RasterOriginY" );
  double columndist = number( "ColumnDistance1" );
  double rowdist = number( "RowDistance1" );
  RasterOffsX = rasteroriginx - floor( rasteroriginx/columndist )*columndist;
  RasterOffsY = rasteroriginy - floor( rasteroriginy/rowdist )*rowdist;
  RasterWidthX = number( "RasterWidthX" );
  RasterWidthY = number( "RasterWidthY" );
  RasterInxX = integer( "RasterStartX" );
  RasterInxY = integer( "RasterStartY" );
  RasterMaxX = (int)round( RasterWidthX / RasterStepX );
  RasterMaxY = (int)round( RasterWidthY / RasterStepY );
  RasterPrevX = -1;
  RasterPrevY = -1;
  NextLabel->setText( string( "Next: " + Str( RasterInxX ) + "|"  + Str( RasterInxY ) ) .c_str() );
  setNumber( "DataTime", 0.1 );
  setNumber( "DataInterval", 1.0 );

  // write configuration to console:
  Channels = 0;
  Grid = 0;
  printlog( "Grid" );
  printlog( "  Rows=" + Str( Rows[0] ) );
  printlog( "  Columns=" + Str( Columns[0] ) );
  GridChannels[0] = Rows[0]*Columns[0];
  printlog( "  Channels=" + Str( GridChannels[0] ) );
  Channels += GridChannels[0];
  setInteger( "AIUsedChannelCount", Channels );
  printlog( "Channels=" + Str( Channels ) );
  printlog( "SampleRate=" + Str( SampleRate ) + "Hz" );
  printlog( "MaxVolts=" + Str( MaxVolts, "%.3f" ) + "V" );
  printlog( "Gain=" + Str( Gain ) );
  printlog( "BufferTime=" + Str( BufferTime ) + "s" );
  printlog( "Raster" );
  printlog( "  NX=" + Str( RasterMaxX ) );
  printlog( "  NY=" + Str( RasterMaxY ) );
  printlog( "  StepX=" + Str( RasterStepX ) + "cm" );
  printlog( "  StepY=" + Str( RasterStepY ) + "cm" );
  printlog( "  XIndex=" + Str( RasterInxX ) );
  printlog( "  YIndex=" + Str( RasterInxY ) );

  plot();
  setup();

  string path = FileSaver.start( false, false );
  if ( ! path.empty() )
    setWindowTitle( string( "FishGridStepper: saving data to " + path ).c_str() );
}


Stepper::~Stepper( void )
{
}


void Stepper::start( void )
{
  RasterPrevX = -1;
  RasterPrevY = -1;
  plot();
  SL->setCurrentIndex( 1 );
  FileSaver.openTraceFiles( "-raster" + Str( RasterInxX, 2, '0' ) + "-" + Str( RasterInxY, 2, '0' ) );
  printlog( "start acquisition for column " + Str( RasterInxX ) + ", row " + Str( RasterInxY ) );
  int r = DataLoop->start( BufferTime );
  if ( r != 0 ) {
    printlog( "error in starting data thread." );
    qApp->quit();
  }
  else {
    QTimer::singleShot( (int)ceil( BufferTime*1000.0 ) + 100,
			this, SLOT( recordData() ) );
  }
}


void Stepper::startAgain( void )
{
  RasterInxX = RasterPrevX;
  RasterInxY = RasterPrevY;
  start();
}


void Stepper::recordData( void )
{
  // save data:   
  FileSaver.save();
  FileSaver.closeTraceFiles();

  // store position:
  RasterPrevX = RasterInxX;
  RasterPrevY = RasterInxY;
  CurrentLabel->setText( string( "Current: " + Str( RasterPrevX ) + "|"  + Str( RasterPrevY ) ) .c_str() );
  CurrentLabel->show();
  AgainButton->setEnabled( true );
  // next position:
  RasterInxX++;
  if ( RasterInxX > RasterMaxX ) {
    RasterInxX = 0;
    RasterInxY++;
    if ( RasterInxY > RasterMaxY ) {
      qApp->quit();
    }
  }
  NextLabel->setText( string( "Next: " + Str( RasterInxX ) + "|"  + Str( RasterInxY ) ) .c_str() );
  SL->setCurrentIndex( 0 );
  plot();
}


void Stepper::finish( void )
{
  FileSaver.stop();
  printlog( "quitting FishGridStepper" );
  CFG.save();
}


void Stepper::plot( void )
{
  P.lock();
  P.clear();
  P.setLMarg( 7 );
  P.setRMarg( 1 );
  P.setTMarg( 1 );
  P.setBMarg( 4 );
  P.noGrid();
  P.setXRange( -2.0*RasterStepX, (RasterMaxX+2)*RasterStepX );
  P.setYRange( -2.0*RasterStepY, (RasterMaxY+2)*RasterStepY );
  P.setXLabel( "X [cm]" );
  P.setYLabel( "Y [cm]" );
  P.setYLabelPos( 2.0, Plot::FirstMargin, 0.5, Plot::Graph,
		  Plot::Center, -90.0 );
  // Electrode grid:
  double cd = number( "ColumnDistance1" );
  double rd = number( "RowDistance1" );
  P.plotVLine( 0.0, Plot::White, 1, Plot::LongDash );
  P.plotVLine( cd, Plot::White, 1, Plot::LongDash );
  P.plotHLine( 0.0, Plot::White, 1, Plot::LongDash );
  P.plotHLine( rd, Plot::White, 1, Plot::LongDash );
  // Electrode positions:
  P.plotPoint( 0.0, Plot::First, 0.0, Plot::First, 0, Plot::Circle,
	       0.6*RasterStepX, Plot::First, Plot::White, Plot::White );
  P.plotPoint( cd, Plot::First, 0.0, Plot::First, 0, Plot::Circle,
	       0.6*RasterStepX, Plot::First, Plot::White, Plot::White );
  P.plotPoint( 0.0, Plot::First, rd, Plot::First, 0, Plot::Circle,
	       0.6*RasterStepX, Plot::First, Plot::White, Plot::White );
  P.plotPoint( cd, Plot::First, rd, Plot::First, 0, Plot::Circle,
	       0.6*RasterStepX, Plot::First, Plot::White, Plot::White );
  // Rastergrid:
  for ( int k=0; k<=RasterMaxX; k++ )
    P.plotLine( k*RasterStepX+RasterOffsX, RasterOffsY,
		k*RasterStepX+RasterOffsX, RasterMaxY*RasterStepY+RasterOffsY,
		Plot::White, 2, Plot::Solid );
  for ( int k=0; k<=RasterMaxY; k++ )
    P.plotLine( RasterOffsX, k*RasterStepY+RasterOffsY,
		RasterMaxX*RasterStepX+RasterOffsX, k*RasterStepY+RasterOffsY,
		Plot::White, 2, Plot::Solid );
  // Rasternodes:
  for ( int k=0; k<=RasterMaxX; k++ ) {
    for ( int j=0; j<=RasterMaxY; j++ )
      P.plotPoint( k*RasterStepX+RasterOffsX, Plot::First,
		   j*RasterStepY+RasterOffsY, Plot::First,
		   0, Plot::Circle, 0.3*RasterStepX, Plot::First, Plot::White, Plot::White );
  }
  // current node:
  if ( RasterPrevX >= 0 )
    P.plotPoint( RasterPrevX*RasterStepX+RasterOffsX, Plot::First,
		 RasterPrevY*RasterStepY+RasterOffsY, Plot::First,
		 0, Plot::Circle, 0.6*RasterStepX, Plot::First, Plot::Yellow, Plot::Yellow );
  // next node:
  P.plotPoint( RasterInxX*RasterStepX+RasterOffsX, Plot::First,
	       RasterInxY*RasterStepY+RasterOffsY, Plot::First,
	       0, Plot::Circle, 0.6*RasterStepX, Plot::First, Plot::Red, Plot::Red );
  P.draw();
  P.unlock();
}


void Stepper::keyPressEvent( QKeyEvent *event )
{
  switch ( event->key() ) {

  case Qt::Key_Enter :
  case Qt::Key_Return :
    start();
    break;

  case Qt::Key_Backspace :
    startAgain();
    break;

  case Qt::Key_M:
    if ( isMaximized() )
      showNormal();
    else
      showMaximized();
    break;
    
  default:
    event->ignore();
    return;
  }

  event->accept();
}


void Stepper::printlog( const string &message ) const
{
  FileSaver.printlog( message );
}


void Stepper::lockAI( int g )
{
  DataLoop->lockAI( g );
}


void Stepper::unlockAI( int g )
{
  DataLoop->unlockAI( g );
}


#include "moc_stepper.cc"

