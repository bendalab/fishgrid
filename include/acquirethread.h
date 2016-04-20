/*
  acquirethread.h
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

#ifndef _ACQUIRETHREAD_H_
#define _ACQUIRETHREAD_H_ 1

#include <NIDAQmxBase.h>
#include "datathread.h"

using namespace std;


/*! 
\class AcquireThread
\brief DataThread implementation for acquisition of data using NIDAQmxBase
\author Jan Benda
*/

class AcquireThread : public DataThread
{

public:

  AcquireThread( FishGridWidget *fgw );


protected:

  virtual int initialize( void );
  virtual void finish( void );
  virtual int read( void );


private:

    TaskHandle Handle;


};


#endif /* ! _ACQUIRETHREAD_H_ */

