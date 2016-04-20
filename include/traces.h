/*
  traces.h
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

#ifndef _TRACES_H_
#define _TRACES_H_ 1

#include <relacs/plot.h>
#include <relacs/multiplot.h>
#include "analyzer.h"

using namespace std;
using namespace relacs;

class BaseWidget;


/*! 
\class Traces
\brief Analyzer implementation that plots voltage traces
\author Jan Benda

\section keys Key shortcuts

- \c V, \c Y : decrease voltage range (zoom in)
- \c v, \c y : increase voltage range (zoom out)
- \c +, \c X : decrease time range (zoom in)
- \c -, \c x : increase time range (zoom out)
- \c d : toggle wether to link visible time window to length of Data segement

*/

class Traces : public Analyzer
{
  Q_OBJECT

public:

    /*! Constructs Traces. */
  Traces( BaseWidget *bw, QWidget *parent=0 );
    /*! Destructs Traces. */
  virtual ~Traces( void );

    /*! Initialize the analyzer. */
  virtual void initialize( void );

    /*! Switch between showing all traces and a single trace
        as well as the grid.
        \param[in] mode 0: show single trace, 1: show all traces, 2: show all traces merged into a single one
	\param[in] grid the currently selected grid */
  virtual void display( int mode, int grid );

    /*! Analyze and plot the data.
        \param[in] data the data for each row and column */
  virtual void process( const deque< deque< SampleDataF > > data[] );


protected:

  void zoomTimeIn( void );
  void zoomTimeOut( void );

  void zoomVoltIn( void );
  void zoomVoltOut( void );

  virtual void keyPressEvent( QKeyEvent *event );

  MultiPlot AP;
  Plot SP;

    /*! The currently displayed time window length. */
  double TimeWindow;
    /*! Couple TimeWindow to dataTime() */
  bool LinkTimeWindow;
    /*! The currently displayed voltage range. */
  double VoltRange;

};


#endif /* ! _TRACES_H_ */

