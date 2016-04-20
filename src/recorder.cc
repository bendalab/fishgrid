/*
  recorder.cc
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

#include "datathread.h"
#include "simulationthread.h"
#ifdef HAVE_COMEDILIB_H
#include "comedithread.h"
#endif
#ifdef HAVE_NIDAQMXBASE_H
#include "nidaqmxthread.h"
#endif
#include "recorder.h"

using namespace std;


Recorder::Recorder( double samplerate,
		    double maxvolts, double gain,
		    double buffertime, bool simulate )
  : ConfigData( "fishgrid.cfg" ),
    FileSaver( this ),
    DataLoop( 0 ),
    FileSaving( false )
{
  MaxVolts = maxvolts;
  SampleRate = samplerate;
  Gain = gain;
  Unit = "mV";
  BufferTime = buffertime;

  // set up the data thread:
  DataThread *acq = 0;
#ifdef HAVE_COMEDILIB_H
  acq = new ComediThread( this );
#else
#ifdef HAVE_NIDAQMXBASE_H
  acq = new NIDAQmxThread( this );
#endif
#endif
  if ( acq == 0 || simulate ) {
    if ( acq != 0 )
      delete acq;
    printlog( "using simulation" );
    DataLoop = new SimulationThread( this );
  }
  else {
    printlog( "using data aquisition board" );
    DataLoop = acq;
  }
  FileSaver.setDataThread( DataLoop );

  // read configuration:
  CFG.read();

  for ( int g=0; g<MaxGrids; g++ ) {
    string ns = Str( g+1 );
    Used[g] = boolean( "Used"+ns );
    if ( Used[g] ) {
      Rows[g] = integer( "Rows"+ns, 0, 4 );
      Columns[g] = integer( "Columns"+ns, 0, 4 );
    }
  }

  if ( SampleRate <= 0 )
    SampleRate = number( "AISampleRate", 10000.0 );
  setNumber( "AISampleRate", SampleRate );

  if ( MaxVolts <= 0 )
    MaxVolts = number( "AIMaxVolt", 1.0 );
  setNumber( "AIMaxVolt", MaxVolts );

  if ( Gain == 0 )
    Gain = number( "Gain", 1.0 );
  setNumber( "Gain", Gain );

  if ( BufferTime <= 0 )
    BufferTime = number( "BufferTime", 60.0 );
  setNumber( "BufferTime", BufferTime );

  if ( DataTime <= 0 )
    DataTime = number( "DataTime", 0.1 );
  setNumber( "DataTime", DataTime );

  if ( DataInterval <= 0 )
    DataInterval = number( "DataInterval", 1.0 );
  setNumber( "DataInterval", DataInterval );

  // write configuration to console:
  Channels = 0;
  Grid = -1;
  for ( int g=0; g<MaxGrids; g++ ) {
    if ( Used[g] ) {
      printlog( "Grid " + Str( g+1 ) );
      printlog( "  Rows=" +Str( Rows[g] ) );
      printlog( "  Columns=" +Str( Columns[g] ) );
      GridChannels[g] = Rows[g]*Columns[g];
      printlog( "  Channels=" +Str( GridChannels[g] ) );
      Channels += GridChannels[g];
      if ( Grid < 0 )
	Grid = g;
    }
  }
  setInteger( "AIUsedChannelCount", Channels );
  printlog( "Channels=" +Str( Channels ) );
  printlog( "SampleRate=" +Str( SampleRate ) + "Hz" );
  printlog( "MaxVolts=" +Str( MaxVolts, "%.3f" ) + "V" );
  printlog( "Gain=" +Str( Gain ) );
  printlog( "BufferTime=" +Str( BufferTime ) + "s" );
  printlog( "DataTime=" +Str( DataTime ) + "s" );
  printlog( "DataInterval=" +Str( DataInterval ) + "s" );

  ConfigData::setup();
}


Recorder::~Recorder( void )
{
}


int Recorder::start( void )
{
  printlog( "start acquisition" );
  int r = DataLoop->start();
  if ( r >= 0 ) {
    FileSaver.start();
  }
  else
    printlog( "error in starting data thread." );
  return r;
}


void Recorder::printlog( const string &message ) const
{
  FileSaver.printlog( message );
}


int Recorder::processData( void )
{
  // error in acquisition:
  if ( ! DataLoop->running() ) {
    FileSaver.interruptionTimeStamp();
    int r = DataLoop->start();
    if ( r != 0 ) {
      printlog( "error in restarting data thread." );
      return r;
    }
  }

  // save data:   
  else
    FileSaver.save();

  return 0;
}


void Recorder::finish( void )
{
  FileSaver.stop();
  printlog( "quitting FishGridRecorder" );
  CFG.save();
  DataLoop->stop();
}


void Recorder::sleep( void )
{
  QThread::msleep( ProcessInterval );
}


void Recorder::run( void )
{
  bool fs = true;
  do {
    sleep();
    processData();
    FileSavingMutex.lock();
    fs = FileSaving;
    FileSavingMutex.unlock();
  } while ( fs );
}

/*
int Recorder::readCommand( int sock ) 
{
  struct MetaData metaData = {0, ""};
  float *data = 0;	
  int recvd = recv( sock, metaData, &data );
  if (recvd < 0 ) {
    perror("recv() failed");
    if ( data != 0 )
      free( data );
    return -1;
  }
  string command( metaData.Command );
  cout << "metaData.text: " << metaData.Command << endl;
  cout << "metaData.sizeData: " << metaData.SizeData << endl;
  //	cout << "data.size(): " << data.size() << endl;
  if ( command == "config" ) {
  }
  else if ( command == "startacq" ) {
    printlog( "start acquisition" );
    int r = DataLoop->start();
    if ( r < 0 )
      printlog( "error in starting data thread." );
  }
  else if ( command == "stopacq" ) {
    FileSavingMutex.lock();
    bool fs = FileSaving;
    FileSavingMutex.unlock();
    if ( ! fs )
      DataLoop->stop();
  }
  else if ( command == "startrec" ) {
    FileSavingMutex.lock();
    bool fs = FileSaving;
    FileSavingMutex.unlock();
    if ( ! fs ) {
      FileSavingMutex.lock();
      FileSaving = true;
      FileSavingMutex.unlock();
      FileSaver.start();
      QThread::start();
      printlog( "Started recording to files." );
    }
  }
  else if ( command == "stoprec" ) {
    FileSavingMutex.lock();
    bool fs = FileSaving;
    FileSavingMutex.unlock();
    if ( fs ) {
      FileSavingMutex.lock();
      FileSaving = false;
      FileSavingMutex.unlock();
      wait();
      FileSaver.stop();
      printlog( "Stopped recording to files." );
    }
  }
  else if ( command == "startsend" ) {
  }
  else if ( command == "stopsend" ) {
  }
  else
    cerr << "unknown command " << command << '\n';

  // INSERT LOGIC HERE ....

  if ( data != 0 )
    free( data );

  // *	
  // send something
  char hello[20] = {"Hello "};
  strcat( hello, metaData.text);
  strcpy( metaData.text , hello );
  metaData.sizeData = 0;
	
  int sentlast = 0; // number of bytes that have been sent with last call
  sentlast = Send( sock, metaData, 0 );
  if ( sentlast == -1)
    {
      perror("send() failed");
      return 1;
    }
//  * /	
  return recvd;
}


int Recorder::serverLoop( void )
{	
  int sock = 0;
  int port = 5000;
	
  // Starte Server-Socket
  if ((sock = socket()) < 0 ) {
    perror("Socket");
    exit(1);
  }
	
  bind(sock, port);
  listen(sock, 3);

  cerr << "\nTCPServer Waiting for client on port 5000\n";
	
  cerr << "Server socket is: " << sock << endl;
  // sock is the server socket
  // do once
  int c, max, sender;
  fd_set fds;
  struct list_type list;
  socklen_t sin_size;

  init_list(&list);
  add_client(&list, STDIN_FILENO);
	 
  // do forever
  for ( ; ; ) {
    FD_ZERO(&fds); // clear set
    max = fill_set(&fds, &list); // fill set and get highest file descriptor

    FD_SET(sock, &fds); // set entry
    if (sock > max) // check, if the server socket is the highest file descriptor
      max = sock;		  
		  
    // start select with the new list // select BLOCKS until something happens
    cerr << "starting select" << endl;
    // define timeout for select
    int f = 0;
    if ( list.count > 1 ) {
      // select with timeout
      struct timeval timeout = {10,0}; // sec, usec
      f = select(max + 1, &fds, NULL, NULL, &timeout); 
    }
    else {
      // select without timeout
      f = select(max + 1, &fds, NULL, NULL, NULL); 
    }
    // check what happened
    if ( f == -1 ) {
      perror("select() failed");
      return -1;
    }
    if ( f == 0 && list.count > 1 ) { // no client did nothing
      cerr << "\nA Timeout of select() occured." << endl;
      // CLOSE ALL SOCKETS HERE!!!
      cerr << "Removing all sockets." << endl;
      clear_list(list);
      cerr << "List.count: " << list.count << "\n" << endl;
    }
    //~ cerr << "select: something has happened." << endl;
		  
    if (FD_ISSET(sock, &fds)) { // if the server got a connect request ...
      cerr << "\nConnect request\n" << endl;
      struct sockaddr_in client_addr;   
      c = ::accept(sock, (struct sockaddr *)&client_addr, &sin_size);
      if ( c == -1 ) {
	perror("accept() failed");
	return -1;
      }
			  
      cerr << "\n I got a connection from " << inet_ntoa(client_addr.sin_addr) << " , " << ntohs(client_addr.sin_port) << '\n';;
      cerr << "Deskriptor: " << c << "\n\n";
			  
      add_client(&list, c);
    }
    else if ( f == 0 ) { // a timeout occured, so don't try to find a socket
    }
    else { // else: find a sending socket in the list
      cerr << "find socket in list" << endl;
      sender = get_sender(&fds);
      cerr << "Sender is: " << sender << endl;
      int out = readCommand( sender );
      if (out == 0 ) // if the sender sends 0 bytes or sending failed, remove it from list
	remove_client(&list, sender);
    }
  }
	
  close(sock);
  return 0;
}
*/
