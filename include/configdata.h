/*
  configdata.h
  All the configuration data for the fishgrid recordings.

  FishGrid
  Copyright (C) 2009-2011 Jan Benda <benda@bio.lmu.de> & Joerg Henninger <henninger@bio.lmu.de>

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

#ifndef _CONFIGDATA_H_
#define _CONFIGDATA_H_ 1

#include <deque>
#include <relacs/sampledata.h>
#include <relacs/configclass.h>
#include <relacs/configureclasses.h>

using namespace std;
using namespace relacs;


/*! 
\class ConfigData
\brief All the configuration data for the fishgrid recordings.
\author Jan Benda

%ConfigData defines configurable parameter that define the grids,
set hardware parameter, and specify some meta data.
The syntax of the add* functions is as follows:
\code
addNumber( identifier, request, default, min, max, step, unit, outunit, format );
addInteger( identifier, request, default );
addBoolean( identifier, reqquest, default );
addText( identifier, reqquest, default );
\endcode
\c identifier is a string that uniquely identifies the option.  This
string is saved in the configuration file and is used to retrieve the
value of an option.
\c request is the text that is shown in a dialog.
\c default is the default value of the option (a floating point
number, an integer, a bool \c true or \c false, or a string,
respectively) that the option has before loading the configuration file.
\c min and \c max is the minimum and maximum value a number or integer option can assume.
\c step is the increment used by the spin-box of a dialog, when pressing the up/down arrows.
\c unit is the unit for \c default, \c min, \c max, \c step.
\c outunit is the unit used for displaying the value in a dialog.
\c format is a C-style format string that specifies how to display the number in a dialog.
The \c format has the following syntax: \c "%w.pf".
\c w is the total width (number of characters) of the number.
If not specified, the number can have arbitrary width.
\c p is the precision, and \c f is the format.
\c f can be either g, e, or f.
See \c man \c 3 \c printf for details on the format string.
*/


class ConfigData : public ConfigClass
{

  friend class DataThread;
  friend class Recording;


public:

  /*! Constructs a ConfigData for using the configuration file \a configfile. */
  ConfigData( const string &configfile="fishgrid.cfg" );
  ~ConfigData( void );

    /*! Maximum number of electrode grids supported. */
  static const int MaxGrids = 4;
  

protected:

    /*! Sets up the data buffer. */
  virtual void setup( void );

    /*! Write current time and \a message to stderr. */
  virtual void printlog( const string &message ) const;

    /*! Indicates whether a grid is used. */
  bool Used[MaxGrids];
    /*! For each grid the number of electrodes in a row. */
  int Rows[MaxGrids];
    /*! For each grid the number of electrodes in a column. */
  int Columns[MaxGrids];
    /*! For each grid the total number of electrodes. */
  int GridChannels[MaxGrids];
    /*! For each grid the currently selected row of electrodes. */
  int Row[MaxGrids];
    /*! For each grid the currently selected column of electrodes. */
  int Column[MaxGrids];
    /*! For each grid the data for analysis. */
  deque< deque< relacs::SampleDataF > > Data[MaxGrids];

    /*! Currently active grid. */
  int Grid;
    /*! The total number of used electrodes. */
  int Channels;
    /*! The maximum voltage to be expected to measure. */
  double MaxVolts;
    /*! The sampling rate per channel to be used in hertz. */
  double SampleRate;
    /*! The gain factor of the amplifiers. */
  double Gain;
    /*! The unit of the recorded voltage after multiplication with \a Gain. */
  string Unit;
    /*! The length of the time segments held in Data. */
  double DataTime;
    /*! The time interval in seconds between data processing */
  double DataInterval;
    /*! The time interval in milliseconds between data processing */
  int ProcessInterval;

    /*! The lenght of the input buffer in seconds. */
  double BufferTime;

  ConfigureClasses CFG;

};


#endif /* ! _CONFIGDATA_H_ */

