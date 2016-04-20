/*
  nidaqmxthread.h
  DataThread implementation for acquisition of data using NIDAQmxBase

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

#ifndef _NIDAQMXTHREAD_H_
#define _NIDAQMXTHREAD_H_ 1

#include <NIDAQmxBase.h>
#include "datathread.h"

using namespace std;


/*! 
\class NIDAQmxThread
\brief DataThread implementation for acquisition of data using NIDAQmxBase
\author Jan Benda
*/

class NIDAQmxThread : public DataThread
{

public:

  NIDAQmxThread( ConfigData *cd );


protected:

  virtual int initialize( double duration=0.0 );
  virtual void finish( void );
  virtual int read( void );


private:

    TaskHandle Handle;


};


#endif /* ! _NIDAQMXTHREAD_H_ */

