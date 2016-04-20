/*
  simulationthread.h
  DataThread implementation for simulating data of a moving fish

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

#ifndef _SIMULATIONTHREAD_H_
#define _SIMULATIONTHREAD_H_ 1

#include <vector>
#include "datathread.h"

using namespace std;


/*! 
\class SimulationThread
\brief DataThread implementation for simulating data of a moving fish
\author Jan Benda
*/

class SimulationThread : public DataThread
{

public:

  SimulationThread( ConfigData *cd );


protected:

  virtual int initialize( double duration=0.0 );
  virtual void finish( void );
  virtual int read( void );


private:

  bool FixedPos;
  vector<float> Sine[ConfigData::MaxGrids];
  double Amplitude;
  double X[ConfigData::MaxGrids];
  double Y[ConfigData::MaxGrids];

  static const double D = 20.0;
  static const double Tau = 6000.0;

  int Samples;
  int MaxSamples;

};


#endif /* ! _SIMULATIONTHREAD_H_ */

