/*
  janalyzer.cc

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
#include "janalyzer.h"


JAnalyzer::JAnalyzer( BaseWidget *bw, QWidget *parent )
  : Analyzer( "JAnalyzer", bw, parent )
{
  // set up your widget.
  // JAnalyzer is a QWidget.
}


JAnalyzer::~JAnalyzer( void )
{
}


void JAnalyzer::initialize( void )
{
  // this function is called once, after all the configuration data are read in.
  // in particular, columns(), rows(), gridChannels() can now be used for some initialization.
}


void JAnalyzer::display( bool alltraces, int grid )
{
  // this function is called whenever the uses from showing all traces (alltraces == true)
  // or a single trace (alltraces == false), or when the grid changes (grid is the index of the grid).
}


void JAnalyzer::process( const deque< deque< SampleDataF > > data[] )
{
  // This is the main function that does the analyzis an plotting.
  // data are short stretches of data for each electrode
  // organized in rows and columns.
}



void JAnalyzer::keyPressEvent( QKeyEvent *event )
{
  switch ( event->key() ) {

  case Qt::Key_X :
    // Do something when the user presses "X".
    break;

  case Qt::Key_Z :
    // Do something when the user presses "X".
    break;

  default:
    event->ignore();
    return;
  }

  event->accept();
}


#include "moc_janalyzer.cc"
