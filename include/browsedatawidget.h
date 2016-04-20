/*
  browsedatawidget.h
  BaseWidget implementation for coordinating data files and analyzers

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

#ifndef _BROWSEDATAWIDGET_H_
#define _BROWSEDATAWIDGET_H_ 1

#include <fstream>
#include <deque>
#ifdef HAVE_PORTAUDIOLIB_H
#include "portaudiomonitor.h"
#endif
#include "basewidget.h"


/*! 
\class BrowseDataWidget
\brief BaseWidget implementation for coordinating data files and analyzers
\author Jan Benda

\section keys Key shortcuts

Navigation:
- \c Q : Increase time increments (Quicker)
- \c W : Decrease time increments (sloWer)
- \c SPACE : Toggle auto increment
- \c PageUp : One page up (earlier, go left)
- \c PageDown : One page down (later, go right)
- \c ALT \c PageUp : 500 pages up (earlier, go left)
- \c ALT \c PageDown : 500 pages down (later, go right)
- \c HOME : Jump to the beginning of the recording
- \c END : Jump to the end of the recording
- \c < : Decrease channel offset of current grid (for debugging)
- \c > : Increase channel offset of current grid (for debugging)
- \c CRTL \c < : Decrease temporal offset of second board (for debugging)
- \c CRTL \c > : Increase temporal offset of second board (for debugging)

Time stamps:
- \c 1, \c  2, ... \c 9 : Jump to time stamp 1, 2, ... 9, respectively
- \c CTRL \c PageUp : Jump to preceding time stamp (next one to the left)
- \c CTRL \c PageDown : Jump to succeeding time stamp (next one to the right)

Save data:
- \c CTRL \c S : Save 1 second of data to an ascii file
- \c ALT \c S : Save changed channel and temporal offsets into current configuration fi
*/


class BrowseDataWidget : public BaseWidget
{
  Q_OBJECT


public:

  /*! Constructs a BrowseDataWidget.
      \param[in] path file name or path to data files */
  BrowseDataWidget( const string &path );
  ~BrowseDataWidget( void );

    /*! Save 1 second of data to an ascii file. */
  void saveData( void );
    /*! Save changed channel and temporal offsets into current configuration file. */
  void saveOffsets( void );


public slots:

  void showData( void );


protected:

  virtual void keyPressEvent( QKeyEvent *event );

    /*! Binary file with the voltage traces for each grid. */
  ifstream TraceFile[MaxGrids];
    /*! Size of TraceFile for each grid. */
  long long TraceSize[MaxGrids];
    /*! Index into TraceFile for each grid. */
  long long TraceIndex[MaxGrids];
    /*! Channel offset for each grid. */
  int TraceOffset[MaxGrids];
    /*! Original channel offset for each grid. */
  int OrgTraceOffset[MaxGrids];
    /*! Temporal offset of each acquisition board in samples (positive values only). */
  int TimeOffset[5];
    /*! Original temporal offset for each acquisition board. */
  int OrgTimeOffset[5];
    /*! Number of used channels per daq board. */
  int BoardChannels[4];
    /*! The minimum number of data elements before end of data for each grid. */
  int MinDataSize[MaxGrids];
    /*! Index increment for autoadvancing the trace for each grid. */
  int TraceIncr[MaxGrids];
    /*! Auto increment the traces? */
  bool AutoIncr;
    /*! The indices of the time stamps for each grid. */
  deque< long long > TimeStampIndex[MaxGrids];
    /*! The correspnding comments of the time stamps. */
  deque< string > TimeStampComment;
    /*! Full path and name of the current configuration file. */
  string ConfigPath;
    /*! Time and date of the start of the recording. */
  QDateTime StartRecTime;

#ifdef HAVE_PORTAUDIOLIB_H
    /*! Audio monitor. */
  PortAudioMonitor Audio;
#endif

};


#endif /* ! _BROWSEDATAWIDGET_H_ */

