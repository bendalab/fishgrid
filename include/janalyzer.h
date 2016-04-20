/*
  janalyzer.h

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

#ifndef _JANALYZER_H_
#define _JANALYZER_H_ 1

#include <relacs/plot.h>
#include <relacs/multiplot.h>
#include "analyzer.h"

using namespace std;
using namespace relacs;

class BaseWidget;


/*! 
\class JAnalyzer
\brief Some analyzer implementation
\author Jan Benda
*/

class JAnalyzer : public Analyzer
{
  Q_OBJECT

public:

  JAnalyzer( BaseWidget *bw, QWidget *parent=0 );
  virtual ~JAnalyzer( void );

    /*! Initialize the analyzer. */
  virtual void initialize( void );

    /*! Switch between showing all traces and a single trace
        as well as the grid.
        \param[in] alltraces if \c true, show all traces
	\param[in] grid the currently selected grid */
  virtual void display( bool alltraces, int grid );

    /*! Analyze and plot the data.
        \param[in] data the data for each row and column */
  virtual void process( const deque< deque< SampleDataF > > data[] );


protected:

  virtual void keyPressEvent( QKeyEvent *event );


};


#endif /* ! _JANALYZER_H_ */

