/*
  basewidget.h
  Base class for coordination of data and analyzers

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

#ifndef _BASEWIDGET_H_
#define _BASEWIDGET_H_ 1

#include <deque>
#include <map>
#include <QMainWindow>
#include <QStackedWidget>
#include <QDateTime>
#include <relacs/sampledata.h>
#include <relacs/configclass.h>
#include <relacs/configureclasses.h>
#include <relacs/optdialog.h>
#include "configdata.h"

using namespace std;
using namespace relacs;


class Analyzer;
class PreProcessor;

/*! 
\class BaseWidget
\brief Base class for coordination of data and analyzers
\author Jan Benda

\section keys Key shortcuts

Plot selection:
- \c ALT \c ArrowLeft, \c ALT \c ArrowRight : select previous/next grid
- \c ALT \c 1, \c ALT \c 2, ... : select grid by its number
- \c I : idle. Displays nothing
- \c T : voltage trace plot
- \c P : power spectrum plots
- \c E : RMS voltage versus electrode
- \c R : RMS voltage pixels and fish trace
- \c A : show all traces
- \c S : show a single trace
- \c M : show all traces merged into a single plot
- \c ArrowLeft, \c ArrowRight, \c ArrowUp, \c ArrowDown : select electrode
- \c CTRL \c ArrowLeft, \c CTRL \c ArrowRight : select plot mode

Analysis window:
- \c L, \c + : make time window for analysis smaller by a factor 2
- \c SHIFT \c L, \c - : make time window for analysis larger by a factor 2
- \c O : launch options dialog

Pre-processor dialogs:
- \c ALT \c P : specify the preprocessor sequence
- \c ALT \c M : de-mean
- \c ALT \c C : common noise removal

General:
- \c SHIFT \c M : show window maximized/normalized

*/


class BaseWidget : public QMainWindow, public ConfigData
{
  Q_OBJECT

  friend class Analyzer;
  friend class PreProcessor;

public:

  /*! Constructs a BaseWidget for using the configuration file \a configfile. */
  BaseWidget( const string &configfile="fishgrid.cfg" );
  ~BaseWidget( void );


protected:

    /*! Sets up the data buffer and the analyzers. */
  virtual void setup( void );

  void electrodeLeft( void );
  void electrodeRight( void );
  void electrodeUp( void );
  void electrodeDown( void );
  void nextGrid( void );
  void prevGrid( void );
  void selectGrid( int g );

  virtual void keyPressEvent( QKeyEvent *event );

    /*! Add an analyzer widget with hotkey. */
  void addAnalyzer( Analyzer *a, int hotkey );

    /*! The list of available analyzer widgets. */
  deque< Analyzer* > AnalyzerWidgets;
    /*! The corresponding list of hotkeys for the analyzer widgets. */
  deque< int > AnalyzerHotkeys;
    /*! Index of the current analyzer widget. */
  int CurrentAnalyzer;
    /*! 0: single trace, 1: all traces, 2: merged. */
  int DisplayMode;

    /*! Add an preprocessor module. */
  void addPreProcessor( PreProcessor *p );

    /*! List of preprocessors that are available. */
  map< string, PreProcessor* > AvailablePreProcessors;
    /*! List of preprocessors to be applied. */
  deque< PreProcessor* > PreProcessors;
    /*! The dialog for the pre-processors. */
  OptDialog *PreProcessorDialog;

  QStackedWidget *MainWidget;


protected slots:

  void closePreProcessorDialog( int r );

};


#endif /* ! _BASEWIDGET_H_ */

