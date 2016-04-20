/*
  recording.h
  Records data to disc

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

#include <ctime>
#include <sstream>
#include <QDir>
#include <QDateTime>
#include "recording.h"


Recording::Recording( ConfigData *cd, DataThread *dt )
  : ConfigClass( "Recording" ),
    CD( cd ),
    DT( dt ),
    Save( false ),
    PathTemplate( "%04Y-%02m-%02d-%02H:%02M" ),
    PathNumber( 0 ),
    LogFile( 0 )
{
  addText( "PathFormat", PathTemplate );

  TimeStampOpts.addInteger( "Num" ).setFlags( 1+2 );
  for ( int g=0; g<ConfigData::MaxGrids; g++ )
    TimeStampOpts.addText( "Index"+Str(g+1), "" ).setFlags( 1+2 );
  TimeStampOpts.addDate( "Date" ).setFlags( 1+2 );
  TimeStampOpts.addTime( "Time" ).setFlags( 1+2 );
  TimeStampOpts.addText( "Comment", "" ).setFlags( 1 );
}


Recording::~Recording( void )
{
}


void Recording::setDataThread( DataThread *dt )
{
  DT = dt;
}


string Recording::start( bool tracefiles, bool timestamps )
{
  if ( Save )
    return "";

  // get current time:
  time_t currenttime = ::time( 0 );
  // generate unused name for new files/directory:
  PathTemplate = text( "PathFormat" );
  Str pathname = "";
  PathNumber++;
  int az = ('z'-'a'+1);
  for ( ; PathNumber <= az*az; PathNumber++ ) {

    // create file name:
    pathname = PathTemplate;
    pathname.format( localtime( &currenttime ) );
    pathname.format( PathNumber, 'n', 'd' );
    int n = PathNumber-1;
    Str s = char( 'a' + n%az );
    n /= az;
    while ( n > 0 ) {
      s.insert( 0u, 1u, char( 'a' + n%az ) );
      n /= az;
    }
    pathname.format( s, 'a' );
    s.upper();
    pathname.format( s, 'A' );

    pathname.provideSlash();
    // try to create new directory:
    QDir curdir;
    if ( curdir.mkpath( pathname.c_str() ) )
      break;
  }
  // running out of names?
  if ( PathNumber > az*az ) {
    printlog( "! panic: FileSaver::start -> can't create data file!" );
    return "";
  }

  // set path:
  Path = pathname;

  // save configuration:
  CD->setCurrentDate( "StartDate" );
  StartRecTime = QDateTime::currentDateTime();
  CD->setTime( "StartTime", StartRecTime.time().hour(), StartRecTime.time().minute(),
	       StartRecTime.time().second(), StartRecTime.time().msec() );
  CD->CFG.setSaveStyle( Options::NoType | Options::FirstOnly );
  CD->CFG.save( 0, Path + "fishgrid.cfg" );
  CD->CFG.setSaveStyle( Options::NoType );

  // save metadata:
  ofstream xml( string( Path + "metadata.xml" ).c_str() );
  // header:
  xml << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n";
  xml << "<?xml-stylesheet type=\"text/xsl\" href=\"odml.xsl\"?>\n";
  xml << "<odML>\n";
  // recording section:
  xml << "  <section>\n";
  xml << "    <type>recording</type>\n";
  CD->Options::setText( "Configuration", Str(CD->Rows) + "x" + Str(CD->Columns) );
  CD->Options::saveXML( xml, 64, 2 );
  xml << "  </section>\n";
  // dataset:
  xml << "  <section>\n";
  xml << "    <type>dataset</type>\n";
  Parameter fp( "File", "", "" );
  if ( tracefiles ) {
    for ( int g=0; g<ConfigData::MaxGrids; g++ ) {
      if ( CD->Used[g] ) {
	fp.setText( Path + "traces-grid" + Str(g+1) + ".raw" );
	fp.saveXML( xml, 2 );
      }
    }
  }
  if ( timestamps ) {
    fp.setText( Path + "timestamps.dat" );
    fp.saveXML( xml, 2 );
  }
  fp.setText( Path + "fishgrid.cfg" );
  fp.saveXML( xml, 2 );
  fp.setText( Path + "fishgrid.log" );
  fp.saveXML( xml, 2 );
  xml << "  </section>\n";
  // setup section:
  xml << "  <section>\n";
  xml << "    <type>setup</type>\n";
  CD->Options::saveXML( xml, 32, 2 );
  xml << "  </section>\n";
  // experiment section:
  xml << "  <section>\n";
  xml << "    <type>experiment</type>\n";
  Parameter ep( CD->Options::operator[]( "Experiment.Name" ) );
  ep.setText( "FishGrid " + ep.text() );
  ep.saveXML( xml, 2 );
  CD->Options::operator[]( "Experiment.ProjectName" ).saveXML( xml, 2 );
  xml << "  </section>\n";
  // hardware properties section:
  xml << "  <section>\n";
  xml << "    <type>collection/hardware_properties</type>\n";
  xml << "    <section>\n";
  xml << "      <type>hardware/daq</type>\n";
  CD->Options::saveXML( xml, 512, 3 );
  xml << "    </section>\n";
  xml << "    <section>\n";
  xml << "      <type>hardware/amplifier</type>\n";
  CD->Options::saveXML( xml, 1024, 3 );
  xml << "    </section>\n";
  xml << "  </section>\n";
  // hardware settings section:
  xml << "  <section>\n";
  xml << "    <type>collection/hardware_settings</type>\n";
  xml << "    <section>\n";
  xml << "      <type>hardware/daq</type>\n";
  CD->Options::saveXML( xml, 128, 3 );
  xml << "    </section>\n";
  xml << "    <section>\n";
  xml << "      <type>hardware/amplifier</type>\n";
  CD->Options::saveXML( xml, 256, 3 );
  xml << "    </section>\n";
  xml << "  </section>\n";
  xml << "</odML>\n";
  xml.flush();

  // logfile:
  LogFile = new ofstream( string( Path + "fishgrid.log" ).c_str() );
  if ( ! LogFile->good() ) {
    delete LogFile;
    LogFile = 0;
    printlog( "! warning: LogFile not good" );
  }
  printlog( "this is FishGrid, version " + string( FISHGRIDVERSION ) );

  // open binary files for the traces:
  TraceFilesOpen = false;
  if ( tracefiles )
    openTraceFiles( "" );

  TimeStampsOpen = timestamps;
  if ( TimeStampsOpen ) {
    // open file for time stamps:
    TimeStampFile.open( string( Path + "timestamps.dat" ).c_str() );
    TimeStampNum = 0;
    TimeStampOpts.setInteger( "Num", TimeStampNum );
    for ( int g=0; g<ConfigData::MaxGrids; g++ ) {
      TimeStampOpts.setText( "Index"+Str(g+1), "0" );
      if ( ! CD->Used[g] )
	TimeStampOpts.delFlags( "Index"+Str(g+1), 1 );
    }
    TimeStampOpts.setCurrentDate( "Date" );
    TimeStampOpts.setTime( "Time", StartRecTime.time().hour(), StartRecTime.time().minute(),
			   StartRecTime.time().second(), StartRecTime.time().msec() );
    TimeStampOpts.setText( "Comment", "begin of recording" );
    TimeStampOpts.save( TimeStampFile );
    TimeStampFile << '\n';
    TimeStampFile.flush();
    TimeStampNum++;
    TimeStampOpts.setText( "Comment", "" );
  }

  // messages:
  printlog( "start saving data in " + Path );
  printlog( "metadata:" );
  CD->Options::save( cerr, "         ", 4, Options::FirstOnly );
  CD->Options::save( *LogFile, "         ", 4, Options::FirstOnly );
  LogFile->flush();
  
  Save = true;

  return Path;
}


void Recording::openTraceFiles( const string &name )
{
  for ( int g=0; g<ConfigData::MaxGrids; g++ ) {
    if ( CD->Used[g] ) {
      DT->lockAI( g );
      TraceFile[g].open( string( Path + "traces-grid" + Str(g+1) + name + ".raw" ).c_str(), ios::out | ios::binary );
      FirstTraceIndex[g] = (DT->inputBuffer( g ).size()/CD->GridChannels[g])*CD->GridChannels[g];
      TraceIndex[g] = FirstTraceIndex[g];
      DT->unlockAI( g );
    }
  }
  TraceFilesOpen = true;
}


void Recording::closeTraceFiles( void )
{
  if ( TraceFilesOpen ) {
    // close trace files:
    for ( int g=0; g<ConfigData::MaxGrids; g++ ) {
      if ( CD->Used[g] )
	TraceFile[g].close();
    }
    TraceFilesOpen = false;
  }
}


string Recording::save( void )
{
  if ( ! Save || ! TraceFilesOpen )
    return "";

  string message = "";
  for ( int g=0; g<ConfigData::MaxGrids; g++ ) {
    if ( CD->Used[g] ) {
      DT->lockAI( g );
      long long buffersize = DT->inputBuffer( g ).size();
      DT->unlockAI( g );
      int n = DT->inputBuffer( g ).saveBinary( TraceFile[g], TraceIndex[g], buffersize );
      if ( n > 0 ) {
	TraceIndex[g] += n;
	if ( message.empty() ) {
	  double recsecs = ((TraceIndex[g]-FirstTraceIndex[g])/CD->GridChannels[g])/CD->SampleRate;
	  qint64 recmsecs = (qint64)( ::round( 1000.0*recsecs ) );
	  QDateTime rectime = StartRecTime.addMSecs( recmsecs );
	  double rechours = floor(recsecs/3600);
	  recsecs -= 3600.0*rechours;
	  double recminutes = floor( recsecs/60 );
	  recsecs -= 60.0*recminutes;
	  message = Str( rechours, "%02.0f" ) + ":" + Str( recminutes, "%02.0f" ) + ":" + Str( recsecs, "%02.0f" ) + " " + rectime.toString( Qt::ISODate ).toStdString() + " saving data to " + Path;
	}
      }
      else if ( n < 0 ) {
	static const string errormsgs[4] = { "file not open", "nothing to be written", "request to write after buffer end", "skipped a cycle" };
	n = -n;
	string msg = "";
	if ( n < 4 )
	  msg = errormsgs[n-1];
	else
	  msg = Str( n );
	printlog( "error in saving data, saveBinary() returned " + Str( n ) + ":" + msg );
	return "save error " + msg;
      }
    }
  }
  return message;
}


void Recording::stop( void )
{
  if ( ! Save )
    return;

  double recsecs = -1.0;
  if ( TraceFilesOpen ) {
    // close trace files:
    for ( int g=0; g<ConfigData::MaxGrids; g++ ) {
      if ( CD->Used[g] ) {
	TraceFile[g].close();
	if ( recsecs < 0.0 ) {
	  long long index = TraceIndex[g] - FirstTraceIndex[g];
	  recsecs = (index/CD->GridChannels[g])/CD->SampleRate;
	}
      }
    }
    TraceFilesOpen = false;
  }

  if ( TimeStampsOpen ) {
    // final timestamp:
    TimeStampOpts.setInteger( "Num", TimeStampNum );
    for ( int g=0; g<ConfigData::MaxGrids; g++ ) {
      if ( CD->Used[g] ) {
	long long index = TraceIndex[g] - FirstTraceIndex[g];
	stringstream str;
	str << index;
	TimeStampOpts.setText( "Index"+Str(g+1), str.str() );
      }
    }
    TimeStampOpts.setCurrentDate( "Date" );
    QTime qtt = QTime::currentTime();
    TimeStampOpts.setTime( "Time", qtt.hour(), qtt.minute(), qtt.second(), qtt.msec() );
    TimeStampOpts.setText( "Comment", "end of recording" );
    TimeStampOpts.save( TimeStampFile );
    TimeStampFile << '\n';
    TimeStampNum++;
    TimeStampFile.close();
    TimeStampsOpen = false;
  }

  // close log file:
  if ( recsecs >= 0.0 ) {
    double rechours = floor(recsecs/3600);
    recsecs -= 3600.0*rechours;
    double recminutes = floor( recsecs/60 );
    recsecs -= 60.0*recminutes;
    printlog( "recording time was " + Str( rechours, "%02.0f" ) + ":" + Str( recminutes, "%02.0f" ) + ":" + Str( recsecs, "%02.0f" ) );
  }
  if ( LogFile != 0 )
    delete LogFile;
  LogFile = 0;

  // messages:
  printlog( "stop saving data" );

  // reset:
  Save = false;
  Path = "";
}


bool Recording::saving( void ) const
{
  return Save;
}


void Recording::timeStamp( void )
{
  if ( ! Save )
    return;

  TimeStampOpts.setInteger( "Num", TimeStampNum );
  for ( int g=0; g<ConfigData::MaxGrids; g++ ) {
    if ( CD->Used[g] ) {
      long long index = TraceIndex[g] - FirstTraceIndex[g];
      stringstream str;
      str << index;
      TimeStampOpts.setText( "Index"+Str(g+1), str.str() );
    }
  }
  TimeStampOpts.setCurrentDate( "Date" );
  QTime qtt = QTime::currentTime();
  TimeStampOpts.setTime( "Time", qtt.hour(), qtt.minute(), qtt.second(), qtt.msec() );
}


void Recording::saveTimeStamp( void )
{
  if ( ! Save )
    return;
  TimeStampOpts.save( TimeStampFile );
  TimeStampFile << '\n';
  TimeStampFile.flush();
  TimeStampNum++;
}


void Recording::saveTimeStamp( const string &comment )
{
  if ( ! Save )
    return;
  timeStamp();
  TimeStampOpts.setText( "Comment", comment );
  saveTimeStamp();
}


void Recording::interruptionTimeStamp( void )
{
  if ( ! Save )
    return;

  // save remaining data in the buffer:
  save();

  // setup time stamp
  Options opt( TimeStampOpts );
  opt.setInteger( "Num", TimeStampNum );
  for ( int g=0; g<ConfigData::MaxGrids; g++ ) {
    if ( CD->Used[g] ) {
      long long index = TraceIndex[g] - FirstTraceIndex[g];
      stringstream str;
      str << index;
      opt.setText( "Index"+Str(g+1), str.str() );
    }
  }
  opt.setCurrentDate( "Date" );
  QTime qtt = QTime::currentTime();
  opt.setTime( "Time", qtt.hour(), qtt.minute(), qtt.second(), qtt.msec() );
  opt.setText( "Comment", "interrupted data acquisition" );

  // save time stamp:
  opt.save( TimeStampFile );
  TimeStampFile << '\n';
  TimeStampFile.flush();
  TimeStampNum++;
}


Options &Recording::timeStampOpts( void )
{
  return TimeStampOpts;
}


void Recording::printlog( const string &message ) const
{
  cerr << QTime::currentTime().toString().toAscii().data() << " "
       << message << endl;
  if ( LogFile != 0 )
    *LogFile << QTime::currentTime().toString().toAscii().data() << " "
	     << message << endl;
}
