/*
  demean.h
  PreProcessor implementation that removes mean values of all traces.

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

#ifndef _DEMEAN_H_
#define _DEMEAN_H_ 1

#include "preprocessor.h"

using namespace std;
using namespace relacs;

class BaseWidget;


/*! 
\class DeMean
\brief PreProcessor implementation that removes mean values of all traces.
\author Jan Benda

*/

class DeMean : public PreProcessor
{
  Q_OBJECT

public:

    /*! Constructs DeMean. */
  DeMean( int hotkey, BaseWidget *bw, QObject *parent=0 );
    /*! Destructs DeMean. */
  virtual ~DeMean( void );

    /*! Initialize the preprocessor. */
  virtual void initialize( void );

    /*! Pre-process the data.
        \param[in] data the data for each row and column
	\return a string for the window title indicating what the preprocessor is doing */
  virtual string process( deque< deque< SampleDataF > > data[] );

};


#endif /* ! _DEMEAN_H_ */

