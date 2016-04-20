/*
  fishgridrecorder.cc
  

  FishGridRecorder
  Copyright (C) 2011 Jan Benda <benda@bio.lmu.de> & Joerg Henninger <henninger@bio.lmu.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.
  
  FishGridRecorder is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdlib>
#include <string>
#include <getopt.h>
#include "recorder.h"

using namespace std;


void usage( void )
{
  cout << "FishGridRecorder " << FISHGRIDVERSION << endl;
  cout << "Copyright (C) 2011 Jan Benda & Joerg Henninger\n";
  cout << "\n";
  cout << "Usage:\n";
  cout << "\n";
  cout << "FishGridRecorder -f SAMPLINGRATE -g GAIN -v MAXVOLT -b BUFFERTIME -3\n";
  cout << "\n";
  cout << "-f SAMPLINGRATE     sampling rate in hertz for each channel\n";
  cout << "-g GAIN             gain factor of the amplifiers\n";
  cout << "-v MAXVOLT          maximum expected voltage at the electrode in millivolt\n";
  cout << "-b BUFFERTIME       time in seconds, the internal buffer should hold the data\n";
  cout << "-3                  simulation (dry) mode\n";
  exit( 0 );
}


int main( int argc, char **argv )
{
  double samplerate = -1.0;
  double gain = 0.0;
  double maxvolts = -1.0;
  double buffertime = -1.0;
  bool simulate = false;

  static struct option longoptions[] = {
    { "version", 0, 0, 0 },
    { "help", 0, 0, 0 },
    { 0, 0, 0, 0 }
  };
  optind = 0;
  opterr = 0;
  int longindex = 0;
  char c;
  while ( (c = getopt_long( argc, argv, "f:g:v:b:3", longoptions, &longindex )) >= 0 ) {
    switch ( c ) {
    case 0: switch ( longindex ) {
      case 0:
	cout << "FishGrid " << FISHGRIDVERSION << endl;
	cout << "Copyright (C) 2011 Jan Benda & Joerg Henninger\n";
	exit( 0 );
	break;
      case 1:
	usage();
	break;
      }
      break;

    case 'f':
      if ( optarg != NULL ) {
	char *ep = NULL;
	double val = strtod( optarg, &ep );
	if ( ep != optarg )
	  samplerate = val;
      }
      break;

    case 'g':
      if ( optarg != NULL ) {
	char *ep = NULL;
	double val = strtod( optarg, &ep );
	if ( ep != optarg )
	  gain = val;
      }
      break;

    case 'v':
      if ( optarg != NULL ) {
	char *ep = NULL;
	int val = strtol( optarg, &ep, 10 );
	if ( ep != optarg )
	  maxvolts = 0.001*val;
      }
      break;

    case 'b':
      if ( optarg != NULL ) {
	char *ep = NULL;
	double val = strtod( optarg, &ep );
	if ( ep != optarg )
	  buffertime = val;
      }
      break;

    case '3':
      simulate = true;
      break;
      
    default:
      cout << "invalid command line argument '-" << c << "' !\n";
      cout << '\n';
      usage();
      break;
    }
  }

  // record new data:
  Recorder fishgrid( samplerate, maxvolts, gain,
		     buffertime, simulate );

  if ( fishgrid.start() < 0 )
    return -1;
  for ( ; ; ) {
    fishgrid.sleep();
    fishgrid.processData();
  }
  fishgrid.finish();
  return 0;

  /*
  return fishgrid.serverLoop();
  */
}
