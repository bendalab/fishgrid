/*
  fishgrid.cc
  

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

#include <cstdlib>
#include <string>
#include <getopt.h>
#include <QApplication>
#include "browsedatawidget.h"
#include "fishgridwidget.h"

using namespace std;


void usage( void )
{
  cout << "FishGrid " << FISHGRIDVERSION << endl;
  cout << "Copyright (C) 2009-2011 Jan Benda & Joerg Henninger\n";
  cout << "\n";
  cout << "Usage:\n";
  cout << "\n";
  cout << "FishGrid -f SAMPLINGRATE -g GAIN -v MAXVOLT -b BUFFERTIME -d DATATIME -n -s -t TIME -m -3 DATAFILE\n";
  cout << "\n";
  cout << "-f SAMPLINGRATE     sampling rate in hertz for each channel\n";
  cout << "-g GAIN             gain factor of the amplifiers\n";
  cout << "-v MAXVOLT          maximum expected voltage at the electrode in millivolt\n";
  cout << "-b BUFFERTIME       time in seconds, the internal buffer should hold the data\n";
  cout << "-d DATATIME         size of the data segments in milliseconds on which analysis operates on\n";
  cout << "-p PROCESSINTERVAL  time between successive calls of the analysis and plot functions in milliseconds\n";
  cout << "-n                  don't open the configuration dialog on startup\n";
  cout << "-s                  start saving right away\n";
  cout << "-t TIME             stop saving and quit at time TIME in hh:mm:ss format\n";
  cout << "-m                  open with maximized window\n";
  cout << "-3                  simulation (dry) mode\n";
  cout << "DATAFILE            browse the existing data file (raw data, config file, or path)\n";
  exit( 0 );
}


int main( int argc, char **argv )
{
  double samplerate = -1.0;
  double gain = 0.0;
  double maxvolts = -1.0;
  double buffertime = -1.0;
  double datatime = -1.0;
  double datainterval = -1.0;
  bool maximize = false;
  bool dialog = true;
  bool saving = false;
  string stoptime = "";
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
  while ( (c = getopt_long( argc, argv, "f:g:v:b:d:p:t:mns3", longoptions, &longindex )) >= 0 ) {
    switch ( c ) {
    case 0: switch ( longindex ) {
      case 0:
	cout << "FishGrid " << FISHGRIDVERSION << endl;
	cout << "Copyright (C) 2009-2011 Jan Benda & Joerg Henninger\n";
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

    case 'd':
      if ( optarg != NULL ) {
	char *ep = NULL;
	double val = strtod( optarg, &ep );
	if ( ep != optarg )
	  datatime = 0.001*val;
      }
      break;

    case 'p':
      if ( optarg != NULL ) {
	char *ep = NULL;
	double val = strtod( optarg, &ep );
	if ( ep != optarg )
	  datainterval = 0.001*val;
      }
      break;

    case 't':
      if ( optarg != NULL )
	stoptime = optarg;
      break;

    case 'm':
      maximize = true;
      break;

    case 'n':
      dialog = false;
      break;

    case 's':
      saving = true;
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

  if ( argc > optind ) {
    // browse existing data:
    QApplication app( argc, argv );
    BrowseDataWidget fishgrid( argv[optind] );
    if ( maximize )
      fishgrid.showMaximized();
    fishgrid.show();
    return app.exec();
  }
  else {
    // record new data:
    QApplication app( argc, argv );
    FishGridWidget fishgrid( samplerate, maxvolts, gain,
			     buffertime, datatime, datainterval,
			     dialog, saving, stoptime, simulate );
    if ( maximize )
      fishgrid.showMaximized();
    fishgrid.show();
    return app.exec();
  }
}
