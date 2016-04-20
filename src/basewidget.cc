/*
  basewidget.cc
  Base class for coordination of data and analyzers

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
#include <QDateTime>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QShortcut>
#include <relacs/optwidget.h>
#include <relacs/optdialog.h>
#include "preprocessor.h"
#include "demean.h"
#include "commonnoiseremoval.h"
#include "analyzer.h"
#include "idle.h"
#include "traces.h"
#include "spectra.h"
#include "rmsplot.h"
#include "rmspixel.h"
#include "janalyzer.h"
#include "basewidget.h"

using namespace std;


BaseWidget::BaseWidget( const string &configfile )
  : QMainWindow( 0 ),
    ConfigData( configfile ),
    DisplayMode( 1 ),
    PreProcessorDialog( 0 ),
    MainWidget( 0 )
{
  // layout:
  setWindowTitle( "FishGrid" );
  resize( 450, 400 );
  MainWidget = new QStackedWidget( this );
  setCentralWidget( MainWidget );
  new QShortcut( QKeySequence( Qt::ALT + Qt::Key_Q ),
		 this, SLOT( close() ) );
  connect( qApp, SIGNAL( lastWindowClosed() ), qApp, SLOT( quit() ) );
  //  connect( qApp, SIGNAL( aboutToQuit() ), this, SLOT( finish() ) );

  // preprocessors:
  AvailablePreProcessors.clear();
  addPreProcessor( new DeMean( Qt::Key_M, this, this ) );
  addPreProcessor( new CommonNoiseRemoval( Qt::Key_C, this, this ) );

  // preprocessor names:
  string prenames = "none";
  for ( map< string, PreProcessor* >::iterator pp = AvailablePreProcessors.begin();
	pp != AvailablePreProcessors.end();
	++pp ) {
    prenames += '|' + (*pp).first;
  }

  // preprocessor options:
  // from OptWidget:
  newSection( "Pre-processing" ).setFlag( 1+16 );
  addSelection( "PreProcessor1", "Name of pre-processing module no. 1", prenames ).setFlags( 1+16+64+8192 );
  addSelection( "PreProcessor2", "Name of pre-processing module no. 2", prenames ).setFlags( 1+16+64+8192 );
  addSelection( "PreProcessor3", "Name of pre-processing module no. 3", prenames ).setFlags( 1+16+64+8192 );
  addSelection( "PreProcessor4", "Name of pre-processing module no. 4", prenames ).setFlags( 1+16+64+8192 );
  addSelection( "PreProcessor5", "Name of pre-processing module no. 5", prenames ).setFlags( 1+16+64+8192 );
  addSelection( "PreProcessor6", "Name of pre-processing module no. 6", prenames ).setFlags( 1+16+64+8192 );
  addSelection( "PreProcessor7", "Name of pre-processing module no. 7", prenames ).setFlags( 1+16+64+8192 );
  addSelection( "PreProcessor8", "Name of pre-processing module no. 8", prenames ).setFlags( 1+16+64+8192 );
  PreProcessors.clear();

  // set up the analyzer widgets:
  AnalyzerWidgets.clear();
  AnalyzerHotkeys.clear();
  addAnalyzer( new Idle( this ), Qt::Key_I );
  addAnalyzer( new Traces( this ), Qt::Key_T );
  addAnalyzer( new Spectra( this ), Qt::Key_P );
  //  addAnalyzer( new RMSPlot( this ), Qt::Key_E );
  //  addAnalyzer( new RMSPixel( this ), Qt::Key_R );
  //  addAnalyzer( new JAnalyzer( this ), Qt::Key_J );
  CurrentAnalyzer = 1;
  MainWidget->setCurrentIndex( CurrentAnalyzer );
}


BaseWidget::~BaseWidget( void )
{
}


void BaseWidget::setup( void )
{
  ConfigData::setup();
  // initialize preprocessors:
  PreProcessors.clear();
  for ( int k=1; k<=8; k++ ) {
    Str ns( k );
    string ps = text( "PreProcessor" + ns );
    if ( ps != "none" && ! ps. empty() ) {
      PreProcessors.push_back( AvailablePreProcessors[ ps ] );
      PreProcessors.back()->initialize();
    }
  }
  // initialize analyzers:
  for ( unsigned int k=0; k<AnalyzerWidgets.size(); k++ ) {
    AnalyzerWidgets[k]->notify();
    AnalyzerWidgets[k]->initialize();
    AnalyzerWidgets[k]->display( DisplayMode, Grid );
  }
}


void BaseWidget::electrodeLeft( void )
{
  Column[Grid]--;
  if ( Column[Grid] < 0 )
    Column[Grid] = 0;
}


void BaseWidget::electrodeRight( void )
{
  Column[Grid]++;
  if ( Column[Grid] >= Columns[Grid] )
    Column[Grid] = Columns[Grid]-1;
}


void BaseWidget::electrodeUp( void )
{
  Row[Grid]--;
  if ( Row[Grid] < 0 )
    Row[Grid] = 0;
}


void BaseWidget::electrodeDown( void )
{
  Row[Grid]++;
  if ( Row[Grid] >= Rows[Grid] )
    Row[Grid] = Rows[Grid]-1;
}


void BaseWidget::nextGrid( void )
{
  do {
    Grid++;
  } while ( Grid < MaxGrids && ! Used[ Grid ] );
  if ( Grid >= MaxGrids ) {
    Grid = -1;
    do {
      Grid++;
    } while ( Grid < MaxGrids && ! Used[ Grid ] );
  }
}


void BaseWidget::prevGrid( void )
{
  do {
    Grid--;
  } while ( Grid >= 0 && ! Used[ Grid ] );
  if ( Grid < 0 ) {
    Grid = MaxGrids;
    do {
      Grid--;
    } while ( Grid >= 0 && ! Used[ Grid ] );
  }
}


void BaseWidget::selectGrid( int g )
{
  if ( g >= 0 && g < MaxGrids && Used[g] )
    Grid = g;
}


void BaseWidget::keyPressEvent( QKeyEvent *event )
{
  // preprocessor dialogs:
  if ( event->modifiers() & Qt::AltModifier ) {
    if ( event->key() == Qt::Key_P ) {
      if ( PreProcessorDialog == 0 ) {
	PreProcessorDialog = new OptDialog( false );
	PreProcessorDialog->setCaption( "PreProcessor" );
	PreProcessorDialog->addOptions( *this, 8192, 0, OptWidget::TabSectionStyle | OptWidget::BoldSectionsStyle ); // select, readonly
	PreProcessorDialog->setHorizontalSpacing( 4 );
	PreProcessorDialog->setVerticalSpacing( 4 );
	PreProcessorDialog->setMargins( 10 );
	PreProcessorDialog->addButton( "&Ok", OptDialog::Accept, 1 );
	PreProcessorDialog->addButton( "&Cancel" );
	connect( PreProcessorDialog, SIGNAL( dialogClosed( int ) ),
		 this, SLOT( closePreProcessorDialog( int ) ) );
	PreProcessorDialog->exec();
      }
      event->accept();
      return;
    }
    for ( map< string, PreProcessor* >::iterator pp = AvailablePreProcessors.begin();
	  pp != AvailablePreProcessors.end();
	  ++pp ) {
      if ( (*pp).second->hotkey() == event->key() ) {
	(*pp).second->dialog();
	event->accept();
	return;
      }
    }
  }

  // activate analyzer:
  for ( unsigned int k=0; k<AnalyzerHotkeys.size(); k++ ) {
    if ( AnalyzerHotkeys[k] == event->key() ) {
      if ( CurrentAnalyzer != (int)k ) {
	CurrentAnalyzer = k;
	AnalyzerWidgets[CurrentAnalyzer]->display( DisplayMode, Grid );
	MainWidget->setCurrentIndex( CurrentAnalyzer );
      }
      event->accept();
      return;
    }
  }

  if ( event->key() >= Qt::Key_1 && event->key() <= Qt::Key_9 ) {
    if ( event->modifiers() & Qt::AltModifier ) {
      int pg = Grid;
      selectGrid( event->key() - Qt::Key_1 );
      if ( pg != Grid )
	AnalyzerWidgets[CurrentAnalyzer]->display( DisplayMode, Grid );
    }
    else {
      event->ignore();
      return;
    }
  }
  else {
    switch ( event->key() ) {

    case Qt::Key_Left :
      if ( event->modifiers() & Qt::ControlModifier ) {
	CurrentAnalyzer--;
	if ( CurrentAnalyzer < 0 )
	  CurrentAnalyzer = AnalyzerWidgets.size()-1;
	AnalyzerWidgets[CurrentAnalyzer]->display( DisplayMode, Grid );
	MainWidget->setCurrentIndex( CurrentAnalyzer );
      }
      else if ( event->modifiers() & Qt::AltModifier ) {
	int pg = Grid;
	prevGrid();
	if ( pg != Grid )
	  AnalyzerWidgets[CurrentAnalyzer]->display( DisplayMode, Grid );
      }
      else
	electrodeLeft();
      break;
    case Qt::Key_Right :
      if ( event->modifiers() & Qt::ControlModifier ) {
	CurrentAnalyzer++;
	if ( CurrentAnalyzer >= (int)AnalyzerWidgets.size() )
	  CurrentAnalyzer = 0;
	AnalyzerWidgets[CurrentAnalyzer]->display( DisplayMode, Grid );
	MainWidget->setCurrentIndex( CurrentAnalyzer );
      }
      else if ( event->modifiers() & Qt::AltModifier ) {
	int pg = Grid;
	nextGrid();
	if ( pg != Grid )
	  AnalyzerWidgets[CurrentAnalyzer]->display( DisplayMode, Grid );
      }
      else
	electrodeRight();
      break;
    case Qt::Key_Up :
      electrodeUp();
      break;
    case Qt::Key_Down :
      electrodeDown();
      break;

    case Qt::Key_A :
      if ( DisplayMode != 1 ) {
	DisplayMode = 1;
	for ( unsigned int k=0; k<AnalyzerWidgets.size(); k++ )
	  AnalyzerWidgets[k]->display( DisplayMode, Grid );
      }
      break;
    case Qt::Key_S :
      if ( ( event->modifiers() & Qt::ControlModifier ) ||
	   ( event->modifiers() & Qt::AltModifier ) ) {
	event->ignore();
	return;
      }
      else {
	if ( DisplayMode != 0 ) {
	  DisplayMode = 0;
	for ( unsigned int k=0; k<AnalyzerWidgets.size(); k++ )
	  AnalyzerWidgets[k]->display( DisplayMode, Grid );
	}
      }
      break;
    case Qt::Key_M :
      if ( ( event->modifiers() & Qt::ControlModifier ) ||
	   ( event->modifiers() & Qt::AltModifier ) ) {
	event->ignore();
	return;
      }
      else if ( event->modifiers() & Qt::ShiftModifier ) {
	if ( isMaximized() )
	  showNormal();
	else
	  showMaximized();
      }
      else {
	if ( DisplayMode != 2 ) {
	  DisplayMode = 2;
	for ( unsigned int k=0; k<AnalyzerWidgets.size(); k++ )
	  AnalyzerWidgets[k]->display( DisplayMode, Grid );
	}
      }
      break;

    case Qt::Key_L :
      if ( event->modifiers() & Qt::ShiftModifier )
	DataTime *= 2.0;
      else
	DataTime *= 0.5;
      break;
    case Qt::Key_Plus :
    case Qt::Key_Equal :
      DataTime *= 0.5;
      break;
    case Qt::Key_Minus :
      DataTime *= 2.0;
      break;

    case Qt::Key_O :
      AnalyzerWidgets[CurrentAnalyzer]->dialog();
      break;

    default:
      event->ignore();
      return;
    }
  }

  event->accept();
}


void BaseWidget::addAnalyzer( Analyzer *a, int hotkey )
{
  MainWidget->addWidget( a );
  AnalyzerWidgets.push_back( a );
  AnalyzerHotkeys.push_back( hotkey );
}


void BaseWidget::addPreProcessor( PreProcessor *p )
{
  AvailablePreProcessors[ p->name() ] = p;
}


void BaseWidget::closePreProcessorDialog( int r )
{
  if ( PreProcessorDialog != 0 )
    delete PreProcessorDialog;
  PreProcessorDialog = 0;

  if ( r != 1 )
    return;

  // initialize preprocessors:
  PreProcessors.clear();
  for ( int k=1; k<=8; k++ ) {
    Str ns( k );
    string ps = text( "PreProcessor" + ns );
    if ( ps != "none" && ! ps. empty() ) {
      PreProcessors.push_back( AvailablePreProcessors[ ps ] );
      PreProcessors.back()->initialize();
    }
  }
}


#include "moc_basewidget.cc"

