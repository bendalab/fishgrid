/*
  portaudiomonitor.h
  Plays recordings on speakers using portaudio library.

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

#ifndef _PORTAUDIOMONITOR_H_
#define _PORTAUDIOMONITOR_H_ 1

#include <portaudio.h>
#include <QMutex>
#include <relacs/sampledata.h>

using namespace std;
using namespace relacs;

class PortAudioMonitor
{

  class BrowseDataWidget;

public:

  PortAudioMonitor( void );
  int init( const BrowseDataWidget *data );
  int close( void );
  bool running( void ) const;


protected:
	
    /*! This routine is called by portaudio when playback is done. */
  static void streamFinished( void *userData );
	
    /*! This routine will be called by the PortAudio engine when audio is needed.
        It may called at interrupt level on some machines so don't do anything
	that could mess up the system like calling malloc() or free(). */
  static int callback( const void *inputBuffer, void *outputBuffer,
		       unsigned long framesPerBuffer,
		       const PaStreamCallbackTimeInfo* timeInfo,
		       PaStreamCallbackFlags statusFlags,
		       void *userData );

  PaStreamParameters OutputParameters;
  PaStream *Stream;

  const BrowseDataWidget *Data;
  double Gain;
  int Offset;
  bool Running;
  mutable QMutex Mutex;
	
};

#endif /* ! _PORTAUDIOMONITOR_H_ */

