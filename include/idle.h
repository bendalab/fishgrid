/*
  idle.h
  Analyzer implementation that does nothing

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

#ifndef _IDLE_H_
#define _IDLE_H_ 1

#include "analyzer.h"

using namespace std;
using namespace relacs;

class BaseWidget;


/*! 
\class Idle
\brief Analyzer implementation that does nothing
\author Jan Benda

*/

class Idle : public Analyzer
{
  Q_OBJECT

public:

    /*! Constructs Idle. */
  Idle( BaseWidget *bw, QWidget *parent=0 );
    /*! Destructs Idle. */
  virtual ~Idle( void );

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

};


#endif /* ! _IDLE_H_ */

