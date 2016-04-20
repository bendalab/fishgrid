/*
  browsedatawidget.cc
  BaseWidget implementation for coordinating data files and analyzers

  FishGrid
  Copyright (C) 2012 Jan Benda <benda@bio.lmu.de> & Joerg Henninger <henninger@bio.lmu.de>

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

#include <cmath>
#include <ctype.h>
#include <QApplication>
#include <QTimer>
#include <QDateTime>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QShortcut>
#include <relacs/optwidget.h>
#include <relacs/optdialog.h>
#include "preprocessor.h"
#include "analyzer.h"
#include "browsedatawidget.h"

using namespace std;


BrowseDataWidget::BrowseDataWidget( const string &path )
  : BaseWidget( "fishgrid.cfg" )
{
  // setup path and file names:
  Str basepath = path;
  string configfile = "fishgrid.cfg";
  string datafilebasename = "traces-grid";
  bool expandtracefile = true;
  string timestampfile = "timestamps.dat";
  if ( ! basepath.extension().empty()  ) {
    Str name = basepath.notdir();
    if ( name.extension() == ".cfg" )
      configfile = name;
    else if ( name.extension() == ".raw" ) {
      datafilebasename = name;
      expandtracefile = false;
      /*
      while ( datafilebasename.size() > 0 &&
	      isdigit( datafilebasename[datafilebasename.size()-1] ) )
	datafilebasename.resize( datafilebasename.size()-1 );
      */
    }
    basepath.stripNotdir();
  }
  else
    basepath.provideSlash();

  // read configuration file:
  ConfigClass acquisition( "Acquisition" );
  acquisition.addText( "device1", "" );
  acquisition.addInteger( "offset1", 0 );
  acquisition.addText( "blacklist1", "" );
  acquisition.addText( "device2", "" );
  acquisition.addInteger( "offset2", 0 );
  acquisition.addText( "blacklist2", "" );
  acquisition.addText( "device3", "" );
  acquisition.addInteger( "offset3", 0 );
  acquisition.addText( "blacklist3", "" );
  acquisition.addText( "device4", "" );
  acquisition.addInteger( "offset4", 0 );
  acquisition.addText( "blacklist4", "" );
  ConfigPath = basepath+configfile;
  CFG.read( 0, ConfigPath );
  Channels = 0;
  Grid = -1;
  for ( int g=0; g<MaxGrids; g++ ) {
    string ns = Str( g+1 );
    Used[g] = boolean( "Used"+ns );
    if ( Used[g] ) {
      Rows[g] = integer( "Rows"+ns, 0, 4 );
      Columns[g] = integer( "Columns"+ns, 0, 4 );
      GridChannels[g] = Rows[g]*Columns[g];
      Channels += GridChannels[g];
      TraceOffset[g] = integer( "ChannelOffset"+ns, 0, 0 );
      cerr << "READ IN CHANNEL OFFSET " << TraceOffset[g] << '\n';
      if ( Grid < 0 )
	Grid = g;
    }
    else
      TraceOffset[g] = 0;
    OrgTraceOffset[g] = TraceOffset[g];
  }
  SampleRate = number( "AISampleRate", 10000.0 );
  MaxVolts = number( "AIMaxVolt", 1.0 );
  Gain = number( "Gain", 1.0 );
  DataTime = number( "DataTime", 0.1 );
  DataInterval = number( "DataInterval", 1.0 );
  int year, month, day, hour, minutes, seconds, milliseconds;
  date( "StartDate", 0, year, month, day );
  time( "StartTime", 0, hour, minutes, seconds, milliseconds );
  StartRecTime = QDateTime( QDate( year, month, day ), QTime( hour, minutes, seconds, milliseconds ) );
  for ( int board=0; board<4; board++ ) {
    string ns = Str( board+1 );
    TimeOffset[board] = acquisition.integer( "offset"+ns, 0, 0 );
    OrgTimeOffset[board] = TimeOffset[board];

    Str blacklist = acquisition.text( "blacklist" + ns );
    vector<int> blackchannels;
    blacklist.range( blackchannels, ",", "-" );
    BoardChannels[board] = 32 - blackchannels.size();
  }
  OrgTimeOffset[4] = TimeOffset[4] = 0;
  cerr << "READ IN TEMPORAL OFFSET " << TimeOffset[1] << '\n';

  setup();

  // read in time stamps:
  Options tsopt;
  tsopt.addInteger( "Num" );
  for ( int g=0; g<MaxGrids; g++ )
    tsopt.addNumber( "Index"+Str(g+1), "" );
  tsopt.addDate( "Date" );
  tsopt.addTime( "Time" );
  tsopt.addText( "Comment", "" );
  ifstream tsf( string( basepath + timestampfile ).c_str() );
  for ( int g=0; g<MaxGrids; g++ )
    TimeStampIndex[g].clear();
  TimeStampComment.clear();
  while ( tsf.good() ) {
    tsopt.setFlags( 0 );
    tsopt.read( tsf, 0, ":=", "", StrQueue::StopEmpty );
    if ( tsopt.flags( "Num" ) == 0 )
      break;
    for ( int g=0; g<MaxGrids; g++ ) {
      if ( Used[g] )
	TimeStampIndex[g].push_back( (long long)tsopt.number( "Index" + Str(g+1) ) );
    }
    TimeStampComment.push_back( tsopt.text( "Comment" ) );
    cerr << Str( tsopt.integer( "Num" ), "%02d" ) << " "
	 << tsopt.text( "Date" ) << " "
	 << tsopt.text( "Time" ) << " "
	 << tsopt.text( "Comment" ) << '\n';
  }

  // open data files:
  for ( int g=0; g<MaxGrids; g++ ) {
    if ( Used[g] ) {
      TraceIncr[g] = (int)::floor(DataTime*SampleRate)*GridChannels[g];
      DataInterval = TraceIncr[g]/GridChannels[g]/SampleRate;
      if ( expandtracefile ) {
	TraceFile[g].open( string( basepath + datafilebasename + Str( g+1 ) + ".raw" ).c_str(), ios::in | ios::binary );
	cerr << "Open trace file " << basepath << datafilebasename << Str( g+1 ) << ".raw" << '\n';
      }
      else {
	TraceFile[g].open( string( basepath + datafilebasename ).c_str(), ios::in | ios::binary );
	cerr << "Open trace file " << basepath << datafilebasename << '\n';
      }
      TraceFile[g].seekg( 0, ios::end );
      TraceSize[g] = TraceFile[g].tellg()/sizeof(float);
      TraceIndex[g] = 0;
      MinDataSize[g] = (int)::floor(DataTime*SampleRate)*GridChannels[g];
    }
  }
  AutoIncr = true;
  ProcessInterval = 100;
    
  QTimer::singleShot( ProcessInterval, this, SLOT( showData() ) );
}


BrowseDataWidget::~BrowseDataWidget( void )
{
}


void BrowseDataWidget::showData( void )
{
  int maxtimeoffset = 0;
  int basetimeoffset = 0;
  for ( int board=0; board<4; board++ ) {
    if ( maxtimeoffset < TimeOffset[board] )
      maxtimeoffset = TimeOffset[board];
    if ( basetimeoffset > TimeOffset[board] )
      basetimeoffset = TimeOffset[board];
  }
  basetimeoffset = -basetimeoffset;
  if ( maxtimeoffset < basetimeoffset )
    maxtimeoffset = basetimeoffset;

  int gridchannels = 0;
  int boardinx = 0;
  int boardchannels = 0;
  int firstinx = -1;
  for ( int g=0; g<MaxGrids; g++ ) {
    if ( Used[g] ) {
      if ( firstinx < 0 )
	firstinx = g;
      // clear analysis buffer:
      for ( int r=0; r<Rows[g]; r++ ) {
	for ( int c=0; c<Columns[g]; c++ ) {
	  Data[g][r][c].resize( 0.0, DataTime, 1.0/SampleRate, 0.0F );
	  Data[g][r][c].clear();
	}
      }
      // read data:
      int datasize = ((int)::floor(DataTime*SampleRate)+maxtimeoffset)*GridChannels[g];
      float buffer[datasize];
      if ( ! TraceFile[g].good() )
	TraceFile[g].clear();
      TraceFile[g].seekg( (TraceIndex[g]+TraceOffset[g])*sizeof( float ) );
      TraceFile[g].read( (char *)buffer, datasize*sizeof( float ) );
      int n = TraceFile[g].gcount();
      n /= sizeof( float );
      while ( boardchannels + BoardChannels[boardinx] < gridchannels )
	boardchannels += BoardChannels[boardinx++];
      int nextboardchannel = boardchannels + BoardChannels[boardinx] - gridchannels;
      int toffs0 = (basetimeoffset+TimeOffset[boardinx])*GridChannels[g];
      int toffs1 = (basetimeoffset+TimeOffset[boardinx+1])*GridChannels[g];
      int maxtoffs = toffs0;
      if ( maxtoffs < toffs1 )
	maxtoffs = toffs1;
      // put them into data:
      int inx = 0;
      while ( inx+maxtoffs<n-GridChannels[g] ) {
	int toffs = toffs0;
	int channel = 0;
	for ( int r=0; r<Rows[g]; r++ ) {
	  for ( int c=0; c<Columns[g]; c++ ) {
	    Data[g][r][c].push( 1000.0*buffer[toffs + inx++] );  // convert to Millivolt
	    channel++;
	    if ( channel >= nextboardchannel )
	      toffs = toffs1;
	  }
	}
      }
    }
    gridchannels += GridChannels[g];
  }

  // window title:
  double recsecs = (TraceIndex[firstinx]/GridChannels[firstinx])/SampleRate;
  qint64 recmsecs = (qint64)( ::round( 1000.0*recsecs ) );
  QDateTime rectime = StartRecTime.addMSecs( recmsecs );
  double rechours = floor(recsecs/3600);
  recsecs -= 3600.0*rechours;
  double recminutes = floor( recsecs/60 );
  recsecs -= 60.0*recminutes;
  string wts = "FishGrid @ " + Str( rechours, "%02.0f" ) + ":" + Str( recminutes, "%02.0f" ) + ":" + Str( recsecs, "%02.0f" ) + " " + rectime.toString( Qt::ISODate ).toStdString();

  // preprocessing:
  for ( deque< PreProcessor* >::iterator pp = PreProcessors.begin();
	pp != PreProcessors.end();
	++pp ) {
    string s = (*pp)->process( Data );
    if ( ! s.empty() )
      wts += " " + s;
  }

  // audio:
#ifdef HAVE_PORTAUDIOLIB_H
  if ( ! Audio.running() ) {
    Audio.init( Data[Grid][Row[Grid]][Column[Grid]] );
    cerr << "Started Audio\n";
  }
#endif

  // channel offset:
  if ( TraceOffset[Grid] != 0 )
    wts += " channel offset=" + Str( TraceOffset[Grid] );

  // temporal offset:
  if ( TimeOffset[1] != 0 )
    wts += " time offset=" + Str( TimeOffset[1] );

  // analyze and plot:
  setWindowTitle( wts.c_str() );
  AnalyzerWidgets[CurrentAnalyzer]->process( Data );

  // advance:
  if ( AutoIncr ) {
    for ( int g=0; g<MaxGrids; g++ ) {
      if ( Used[g] ) {
	TraceIndex[g] += TraceIncr[g];
	if ( TraceIndex[g] > TraceSize[g]-MinDataSize[g] ) {
	  AutoIncr = false;
	  TraceIndex[g] = TraceSize[g]-MinDataSize[g];
	  TraceIndex[g] = (TraceIndex[g]/GridChannels[g])*GridChannels[g];
	  if ( TraceIndex[g] < 0 )
	    TraceIndex[g] = 0;
	}
      }
    }
  }
    
  QTimer::singleShot( ProcessInterval, this, SLOT( showData() ) );
}


void BrowseDataWidget::saveData( void )
{
  for ( int g=0; g<MaxGrids; g++ ) {
    if ( Used[g] ) {
      // read data:
      int datasize = (int)::floor(1.0*SampleRate)*GridChannels[g];
      float buffer[datasize];
      if ( ! TraceFile[g].good() )
	TraceFile[g].clear();
      TraceFile[g].seekg( (TraceIndex[g]+TraceOffset[g])*sizeof( float ) );
      TraceFile[g].read( (char *)buffer, datasize*sizeof( float ) );
      int n = TraceFile[g].gcount();
      n /= sizeof( float );
      // open file:
      string datafile( "traces-grid" + Str(g+1) + "-" + Str( TraceIndex[g]/SampleRate, "%.3f" ) + "s.dat" );
      ofstream df( datafile.c_str() );
      df << "#Key\n";
      df << "# Time      ";
      for ( int r=0; r<Rows[g]; r++ ) {
	for ( int c=0; c<Columns[g]; c++ )
	  df << "r" << Str( r+1, 2, '0' ) << "-c" << Str( c+1, 2, '0' ) + "   ";
      }
      df << '\n';
      df << "# ms        ";
      for ( int r=0; r<Rows[g]; r++ ) {
	for ( int c=0; c<Columns[g]; c++ )
	  df << Str( Unit, -10, ' ' );
      }
      df << '\n';
      // write to file:
      int inx = 0;
      while ( inx<n-GridChannels[g] ) {
	df << "  " << Str( 1000.0*inx/GridChannels[g]/SampleRate, "%8.2f" );
	for ( int r=0; r<Rows[g]; r++ ) {
	  for ( int c=0; c<Columns[g]; c++ )
	    df << "  " << Str( 1000.0*buffer[inx++], "%8.3f" );  // convert to Millivolt
	}
	df << '\n';
      }
      printlog( "wrote data to file " + datafile );
    }
  }
}


void BrowseDataWidget::saveOffsets( void )
{
  bool channelchanged = false;
  for ( int g=0; g<MaxGrids; g++ ) {
    if ( Used[g] && TraceOffset[g] != OrgTraceOffset[g] ) {
      channelchanged = true;
      break;
    }
  }
  bool timechanged = false;
  for ( int board=0; board<4; board++ ) {
    if ( TimeOffset[board] != OrgTimeOffset[board] ) {
      timechanged = true;
      break;
    }
  }
  if ( ( ! channelchanged ) && ( ! timechanged ) )
    return;

  cout << "Read in '" << ConfigPath << "' ...\n";
  ifstream sf( ConfigPath.c_str() );
  deque< StrQueue > config;
  string line = "";
  StrQueue sq;
  while ( true ) {
    sq.clear();
    sq.load( sf, "*", &line ).good();
    config.push_back( sq );
    if ( ! sf.good() )
      break;
  }
  sf.close();

  cout << "Write out '" << ConfigPath << "' ...\n";
  ofstream df( ConfigPath.c_str() );
  for ( unsigned int k=0; k<config.size(); k++ ) {
    if ( config[k].empty() )
      continue;
    string title = config[k][0];
    df << title << '\n';
    config[k].erase( 0 );
    // Fix empty and comma options (fo backwards compatibility):
    for ( int i=0; i<config[k].size(); i++ ) {
      int f = config[k][i].findFirstNot( Str::WhiteSpace );
      int p = config[k][i].findLastNot( Str::WhiteSpace );
      if ( f >= 4 && p > 0 && config[k][i][p] == ':' )
	config[k][i] += " ~";
      string val = config[k][i].value( 0, ":" );
      if ( val.find( ',' ) != string::npos && val[0] != '"' ) {
	config[k][i].erase( config[k][i].find( ':' ) );
	config[k][i] += ": \"" + val + "\"";
      }
    }
    Options opt( config[k] );
    if ( channelchanged && config[k].size() > 0 && title == "*FishGrid" ) {
      for ( int g=0; g<MaxGrids; g++ ) {
	string ns = Str( g+1 );
	if ( !opt.exist( "ChannelOffset"+ns ) )
	  opt.insertInteger( "ChannelOffset"+ns, "ElectrodeType"+ns, 0 );
	opt.setInteger( "ChannelOffset"+ns, TraceOffset[g] );
      }
    }
    else if ( timechanged && config[k].size() > 0 && title == "*Acquisition" ) {
      for ( int board=0; board<4; board++ ) {
	string ns = Str( board+1 );
	if ( !opt.exist( "offset"+ns ) )
	  opt.insertInteger( "offset"+ns, "blacklist"+ns, 0 );
	opt.setInteger( "offset"+ns, TimeOffset[board] );
      }
    }
    opt.save( df, "  " );
    df << '\n';
  }
}


void BrowseDataWidget::keyPressEvent( QKeyEvent *event )
{
  BaseWidget::keyPressEvent( event );
  if ( event->isAccepted() )
    return;

  if ( event->key() >= Qt::Key_1 &&
       event->key() <= Qt::Key_9 ) {
    int tsinx = event->key()-Qt::Key_0;
    if ( tsinx >= 0 && tsinx < (int)TimeStampComment.size() ) {
      AutoIncr = false;
      for ( int g=0; g<MaxGrids; g++ ) {
	if ( Used[g] ) {
	  TraceIndex[g] = TimeStampIndex[g][tsinx];
	  if ( tsinx == (int)TimeStampIndex[g].size()-1 )
	    TraceIndex[g] -= MinDataSize[g];
	  TraceIndex[g] = (TraceIndex[g]/GridChannels[g])*GridChannels[g];
	}
      }
      cerr << Str( tsinx, "%02d" ) << " " << TimeStampComment[tsinx] << '\n';
    }
    event->accept();
    return;
  }

  switch ( event->key() ) {

  case Qt::Key_Space :
    AutoIncr = ! AutoIncr;
    break;

  case Qt::Key_Q :
    for ( int g=0; g<MaxGrids; g++ ) {
      if ( Used[g] ) {
	TraceIncr[g] *= 2;
	TraceIncr[g] = (TraceIncr[g]/GridChannels[g])*GridChannels[g];
	DataInterval = TraceIncr[g]/GridChannels[g]/SampleRate;
      }
    }
    break;

  case Qt::Key_W :
    for ( int g=0; g<MaxGrids; g++ ) {
      if ( Used[g] ) {
	TraceIncr[g] /= 2;
	TraceIncr[g] = (TraceIncr[g]/GridChannels[g])*GridChannels[g];
	DataInterval = TraceIncr[g]/GridChannels[g]/SampleRate;
      }
    }
    break;

  case Qt::Key_PageUp :
    if ( event->modifiers() & Qt::ControlModifier ) {
      // first grid:
      int fg = 0;
      for ( int g=0; g<MaxGrids; g++ ) {
	if ( Used[g] ) {
	  fg = g;
	  break;
	}
      }
      // previous time stamp:
      for ( unsigned int tsinx=TimeStampIndex[fg].size()-1; tsinx >=0; tsinx-- ) {
	if ( TimeStampIndex[fg][tsinx] < TraceIndex[fg] ) {
	  AutoIncr = false;
	  for ( int g=0; g<MaxGrids; g++ ) {
	    if ( Used[g] ) {
	      TraceIndex[g] = TimeStampIndex[g][tsinx];
	      if ( tsinx == TimeStampIndex[g].size()-1 )
		TraceIndex[g] -= MinDataSize[g];
	      TraceIndex[g] = (TraceIndex[g]/GridChannels[g])*GridChannels[g];
	    }
	  }
	  cerr << Str( tsinx, "%02d" ) << " " << TimeStampComment[tsinx] << '\n';
	  break;
	}
      }
    }
    else if ( event->modifiers() & Qt::AltModifier ) {
      // 500 pages up:
      AutoIncr = false;
      for ( int g=0; g<MaxGrids; g++ ) {
	if ( Used[g] ) {
	  TraceIndex[g] -= 500 * TraceIncr[g];
	  if ( TraceIndex[g] < 0 )
	    TraceIndex[g] = 0;
	}
      }
    }
    else {
      // one page up:
      AutoIncr = false;
      for ( int g=0; g<MaxGrids; g++ ) {
	if ( Used[g] ) {
	  TraceIndex[g] -= TraceIncr[g];
	  if ( TraceIndex[g] < 0 )
	    TraceIndex[g] = 0;
	}
      }
    }
    break;

  case Qt::Key_PageDown :
    if ( event->modifiers() & Qt::ControlModifier ) {
      // first grid:
      int fg = 0;
      for ( int g=0; g<MaxGrids; g++ ) {
	if ( Used[g] ) {
	  fg = g;
	  break;
	}
      }
      // next time stamp:
      for ( unsigned int tsinx=0; tsinx < TimeStampIndex[fg].size(); tsinx++ ) {
	if ( TimeStampIndex[fg][tsinx] > TraceIndex[fg] ) {
	  AutoIncr = false;
	  for ( int g=0; g<MaxGrids; g++ ) {
	    if ( Used[g] ) {
	      TraceIndex[g] = TimeStampIndex[g][tsinx];
	      if ( tsinx == TimeStampIndex[g].size()-1 )
		TraceIndex[g] -= MinDataSize[g];
	      TraceIndex[g] = (TraceIndex[g]/GridChannels[g])*GridChannels[g];
	    }
	  }
	  cerr << Str( tsinx, "%02d" ) << " " << TimeStampComment[tsinx] << '\n';
	  break;
	}
      }
    }
    else if ( event->modifiers() & Qt::AltModifier ) {
      // 500 pages down:
      AutoIncr = false;
      for ( int g=0; g<MaxGrids; g++ ) {
	if ( Used[g] ) {
	  TraceIndex[g] += 500 * TraceIncr[g];
	  if ( TraceIndex[g] > TraceSize[g]-MinDataSize[g] ) {
	    TraceIndex[g] = TraceSize[g]-MinDataSize[g];
	    TraceIndex[g] = (TraceIndex[g]/GridChannels[g])*GridChannels[g];
	  }
	}
      }
    }
    else {
      // one page down:
      AutoIncr = false;
      for ( int g=0; g<MaxGrids; g++ ) {
	if ( Used[g] ) {
	  TraceIndex[g] += TraceIncr[g];
	  if ( TraceIndex[g] > TraceSize[g]-MinDataSize[g] ) {
	    TraceIndex[g] = TraceSize[g]-MinDataSize[g];
	    TraceIndex[g] = (TraceIndex[g]/GridChannels[g])*GridChannels[g];
	  }
	}
      }
    }
    break;

  case Qt::Key_Home :
    AutoIncr = false;
    for ( int g=0; g<MaxGrids; g++ )
      TraceIndex[g] = 0;
    break;

  case Qt::Key_End :
    AutoIncr = false;
    for ( int g=0; g<MaxGrids; g++ ) {
      if ( Used[g] ) {
	TraceIndex[g] = TraceSize[g]-MinDataSize[g];
	TraceIndex[g] = (TraceIndex[g]/GridChannels[g])*GridChannels[g];
      }
    }
    break;

  case Qt::Key_S :
    if ( event->modifiers() & Qt::ControlModifier ) {
      saveData();
    }
    else if ( event->modifiers() & Qt::AltModifier ) {
      saveOffsets();
    }
    else {
      event->ignore();
      return;
    }
    break;

  case Qt::Key_Greater :
    if ( event->modifiers() & Qt::ControlModifier ) {
      TimeOffset[1]++;
    }
    else {
      if ( TraceOffset[Grid] > -GridChannels[Grid]+1 )
	TraceOffset[Grid]--;
    }
    break;

  case Qt::Key_Less :
    if ( event->modifiers() & Qt::ControlModifier ) {
      TimeOffset[1]--;
    }
    else {
      if ( TraceOffset[Grid] < GridChannels[Grid]-1 )
	TraceOffset[Grid]++;
    }
    break;

  default:
    event->ignore();
    return;
  }

  event->accept();
}


#include "moc_browsedatawidget.cc"

