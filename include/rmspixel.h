/*
  rmspixel.h
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

#ifndef _RMSPIXEL_H_
#define _RMSPIXEL_H_ 1

#include <QPixmap>
#include <QColor>
#include <relacs/cyclicarray.h>
#include "analyzer.h"

using namespace std;
using namespace relacs;

class BaseWidget;


/*! 
\class RMSPixel
\brief Analyzer implementation that plots for each electrode a box that is colored according to the rms signal strength
\author Jan Benda

The rms is the standard deviation of the voltage traces over the whole data section.

The color code goes from blue (rms=zero) over magenta, red, orange to yellow (maximum possible rms).

\section keys Key shortcuts

- \c F : toggle fish trace
*/

class RMSPixel : public Analyzer
{
  Q_OBJECT

public:

    /*! Constructs an RMSPixel. */
  RMSPixel( BaseWidget *bw, QWidget *parent=0 );
    /*! Destructs an RMSPixel. */
  virtual ~RMSPixel( void );

    /*! Read out parameter. */
  virtual void notify( void );

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

    /*! Maps rms signla strength to a color.
        \param[in] rms the rms signal strength 
        \return the color to be used for this value */
  QColor map( double rms );


protected:

    /*! Handles the resize event. */
  virtual void resizeEvent( QResizeEvent *qre );
    /*! Paints the entire plot. */
  virtual void paintEvent( QPaintEvent *qpe );

  virtual void keyPressEvent( QKeyEvent *event );

  QPixmap *PixMap;

  CyclicArray< double > XPosition;
  CyclicArray< double > YPosition;
  bool PlotTrace;
  int TraceSize;
  int CurrentTraceSize;

};


#endif /* ! _RMSPIXEL_H_ */

