/*
  analyzer.h
  Base class for different analyzer and plot methods

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

#ifndef _ANALYZER_H_
#define _ANALYZER_H_ 1

#include <deque>
#include <string>
#include <relacs/sampledata.h>
#include <relacs/configdialog.h>

using namespace std;
using namespace relacs;

class BaseWidget;


/*! 
\class Analyzer
\brief Base class for different analyzer and plot methods
\author Jan Benda
*/

class Analyzer : public QWidget
{
  Q_OBJECT

public:

    /*! Constructs an Analyzer. */
  Analyzer( const string &name, BaseWidget *bw, QWidget *parent=0 );
    /*! Destructs an Analyzer. */
  virtual ~Analyzer( void );

    /*! Initialize the analyzer. */
  virtual void initialize( void ) = 0;

    /*! Switch between showing all traces and a single trace
        as well as the grid.
        \param[in] mode 0: show single trace, 1: show all traces, 2: show all traces merged into a single one
	\param[in] grid the currently selected grid */
  virtual void display( int mode, int grid ) = 0;

    /*! Analyze and plot the data.
        \param[in] data the data for each row and column */
  virtual void process( const deque< deque< SampleDataF > > data[] ) = 0;

    /*! Returns the Analyzer's options. */
  Options &opts( void );
    /*! Returns the Analyzer's options. */
  const Options &opts( void ) const;
    /*! Read out the options. */
  virtual void notify( void );

    /*! Launches the Analyzer's dialog for setting the options. */
  void dialog( void );

    /*! \return the currently active grid. */
  int grid( void ) const;
    /*! \return \c true if grid \a g is used. */
  bool used( int g ) const;
    /*! \return the number of electrodes in a row for grid \a g. */
  int rows( int g ) const;
    /*! \return the number of electrodes in a column for grid \a g. */
  int columns( int g ) const;
    /*! \return the total number of electrodes for grid \a g. */
  int gridChannels( int g ) const;
    /*! \return the number of electrodes in a row for the active grid. */
  int rows( void ) const;
    /*! \return the number of electrodes in a column for the active grid. */
  int columns( void ) const;
    /*! \return the total number of electrodes for the active grid. */
  int gridChannels( void ) const;
    /*! \return the total number of electrodes for all grids. */
  int channels( void ) const;
    /*! \return The currently selected row of electrodes for grid \a g. */
  int row( int g ) const;
    /*! \return The currently selected column of electrodes for grid \a g. */
  int column( int g ) const;
    /*! \return The currently selected row of electrodes for the active grid. */
  int row( void ) const;
    /*! \return The currently selected column of electrodes for the active grid. */
  int column( void ) const;
    /*! The maximum voltage to be expected to measure in volts. */
  double maxVolts( void ) const;
    /*! The unit of the voltage traces. */
  string unit( void ) const;
    /*! The sampling rate per channel to be used in hertz. */
  double sampleRate( void ) const;
    /*! The length of the time segments in seconds held in Data. */
  double dataTime( void ) const;
    /*! The time interval between succesive calls to process() in seconds. */
  double dataInterval( void ) const;
    /*! \return 0: show single trace, 1: show all traces, 2: show all traces merged into a single one. */
  int displayMode( void ) const;
    /*! \return: \c true if all traces should be displayed. */
  bool displayAll( void ) const;
    /*! \return: \c true if only a single traces should be displayed. */
  bool displaySingle( void ) const;
    /*! \return: \c true if all traces should be displayed merged into a single one. */
  bool displayMerged( void ) const;


protected:

    /*! Write current time and \a message to stderr and into a log file. */
  void printlog( const string &message ) const;


protected slots:

    /*! Calls the notify function after the dialog was accepted. */
  void callNotify( void );


private:

  QWidget *MyWidget;
  BaseWidget *BW;
  ConfigDialog Cfg;

};


#endif /* ! _ANALYZER_H_ */

