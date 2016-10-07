/*
  daqflexthread.cc
  DataThread implementation for acquisition of data using DAQFlex

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

#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <QMutexLocker>
#include "daqflexthread.h"


DAQFlexThread::DAQFlexThread( ConfigData *cd )
  : DataThread( "Acquisition", cd )
{
  IsPrepared = false;
  IsRunning = false;
  DAQFlexDevice = NULL;
  Traces = 0;
  ReadBufferSize = 0;
  BufferSize = 0;
  BufferN = 0;
  Buffer = NULL;
  TraceIndex = 0;
  TotalSamples = 0;
  CurrentSamples = 0;

  for ( int j=0; j<MaxDevices; j++ ) {
    addText( "device" + Str( j+1 ), "/dev/comedi" + Str( j ) );
    addText( "blacklist" + Str( j+1 ), "" );
  }
  addSelection( "reference", "RSE|DIFF|RSE|NRSE" );
}


int DAQFlexThread::initialize( double duration )
{
  // open DAQFlexCore:
  //  DAQFlexDevice = 0; // XXX

  if ( !DAQFlexDevice->isOpen() ) {
    setErrorStr( "Daqflex core device " + DAQFlexDevice->deviceName() + " is not open." );
    return NotOpen;
  }


  // initialize ranges:
  BipolarRange.clear();
  BipolarRangeCmds.clear();
  double checkrange[17] = { 20.0, 10.0, 5.0, 4.0, 2.5, 2.5, 2.0, 1.25, 1.25, 1.0, 0.625, 0.3125, 0.15625, 0.14625, 0.078125, 0.073125, -1.0 };
  string checkstr[16] = { "BIP20V", "BIP10V", "BIP5V", "BIP4V", "BIP2PT5V", "BIP2.5V", "BIP2V", "BIP1PT25V", "BIP1.25V", "BIP1V", "BIP625.0E-3V", "BIP312.5E-3V", "BIP156.25E-3V", "BIP146.25E-3V", "BIP78.125E-3V", "BIP73.125E-3V" };
  for ( int i = 0; i < 20 && checkrange[i] > 0.0; i++ ) {
    string message = "AI{0}:RANGE=" + checkstr[i];
    DAQFlexDevice->sendMessage( message );
    if ( DAQFlexDevice->success() ) {
      string response = DAQFlexDevice->sendMessage( "?AI{0}:RANGE" );
      if ( DAQFlexDevice->success() && response == message ) {
	BipolarRange.push_back( checkrange[i] );
	BipolarRangeCmds.push_back( checkstr[i] );
      }
    }
  }
  if ( BipolarRange.size() == 0 ) {
    if ( DAQFlexDevice->error() == DAQFlexCore::ErrorLibUSBIO ) {
      setErrorStr( "Error in initializing DAQFlexAnalogInput device: no input ranges found. Error: " +
		   DAQFlexDevice->daqflexErrorStr() + ". Check the USB cable/connection!" );
      return ReadError;
    }
    // retrieve single supported range:
    Str response = DAQFlexDevice->sendMessage( "?AI{0}:RANGE" );
    if ( DAQFlexDevice->success() && response.size() > 16 ) {
      bool uni = ( response[12] == 'U' );
      double range = response.number( 0.0, 15 );
      if ( range <= 1e-6 || uni ) {
	setErrorStr( "Failed to read out analog input range from device " + DAQFlexDevice->deviceName() );
	return InvalidDevice;
      }
      BipolarRange.push_back( range );
      BipolarRangeCmds.push_back( response.right( 12 ) );
    }
    else {
      setErrorStr( "Failed to retrieve analog input range from device " + DAQFlexDevice->deviceName() +
		   ". Error: " + DAQFlexDevice->daqflexErrorStr() );
      return InvalidDevice;
    }
  }

  // clear flags:
  IsPrepared = false;
  IsRunning = false;
  TotalSamples = 0;
  CurrentSamples = 0;
  ReadBufferSize = 2 * DAQFlexDevice->aiFIFOSize();





  // prepare:

  if ( !isOpen() ) {
    traces.setError( DaqError::DeviceNotOpen );
    return -1;
  }

  QMutexLocker locker( mutex() );

  Settings.clear();
  IsPrepared = false;
  Traces = 0;
  TraceIndex = 0;

  // init internal buffer:
  if ( Buffer != 0 )
    delete [] Buffer;
  // 2 times the updatetime ...
  BufferSize = 2 * traces.size() * traces[0].indices( traces[0].updateTime() ) * 2;
  // ... as a multiple of the packet size:
  int inps = DAQFlexDevice->inPacketSize();
  BufferSize = (BufferSize/inps+1)*inps;
  Buffer = new char[BufferSize];
  BufferN = 0;

  // setup acquisition:
  DAQFlexDevice->sendMessage( "AISCAN:XFRMODE=BLOCKIO" );
  DAQFlexDevice->sendMessage( "AISCAN:RATE=" + Str( traces[0].sampleRate(), "%g" ) );
  DAQFlexDevice->setAISampleRate( traces[0].sampleRate() );
  if ( traces[0].continuous() ) {
    DAQFlexDevice->sendMessage( "AISCAN:SAMPLES=0" );
    TotalSamples = 0;
  }
  else {
    DAQFlexDevice->sendMessage( "AISCAN:SAMPLES=" + Str( traces[0].size() ) );
    TotalSamples = traces[0].size() * traces.size();
  }
  CurrentSamples = 0;

  // setup channels:
  DAQFlexDevice->sendMessage( "AISCAN:QUEUE=ENABLE" );
  DAQFlexDevice->sendMessage( "AIQUEUE:CLEAR" );
  for( int k = 0; k < traces.size(); k++ ) {
    // DAQFlexDevice->sendMessage( "?AIQUEUE:COUNT" ); USE THIS AS QUEUE Element

    // delay:
    if ( traces[k].delay() > 0.0 ) {
      traces[k].addError( DaqError::InvalidDelay );
      traces[k].addErrorStr( "delays are not supported by DAQFlex analog input!" );
      traces[k].setDelay( 0.0 );
    }

    // XXX 7202, 7204 do not have AIQUEUE! channels need to be in a sequence!
    string aiq = "AIQUEUE{" + Str( k ) + "}:";

    // channel:
    DAQFlexDevice->sendMessage( aiq + "CHAN=" + Str( traces[k].channel() ) );

    // reference:
    // XXX 20X: Has only SE CHMODE! Cannot be set.
    // XXX 7202, 1608FS: Has no CHMODE
    // XXX 7204: Has only AI:CHMODE
    // XXX 1208-FS, 1408FS do not have CHMODE FOR AIQUEUE! But AI:CHMODE
    switch ( traces[k].reference() ) {
    case InData::RefCommon:
      DAQFlexDevice->sendMessage( aiq + "CHMODE=SE" );
      break;
    case InData::RefDifferential:
      DAQFlexDevice->sendMessage( aiq + "CHMODE=DIFF" );
      break;
    case InData::RefGround:
      DAQFlexDevice->sendMessage( aiq + "CHMODE=SE" );
      break;
    default:
      traces[k].addError( DaqError::InvalidReference );
    }

    // allocate gain factor:
    char *gaindata = traces[k].gainData();
    if ( gaindata != NULL )
      delete [] gaindata;
    gaindata = new char[sizeof(Calibration)];
    traces[k].setGainData( gaindata );
    Calibration *gainp = (Calibration *)gaindata;

    // ranges:
    if ( traces[k].unipolar() ) {
      traces[k].addError( DaqError::InvalidGain );
    }
    else {
      double max = BipolarRange[traces[k].gainIndex()];
      if ( max < 0 )
	traces[k].addError( DaqError::InvalidGain );
      else {
	traces[k].setMaxVoltage( max );
	traces[k].setMinVoltage( -max );
	if ( BipolarRange.size() > 1 ) {
	  string message = aiq + "RANGE=" + BipolarRangeCmds[traces[k].gainIndex()];
	  string response = DAQFlexDevice->sendMessage( message );
	  if ( DAQFlexDevice->failed() || response.empty() )
	    traces[k].addError( DaqError::InvalidGain );
	}
	if ( traces[k].success() ) {
	  // get calibration:
	  string response = DAQFlexDevice->sendMessage( "?AI{" + Str( traces[k].channel() ) + "}:SLOPE" );
	  gainp->Slope = Str( response.erase( 0, 12 ) ).number();
	  response = DAQFlexDevice->sendMessage( "?AI{" + Str( traces[k].channel() ) + "}:OFFSET" );
	  gainp->Offset = Str( response.erase( 0, 13 ) ).number();
	  gainp->Slope *= 2.0*max/DAQFlexDevice->maxAIData();
	  gainp->Offset *= 2.0*max/DAQFlexDevice->maxAIData();
	  gainp->Offset -= max;
	}
      }
    }
  }

  if ( traces.failed() )
    return -1;

  if ( traces.success() ) {
    traces.setReadTime( traces[0].interval( ReadBufferSize/2/traces.size() ) );
    traces.setUpdateTime( traces[0].interval( BufferSize/2/traces.size() ) );
    setSettings( traces, BufferSize, ReadBufferSize );
    Traces = &traces;
    IsPrepared = true;
    return 0;
  }
  else
    return -1;


  // start read:
  QMutexLocker locker( mutex() );
  if ( !IsPrepared || Traces == 0 ) {
    cerr << "AI not prepared or no traces!\n";
    return -1;
  }

  bool tookao = ( TakeAO && aosp != 0 && DAQFlexAO != 0 && DAQFlexAO->prepared() );

  if ( tookao ) {
    if ( DAQFlexAO->useAIRate() ) {
      DAQFlexDevice->sendCommand( "AOSCAN:START" );
      DAQFlexDevice->sendCommand( "AISCAN:START" );
    }
    else
      DAQFlexDevice->sendCommands( "AISCAN:START", "AOSCAN:START" );
  }
  else
    DAQFlexDevice->sendCommand( "AISCAN:START" );

  bool finished = true;
  TraceIndex = 0;
  IsRunning = true;
  startThread( sp, datamutex, datawait );
  if ( tookao ) {
    DAQFlexAO->startThread( aosp );
    finished = DAQFlexAO->noMoreData();
  }
  return finished ? 0 : 1;











  printlog( "Number of channels needed: " + Str( channels() ) );
  int nchan = channels();

  // reference:
  int ref = index( "reference" );
  printlog( "DAQFlex reference: " + Str( ref ) + " (0=DIFF, 1=RSE, 2=NRSE)" );

  NDevices = 0;
  int grid = 0;
  while ( grid < maxGrids() && ! used(grid) )
    grid++;
  int gridchannels = 0;

  unsigned int masterscanbeginarg = 0;

  for ( int j=0; j<MaxDevices; j++ ) {
    // open comedi device:
    string devicefile = text( "device" + Str( j+1 ) );
    if ( devicefile.empty() )
      continue;
    DeviceP[NDevices] = comedi_open( devicefile.c_str() );
    if ( DeviceP[NDevices] == NULL ) {
      if ( NDevices == 0 )
	printlog( "! error: DAQFlexThread::initialize() -> Device-file "
		  + text( "device" + Str( j+1 ) ) + " could not be opened!" );
      else
	printlog( "DAQFlexThread::initialize() -> Device-file "
		  + text( "device" + Str( j+1 ) ) + " could not be opened!" );
      continue;
    }
    
    // get AI subdevice:
    int subdev = comedi_find_subdevice_by_type( DeviceP[NDevices], COMEDI_SUBD_AI, 0 );
    if ( subdev < 0 ) {
      printlog( "! error: DAQFlexThread::initialize() -> No subdevice for AI found on device "
		+ devicefile );
      comedi_close( DeviceP[NDevices] );
      DeviceP[NDevices] = NULL;
      continue;
    }
    SubDevice[NDevices] = subdev;

    // lock AI subdevice:
    if ( comedi_lock( DeviceP[NDevices], SubDevice[NDevices] ) != 0 ) {
      printlog( "! error: DAQFlexThread::initialize() -> Locking of AI subdevice failed on device "
		+ devicefile );
      comedi_close( DeviceP[NDevices] );
      DeviceP[NDevices] = NULL;
      SubDevice[NDevices] = 0;
      continue;
    }  

    // check for async. command support:
    if ( ( comedi_get_subdevice_flags( DeviceP[NDevices], SubDevice[NDevices] ) & SDF_CMD_READ ) == 0 ) {
      printlog( "! error: DAQFlexThread::initialize() -> Device "
		+ devicefile + " not supported! SubDevice needs to support async. commands!" );
      comedi_unlock( DeviceP[NDevices],  SubDevice[NDevices] );
      comedi_close( DeviceP[NDevices] );
      DeviceP[NDevices] = NULL;
      SubDevice[NDevices] = 0;
      continue;
    }

    printlog( "Opened comedi device: " + devicefile );


    // set size of comedi-internal buffer to maximum:
    int readbuffersize = comedi_get_max_buffer_size( DeviceP[NDevices], SubDevice[NDevices] );
    comedi_set_buffer_size( DeviceP[NDevices], SubDevice[NDevices], readbuffersize );
    comedi_get_buffer_size( DeviceP[NDevices], SubDevice[NDevices] );



    // setup command:
    comedi_cmd cmd;

    // channels:
    unsigned int *chanlist = new unsigned int[512];
    memset( chanlist, 0, sizeof( chanlist ) );
    memset( &cmd, 0, sizeof( comedi_cmd ) );

    // reference:
    int aref = -1;
    int subdeviceflags = comedi_get_subdevice_flags( DeviceP[NDevices], SubDevice[NDevices] );
    switch ( ref ) {
    case 0:
      if ( subdeviceflags & SDF_DIFF )
	aref = AREF_DIFF; 
      break;
    case 1:
      if ( subdeviceflags & SDF_GROUND )
	aref = AREF_GROUND; 
      break;
    case 2: 
      if ( subdeviceflags & SDF_COMMON )
	aref = AREF_COMMON;
      break;
    }
    if ( aref == -1 ) {
      printlog( "! error: DAQFlexThread::initialize() -> invalid reference" );
      comedi_close( DeviceP[NDevices] );
      DeviceP[NDevices] = NULL;
      SubDevice[NDevices] = 0;
      continue;
    }

    // maximum number channels:
    int maxchannels = comedi_get_n_channels( DeviceP[NDevices], SubDevice[NDevices] );
    NChannels[NDevices] = 0;
    Str blacklist = text( "blacklist" + Str( j+1 ) );
    vector<int> blackchannels;
    blacklist.range( blackchannels, ",", "-" );

    bool softcal = ( ( comedi_get_subdevice_flags( DeviceP[NDevices], SubDevice[NDevices] ) &
		       SDF_SOFT_CALIBRATED ) > 0 );
    Calib[NDevices] = new comedi_polynomial_t[maxchannels];

    for( int c = 0; c < maxchannels && nchan > 0; c++ ) {
      bool black = false;
      for ( unsigned int i=0; i<blackchannels.size(); i++ ) {
	if ( c == blackchannels[i] ) {
	  black = true;
	  break;
	}
      }
      if ( black )
	continue;
      int range = comedi_find_range( DeviceP[NDevices], SubDevice[NDevices],
				     c, UNIT_volt, -gain()*maxVolts(), gain()*maxVolts() );
      if ( range < 0 ) {
	printlog( "! error in DAQFlexThread::initialize() -> comedi_find_range: no range for "
		  + Str( gain()*maxVolts() ) + " V found." );
	comedi_close( DeviceP[NDevices] );
	DeviceP[NDevices] = NULL;
	SubDevice[NDevices] = 0;
	continue;
      }

      {
	comedi_range *crange = comedi_get_range( DeviceP[NDevices], SubDevice[NDevices], c, range );
	if ( c == 0 )
	  printlog( "Comedi range: channel=" + Str( c ) + ", range=" + Str( range )
		  + ", min=" + Str( crange->min ) + "V, max=" + Str( crange->max ) + "V" );
      }
      chanlist[NChannels[NDevices]] = CR_PACK( c, range, aref );
      if ( softcal && Calibration[NDevices] != 0 )
	comedi_get_softcal_converter( SubDevice[NDevices], c, range, COMEDI_TO_PHYSICAL,
				      Calibration[NDevices], &Calib[NDevices][NChannels[NDevices]] );
      else
	comedi_get_hardcal_converter( DeviceP[NDevices], SubDevice[NDevices], c, range,
				      COMEDI_TO_PHYSICAL, &Calib[NDevices][NChannels[NDevices]] );
      GridChannel[NDevices][NChannels[NDevices]] = grid;
      NChannels[NDevices]++;
      nchan--;
      gridchannels++;
      if ( gridchannels >= gridChannels( grid ) ) {
	do {
	  grid++;
	} while ( grid < maxGrids() && ! used(grid) );
	gridchannels = 0;
	if ( grid >= maxGrids() )
	  break;
      }
    }
    if ( NChannels[NDevices] <= 0 ) {
      printlog( "! error in DAQFlexThread::initialize() -> no channels to be configured" );
      comedi_close( DeviceP[NDevices] );
      DeviceP[NDevices] = NULL;
      SubDevice[NDevices] = 0;
      delete [] Calib[NDevices];
      Calib[NDevices] = 0;
      continue;
    }
 
    // try automatic command generation:
    cmd.scan_begin_src = TRIG_TIMER;
    cmd.flags = TRIG_ROUND_NEAREST;  // alternatives: TRIG_ROUND_DOWN, TRIG_ROUND_UP
    unsigned int period = (int)::rint( 1.0e9 / sampleRate() );  
    int retVal = comedi_get_cmd_generic_timed( DeviceP[NDevices], SubDevice[NDevices],
					       &cmd, NChannels[NDevices], period );
    if ( retVal < 0 ) {
      printlog( "! error in DAQFlexThread::initialize() -> comedi_get_cmd_generic_timed failed: "
		+ Str( comedi_strerror( comedi_errno() ) ) + ")" );
      comedi_close( DeviceP[NDevices] );
      DeviceP[NDevices] = NULL;
      SubDevice[NDevices] = 0;
      delete [] Calib[NDevices];
      Calib[NDevices] = 0;
      continue;
    }
    if ( cmd.scan_begin_src != TRIG_TIMER ) {
      printlog( "! error in DAQFlexThread::initialize() -> acquisition timed by a daq-board counter not possible" );
      comedi_close( DeviceP[NDevices] );
      DeviceP[NDevices] = NULL;
      SubDevice[NDevices] = 0;
      delete [] Calib[NDevices];
      Calib[NDevices] = 0;
      continue;
    }
    if ( masterscanbeginarg > 0 )
      cmd.scan_begin_arg = masterscanbeginarg;
    else
      cmd.scan_begin_arg = period;

    // adapt command to our purpose:
    comedi_cmd testCmd;
    comedi_get_cmd_src_mask( DeviceP[NDevices], SubDevice[NDevices], &testCmd );
#ifdef SYNCAIMASTER
    if ( NDevices == 0 ) {
      cerr << "NDevices=0\n";
      // master:
      if ( testCmd.start_src & TRIG_INT ) {
	cmd.start_src = TRIG_INT;
	cmd.start_arg = 0;
	masterdevice = DeviceP[NDevices];
	cerr << "setting masterdevice to " << (void*)masterdevice << "\n";
	mastersubdev = SubDevice[NDevices];

	bool failed = false;
        static const unsigned rtsi_subdev = 10;
        static const unsigned rtsi_clock_line = 7;

	// setting up an comedi instruction for routing RTSI signals:
        // Route RTSI clock to line 7:
	// (not needed on pre-m-series boards since their clock is always on line 7)
        comedi_insn configCmd;
        memset( &configCmd, 0, sizeof(configCmd) );
        lsampl_t configData[2];
        memset( &configData, 0, sizeof(configData) );
        configCmd.insn = INSN_CONFIG;
        configCmd.subdev = rtsi_subdev;
        configCmd.chanspec = rtsi_clock_line;
        configCmd.n = 2;
        configCmd.data = configData;
        configCmd.data[0] = INSN_CONFIG_SET_ROUTING;
        configCmd.data[1] = NI_RTSI_OUTPUT_RTSI_OSC;

#ifdef SYNCAICLOCK
	// do route the clock:
        retVal = comedi_do_insn( masterdevice, &configCmd );
        if ( retVal < 0 ) {
	  comedi_perror( "comedi_do_insn: INSN_CONFIG SET_ROUTING" );
	  failed = true;
        }
        // Set clock RTSI line as output
        retVal = comedi_dio_config( masterdevice, rtsi_subdev,
				    rtsi_clock_line, INSN_CONFIG_DIO_OUTPUT );
        if ( retVal < 0 ) {
	  comedi_perror( "comedi_dio_config for setting clock RTSI line as output");
	  failed = true;
        }
#endif

        // Set routing of the 3 main AI RTSI signals and their direction to output.
	// We're reusing the already initialized configCmd instruction here since
	// it's mostly the same.
#ifdef SYNCAISTART
        configCmd.chanspec = 0;
        configCmd.data[1] = NI_RTSI_OUTPUT_ADR_START1;
        retVal = comedi_do_insn( masterdevice, &configCmd );
        if ( retVal < 0 ) {
	  comedi_perror( "comedi_do_insn: INSN_CONFIG set RTSI_OUTPUT_ADR_START1" );
	  failed = true;
        }
        retVal = comedi_dio_config( masterdevice, rtsi_subdev, 0,
				    INSN_CONFIG_DIO_OUTPUT );
        if ( retVal < 0 ) {
	  comedi_perror( "comedi_dio_config RTSI_OUTPUT_ADR_START1 for output");
	  failed = true;
        }
#endif


#ifdef SYNCAICONVERT
        configCmd.chanspec = 2;
        configCmd.data[1] =  NI_RTSI_OUTPUT_SCLKG;
        retVal = comedi_do_insn( masterdevice, &configCmd );
        if ( retVal < 0 ) {
	  comedi_perror("comedi_do_insn: INSN_CONFIG RTSI_OUTPUT_SCLKG");
	  failed = true;
        }
        retVal = comedi_dio_config( masterdevice, rtsi_subdev, 2,
				    INSN_CONFIG_DIO_OUTPUT );
        if ( retVal < 0 ) {
	  comedi_perror("comedi_dio_config RTSI_OUTPUT_SCLKG for output");
	  failed = true;
        }
#endif

	if ( failed ) {
	  printlog( "! error in DAQFlexThread::initialize() -> failed to set up master device" );
	  comedi_close( DeviceP[NDevices] );
	  DeviceP[NDevices] = NULL;
	  SubDevice[NDevices] = 0;
	  delete [] Calib[NDevices];
	  Calib[NDevices] = 0;
	  continue;
	}

      }
      else {
	printlog( "! error in DAQFlexThread::initialize() -> trigger INT not supported" );
	comedi_close( DeviceP[NDevices] );
	DeviceP[NDevices] = NULL;
	SubDevice[NDevices] = 0;
	delete [] Calib[NDevices];
	Calib[NDevices] = 0;
	continue;
      }
    }
    else {
      // slave:
      if ( testCmd.start_src & TRIG_EXT ) {
	bool failed = false;
	//
        static const unsigned rtsi_subdev = 10;
#ifdef SYNCAICLOCK
        static const unsigned rtsi_clock_line = 7;
        comedi_insn configCmd;
        memset( &configCmd, 0, sizeof(configCmd) );
        lsampl_t configData[3];
        memset( &configData, 0, sizeof(configData) );
        configCmd.insn = INSN_CONFIG;
        configCmd.subdev = rtsi_subdev;
        configCmd.chanspec = 0;
        configCmd.n = 3;
        configCmd.data = configData;
        configCmd.data[0] = INSN_CONFIG_SET_CLOCK_SRC;
        configCmd.data[1] = NI_MIO_PLL_RTSI_CLOCK( rtsi_clock_line );
        configCmd.data[2] = 100;        // need to give it correct external clock period
        retVal = comedi_do_insn( DeviceP[NDevices], &configCmd );
        if ( retVal < 0 ) {
	  comedi_perror( "comedi_do_insn: INSN_CONFIG SET_CLOCK_SOURCE" );
	  failed = true;
        }
        // configure RTSI clock line as input:
        retVal = comedi_dio_config( DeviceP[NDevices], rtsi_subdev,
				    rtsi_clock_line, INSN_CONFIG_DIO_INPUT );
        if ( retVal < 0 ) {
	  comedi_perror( "comedi_dio_config CLOCK_SOURCE for input" );
	  failed = true;
        }
#endif
        // Configure RTSI lines we are using for AI signals as inputs:
#ifdef SYNCAISTART
        retVal = comedi_dio_config( DeviceP[NDevices], rtsi_subdev, 0,
				    INSN_CONFIG_DIO_INPUT );
        if ( retVal < 0 ) {
	  comedi_perror( "comedi_dio_config RTSI chan 0 for input" );
	  failed = true;
        }
	else {
	  cmd.start_src = TRIG_EXT;
	  cmd.start_arg = CR_EDGE | NI_EXT_RTSI(0);
	}
#endif
	/* not used ...
        retVal = comedi_dio_config( DeviceP[NDevices], rtsi_subdev, 1,
				    INSN_CONFIG_DIO_INPUT );
        if ( retVal < 0 ) {
	  comedi_perror( "comedi_dio_config RTSI chan 1 for input" );
	  failed = true;
        }
	*/
#ifdef SYNCAICONVERT
        retVal = comedi_dio_config( DeviceP[NDevices], rtsi_subdev, 2,
				    INSN_CONFIG_DIO_INPUT );
        if ( retVal < 0 ) {
	  comedi_perror( "comedi_dio_config RTSI chan 2 for input" );
	  failed = true;
        }
	else {
	  cmd.convert_src = TRIG_EXT;
	  cmd.convert_arg = CR_INVERT | CR_EDGE | NI_EXT_RTSI(2);
	}
#endif
	if ( failed ) {
	  printlog( "! error in DAQFlexThread::initialize() -> failed to set up slave device" );
	  comedi_close( DeviceP[NDevices] );
	  DeviceP[NDevices] = NULL;
	  SubDevice[NDevices] = 0;
	  delete [] Calib[NDevices];
	  Calib[NDevices] = 0;
	  continue;
	}

      }
      else {
	printlog( "! error in DAQFlexThread::initialize() -> trigger EXT not supported" );
	comedi_close( DeviceP[NDevices] );
	DeviceP[NDevices] = NULL;
	SubDevice[NDevices] = 0;
	delete [] Calib[NDevices];
	Calib[NDevices] = 0;
	continue;
      }
    }
#else
    // no master/slave:
    if ( testCmd.start_src & TRIG_NOW )
      cmd.start_src = TRIG_NOW;
    else {
      printlog( "! error in DAQFlexThread::initialize() -> trigger NOW not supported" );
      comedi_close( DeviceP[NDevices] );
      DeviceP[NDevices] = NULL;
      SubDevice[NDevices] = 0;
      delete [] Calib[NDevices];
      Calib[NDevices] = 0;
      continue;
    }
    cerr << "setup TRIG_NOW\n";
    cmd.start_arg = 0;
#endif
    cmd.scan_end_arg = NChannels[NDevices];

    Samples[NDevices] = 0;
    if ( duration > 0.0 ) {
      cmd.stop_src = TRIG_COUNT;
      cmd.stop_arg = (int)ceil( duration*sampleRate() );
      MaxSamples[NDevices] = cmd.stop_arg*NChannels[NDevices];
    }
    else {
      // test if countinous-state is supported
      if ( !(testCmd.stop_src & TRIG_NONE) ) {
	printlog( "! error in DAQFlexThread::initialize() -> continuous mode not supported!" );
	comedi_close( DeviceP[NDevices] );
	DeviceP[NDevices] = NULL;
	SubDevice[NDevices] = 0;
	delete [] Calib[NDevices];
	Calib[NDevices] = 0;
	continue;
      }
      // set countinous-state:
      MaxSamples[NDevices] = 0;
      cmd.stop_src = TRIG_NONE;
      cmd.stop_arg = 0;
    }

    cmd.chanlist = chanlist;
    cmd.chanlist_len = NChannels[NDevices];

    cmd.data = 0;
    cmd.data_len = 0;
  
    // test command:
    clearError();

    //    cerr << "REQUEST SCAN_BEGIN_ARG=" << cmd.scan_begin_arg << '\n';
    memcpy( &testCmd, &cmd, sizeof( comedi_cmd ) ); // store original state
    for ( int k=0; k<=5; k++ ) {
      retVal = comedi_command_test( DeviceP[NDevices], &cmd );
      if ( retVal == 0 )
	break;
      switch ( retVal ) {
      case 1: // unsupported trigger in *_src:
	printlog( "! error in DAQFlexThread::initialize() -> unsupported trigger" );
	if ( cmd.start_src != testCmd.start_src )
	  printlog( "! error in DAQFlexThread::initialize() -> unsupported trigger in start_src" );
	if ( cmd.scan_begin_src != testCmd.scan_begin_src )
	  printlog( "! error in DAQFlexThread::initialize() -> unsupported trigger in scan_begin_src" );
	if ( cmd.convert_src != testCmd.convert_src )
	  printlog( "! error in DAQFlexThread::initialize() -> unsupported trigger in convert_src" );
	if ( cmd.scan_end_src != testCmd.scan_end_src )
	  printlog( "! error in DAQFlexThread::initialize() -> unsupported trigger in scan_end_src" );
	if ( cmd.stop_src != testCmd.stop_src )
	  printlog( "! error in DAQFlexThread::initialize() -> unsupported trigger in stop_src" );
	break;
      case 2: // invalid trigger in *_src:
	printlog( "! error in DAQFlexThread::initialize() -> invalid trigger" );
	if ( cmd.start_src != testCmd.start_src )
	  printlog( "! error in DAQFlexThread::initialize() -> invalid trigger in start_src" );
	if ( cmd.scan_begin_src != testCmd.scan_begin_src )
	  printlog( "! error in DAQFlexThread::initialize() -> invalid trigger in scan_begin_src" );
	if ( cmd.convert_src != testCmd.convert_src )
	  printlog( "! error in DAQFlexThread::initialize() -> invalid trigger in convert_src" );
	if ( cmd.scan_end_src != testCmd.scan_end_src )
	  printlog( "! error in DAQFlexThread::initialize() -> invalid trigger in scan_end_src" );
	if ( cmd.stop_src != testCmd.stop_src )
	  printlog( "! error in DAQFlexThread::initialize() -> invalid trigger in stop_src" );
	break;
      case 3: // *_arg out of range:
	printlog( "! error in DAQFlexThread::initialize() -> _arg out of range:" );
	if ( cmd.start_arg != testCmd.start_arg )
	  printlog( "! error in DAQFlexThread::initialize() -> start_arg out of range" );
	if ( cmd.scan_begin_arg != testCmd.scan_begin_arg ) {
	  printlog( "! warning in DAQFlexThread::initialize() -> requested sampling period of "
		    + Str( testCmd.scan_begin_arg ) + "ns smaller than supported! min "
		    + Str( cmd.scan_begin_arg ) + "ns sampling interval possible." );
	  // XXX	setSampleRate( 1.0e9 / cmd.scan_begin_arg );
	}
	if ( cmd.convert_arg != testCmd.convert_arg )
	  printlog( "! error in DAQFlexThread::initialize() -> convert_arg out of range" );
	if ( cmd.scan_end_arg != testCmd.scan_end_arg )
	  printlog( "! error in DAQFlexThread::initialize() -> scan_end_arg out of range" );
	if ( cmd.stop_arg != testCmd.stop_arg )
	  printlog( "! error in DAQFlexThread::initialize() -> stop_arg out of range" );
	break;
      case 4: // adjusted *_arg:
	printlog( "! warning in DAQFlexThread::initialize() -> _arg adjusted" );
	if ( cmd.start_arg != testCmd.start_arg )
	  printlog( "! error in DAQFlexThread::initialize() -> start_arg adjusted" );
	if ( cmd.scan_begin_arg != testCmd.scan_begin_arg ) {
	  printlog( "! warning in DAQFlexThread::initialize() -> scan_begin_arg adjusted" );
	  clearError();
	  // XXX setSampleRate( 1.0e9 / cmd.scan_begin_arg );
	}
	if ( cmd.convert_arg != testCmd.convert_arg )
	  printlog( "! error in DAQFlexThread::initialize() -> convert_arg adjusted" );
	if ( cmd.scan_end_arg != testCmd.scan_end_arg )
	  printlog( "! error in DAQFlexThread::initialize() -> scan_end_arg adjusted" );
	if ( cmd.stop_arg != testCmd.stop_arg )
	  printlog( "! error in DAQFlexThread::initialize() -> stop_arg adjusted" );
	break;
      case 5: // invalid chanlist:
	printlog( "! error in DAQFlexThread::initialize() -> invalid chanlist" );
	break;
      }
    }

    if ( error() ) {
      printlog( "! error in DAQFlexThread::initialize() -> setting up command failed!" );
      comedi_close( DeviceP[NDevices] );
      DeviceP[NDevices] = NULL;
      SubDevice[NDevices] = 0;
      delete [] Calib[NDevices];
      Calib[NDevices] = 0;
      continue;
    }

    if ( masterscanbeginarg > 0 && cmd.scan_begin_arg != masterscanbeginarg ) {
      printlog( "! error in DAQFlexThread::initialize() -> sampling rates not identical!" );
      printlog( "! Try slightly differnt sampling rates!" );
      comedi_close( DeviceP[NDevices] );
      DeviceP[NDevices] = NULL;
      SubDevice[NDevices] = 0;
      delete [] Calib[NDevices];
      Calib[NDevices] = 0;
      break;
    }

    printlog( "Comedi sampling rate: " + Str( sampleRate() ) + "Hz" );

    // apply calibration:
    if ( Calibration[NDevices] != 0 ) {
      for( int k=0; k < NChannels[NDevices]; k++ ) {
	unsigned int channel = CR_CHAN( cmd.chanlist[k] );
	unsigned int range = CR_RANGE( cmd.chanlist[k] );
	unsigned int aref = CR_AREF( cmd.chanlist[k] );
	if ( comedi_apply_parsed_calibration( DeviceP[NDevices], SubDevice[NDevices], channel,
					      range, aref, Calibration[NDevices] ) < 0 )
	  printlog( "! warning in DAQFlexThread::initialize() -> apply calibration for channel "
		    + Str( channel ) + " failed!: " + comedi_strerror( comedi_errno() ) );
      }
    }

    // init internal buffer:
    BufferSize[NDevices] = NChannels[NDevices] * (int)::ceil( 0.05 * sampleRate() )
      * BufferElemSize[NDevices];
    Buffer[NDevices] = new char[BufferSize[NDevices]];
    NBuffer[NDevices] = 0;

    // execute command:
    if ( masterscanbeginarg == 0 )
      masterscanbeginarg = cmd.scan_begin_arg;
    cerr << "execute command for device " << NDevices << ":\n";
    dump_cmd( &cmd );
    if ( comedi_command( DeviceP[NDevices], &cmd ) < 0 ) {
      printlog( "! error in DAQFlexThread::initialize() -> " + devicefile
		+ " - execution of comedi_cmd failed: " + comedi_strerror( comedi_errno() ) );
      comedi_close( DeviceP[NDevices] );
      DeviceP[NDevices] = NULL;
      SubDevice[NDevices] = 0;
      delete [] Calib[NDevices];
      Calib[NDevices] = 0;
      delete [] Buffer[NDevices];
      BufferSize[NDevices] = 0;
      NBuffer[NDevices] = 0;
      continue;
    }

    NDevices++;
  }

  if ( nchan > 0 )
    printlog( "! error in DAQFlexThread::initialize() -> cannot record from all " + Str( channels() ) + " channels." );

#ifdef SYNCAIMASTER
  // start masterdevice:
  if ( masterdevice == 0 )
    printlog( "! error in DAQFlexThread::initialize() -> no master device set." );
  comedi_insn masterstart;
  lsampl_t insndata[1];
  insndata[0] = 0;
  masterstart.insn = INSN_INTTRIG;
  masterstart.subdev = mastersubdev;
  masterstart.data = insndata;
  masterstart.n = 1;
  cerr << "executing masterdevice " << (void*)masterdevice << "\n";
  if ( comedi_do_insn( masterdevice, &masterstart ) < 0 ) {
    printlog( "! error in DAQFlexThread::initialize() -> cannot execute instruction for internal trigger to start master device." );
    comedi_perror( "reason is" );
  }
#endif

  DeviceInx = 0;
  ChannelInx = 0;

  cerr << "NDevices = " << NDevices << '\n';

  return NDevices > 0 ? 0 : -1;
}


void DAQFlexThread::finish( void )
{
  for ( int j=0; j<NDevices; j++ ) {

    if ( !isOpen() )
      return NotOpen;

    if ( ! IsRunning )
      return 0;

    lock();
    DAQFlexDevice->sendCommand( "AISCAN:STOP" );
    DAQFlexDevice->sendMessage( "AISCAN:RESET" );
    unlock();

    stopRead();

    lock();
    IsRunning = false;
    unlock();

    return 0;

    // reset:
    if ( !isOpen() )
      return NotOpen;

    QMutexLocker locker( mutex() );

    if ( IsRunning )
      DAQFlexDevice->sendCommand( "AISCAN:STOP" );

    DAQFlexDevice->sendMessage( "AISCAN:RESET" );

    // clear overrun condition:
    DAQFlexDevice->clearRead();

    // flush:
    int numbytes = 0;
    int status = 0;
    do {
      const int nbuffer = DAQFlexDevice->inPacketSize()*4;
      unsigned char buffer[nbuffer];
      status = DAQFlexDevice->readBulkTransfer( buffer, nbuffer, &numbytes, 200 );
    } while ( numbytes > 0 && status == 0 );

    // free internal buffer:
    if ( Buffer != 0 )
      delete [] Buffer;
    Buffer = NULL;
    BufferSize = 0;
    BufferN = 0;
    TotalSamples = 0;
    CurrentSamples = 0;

    Settings.clear();

    IsPrepared = false;
    IsRunning = false;
    Traces = 0;
    TraceIndex = 0;
  
    return 0;


    // close:
    if ( ! isOpen() )
      return;

    reset();

    // clear flags:
    DAQFlexDevice = NULL;
    IsPrepared = false;
    TraceIndex = 0;
    TotalSamples = 0;
    CurrentSamples = 0;
    TakeAO = true;
    DAQFlexAO = 0;





    // stop acquisition:
    comedi_cancel( DeviceP[j], SubDevice[j] );

    // clear buffers by reading:
    while ( comedi_get_buffer_contents( DeviceP[j], SubDevice[j] ) > 0 ) {
      char buffer[BufferSize[j]];
      ::read( comedi_fileno( DeviceP[j] ), buffer, BufferSize[j] );
    }

    // cleanup calibration:
    if ( Calibration[j] != 0 )
      comedi_cleanup_calibration( Calibration[j] );
    Calibration[j] = 0;

    // unlock:
    comedi_unlock( DeviceP[j],  SubDevice[j] );

    // close:
    comedi_close( DeviceP[j] );

    // clear flags:
    DeviceP[j] = NULL;
    SubDevice[j] = 0;
    if ( Calib[j] != 0 )
      delete [] Calib[j];
    Calib[j] = 0;
    delete [] Buffer[j];
    BufferSize[j] = 0;
    NBuffer[j] = 0;
    Samples[j] = 0;
    MaxSamples[j] = 0;
  }

  // clear grids to keep buffers in shape:
  for ( int g=0; g < maxGrids(); g++ ) {
    if ( used(g) ) {
      inputBuffer(g).resize( (inputBuffer(g).size()/gridChannels(g))
			     * gridChannels(g) );
    }
  }
}


int DAQFlexThread::read( void )
{
  //  cerr << "DAQFlex::readData() start\n";
  QMutexLocker locker( mutex() );

  if ( Traces == 0 || Buffer == 0 || ! IsRunning )
    return -2;

  int readn = 0;
  int buffern = BufferN*2;
  int inps = DAQFlexDevice->inPacketSize();
  int maxn = ((BufferSize - buffern)/inps)*inps;
  if ( maxn > ReadBufferSize )
    maxn = ReadBufferSize;
  if ( maxn <= 0 )
    return 0;

  // read data:
  int timeout = (int)::ceil( 10.0 * 1000.0*(*Traces)[0].interval( maxn/2/Traces->size() ) ); // in ms
  DAQFlexCore::DAQFlexError ern = DAQFlexCore::Success;
  ern = DAQFlexDevice->readBulkTransfer( (unsigned char*)(Buffer + buffern),
					 maxn, &readn, timeout );

  // store data:
  if ( readn > 0 ) {
    buffern += readn;
    BufferN = buffern / 2;
    readn /= 2;
    CurrentSamples += readn;
  }

  if ( ern == DAQFlexCore::Success || ern == DAQFlexCore::ErrorLibUSBTimeout ) {
    string status = DAQFlexDevice->sendMessage( "?AISCAN:STATUS" );
    if ( status != "AISCAN:STATUS=RUNNING" ) {
      if ( status == "AISCAN:STATUS=OVERRUN" ) {
	Traces->addError( DaqError::OverflowUnderrun );
	return -2;
      }
      else {
	cerr << "DAQFlexAnalogInput::readData() -> analog input not running anymore\n";
	// This error occurs on to fast sampling, but at lower sampling it looks like it can be ignored....
	// Traces->addErrorStr( "analog input not running anymore" );
	// Traces->addError( DaqError::Unknown );
      }
      // return -2;
    }
    /*
    // no more data to be read:
    XXX This does not make any sense, since IsRunning is always true!!!
    if ( readn <= 0 && !IsRunning ) {
      if ( IsRunning && ( TotalSamples <=0 || CurrentSamples < TotalSamples ) ) {
	Traces->addErrorStr( deviceFile() + " - buffer-overflow " );
	Traces->addError( DaqError::OverflowUnderrun );
	return -2;
      }
      return -1;
    }
    */
  }
  else {
    // error:
    cerr << "READBULKTRANSFER error=" << DAQFlexDevice->daqflexErrorStr( ern )
	 << " readn=" << readn << '\n';

    switch( ern ) {

    case DAQFlexCore::ErrorLibUSBOverflow:
    case DAQFlexCore::ErrorLibUSBPipe:
      Traces->addError( DaqError::OverflowUnderrun );
      return -2;

    case DAQFlexCore::ErrorLibUSBBusy:
      Traces->addError( DaqError::Busy );
      return -2;

    case DAQFlexCore::ErrorLibUSBNoDevice:
      Traces->addError( DaqError::NoDevice );
      return -2;

    default:
      Traces->addErrorStr( DAQFlexDevice->daqflexErrorStr( ern ) );
      Traces->addError( DaqError::Unknown );
      return -2;
    }
  }

  return readn;


  // convert:
  QMutexLocker locker( mutex() );

  if ( Traces == 0 || Buffer == 0 )
    return -1;

  // conversion factors and scale factors:
  double scale[Traces->size()];
  const Calibration* calib[Traces->size()];
  for ( int k=0; k<Traces->size(); k++ ) {
    scale[k] = (*Traces)[k].scale();
    calib[k] = (const Calibration *)(*Traces)[k].gainData();
  }

  // trace buffer pointers and sizes:
  float *bp[Traces->size()];
  int bm[Traces->size()];
  int bn[Traces->size()];
  for ( int k=0; k<Traces->size(); k++ ) {
    bp[k] = (*Traces)[k].pushBuffer();
    bm[k] = (*Traces)[k].maxPush();
    bn[k] = 0;
  }

  // type cast for device buffer:
  unsigned short *db = (unsigned short *)Buffer;

  for ( int k=0; k<BufferN; k++ ) {
    // convert:
    *bp[TraceIndex] = (float) db[k]*calib[TraceIndex]->Slope + calib[TraceIndex]->Offset;
    *bp[TraceIndex] *= scale[TraceIndex];
    // update pointers:
    bp[TraceIndex]++;
    bn[TraceIndex]++;
    if ( bn[TraceIndex] >= bm[TraceIndex] ) {
      (*Traces)[TraceIndex].push( bn[TraceIndex] );
      bp[TraceIndex] = (*Traces)[TraceIndex].pushBuffer();
      bm[TraceIndex] = (*Traces)[TraceIndex].maxPush();
      bn[TraceIndex] = 0;
    }
    // next trace:
    TraceIndex++;
    if ( TraceIndex >= Traces->size() )
      TraceIndex = 0;
  }

  // commit:
  for ( int c=0; c<Traces->size(); c++ )
    (*Traces)[c].push( bn[c] );

  int n = BufferN;
  BufferN = 0;

  return n;






  lsampl_t *lbuffer[NDevices];
  sampl_t *sbuffer[NDevices];
  int readn[NDevices];
  int bufferinx[NDevices];
  bool ready = true;

  for ( int j=0; j<NDevices; j++ ) {
    // check:
    if ( MaxSamples[j] > 0 && Samples[j] >= MaxSamples[j] )
      continue;
    ready = false;

    // initialize pointers:
    lbuffer[j] = (lsampl_t *)Buffer[j];
    sbuffer[j] = (sampl_t *)Buffer[j];
    readn[j] = 0;
    bufferinx[j] = 0;

    // get data from daq driver:
    //    cerr << "READ " << BufferSize[j]-NBuffer[j] << '\n';
    ssize_t m = ::read( comedi_fileno( DeviceP[j] ),
			&Buffer[j][NBuffer[j]], BufferSize[j]-NBuffer[j] );
    int ern = errno;
    //    cerr << "READ j=" << j << " m=" << m << " ern=" << ern << " NBuffer[j]=" << NBuffer[j] << " bufferinx[j]=" << bufferinx[j] << " readn[j]=" << readn[j] << '\n';
    if ( m < 0 && ern != EAGAIN && ern != EINTR ) {
      printlog( "DAQFlexThread::read(): error on device " + Str( j )
		+ " -> " + Str( errno ) + ": " + Str( strerror( ern ) ) );
      return -1;
    }

    // no more data to be read:
    if ( m <= 0 && ( comedi_get_subdevice_flags( DeviceP[j], SubDevice[j] )
		     & SDF_RUNNING ) == 0 ) {
      printlog( "! error in DAQFlexThread::read(): no data and not running on device " + Str( j ) );
      return -1;
    }

    // number of read samples:
    if ( m > 0 ) {
      NBuffer[j] += m;
      readn[j] = NBuffer[j] / BufferElemSize[j];
      Samples[j] += m / BufferElemSize[j];
      //      cerr << "UPDATE READ DATA j=" << j << " NBuffer[j]=" << NBuffer[j] << " readn[j]=" << readn[j] << " m=" << m << '\n';
    }
  }

  // nothing read anymore:
  if ( ready )
    return 1;

  // transfer data to input buffer:
  bool data = true;
  do {
    int pn[maxGrids()];
    int mp[maxGrids()];
    float *fp[maxGrids()];
    for ( int g=0; g<maxGrids(); g++ ) {
      if ( used( g ) ) {
	pn[g] = 0;
	mp[g] = inputBuffer( g ).maxPush();
	fp[g] = inputBuffer( g ).pushBuffer();
      }
      else
	mp[g] = -1;
    }
    //    cerr << "TRANSFER DeviceInx=" << DeviceInx << " bufferinx[DeviceInx]=" << bufferinx[DeviceInx] << " readn[DeviceInx]=" << readn[DeviceInx] << '\n';
    while ( bufferinx[DeviceInx] < readn[DeviceInx] ) {
      int g = GridChannel[DeviceInx][ChannelInx];
      if ( mp[g] <= 0 )
	break;
      lsampl_t v;
      if ( LongSampleType[DeviceInx] )
	v = lbuffer[DeviceInx][bufferinx[DeviceInx]];
      else
	v = sbuffer[DeviceInx][bufferinx[DeviceInx]];
      *(fp[g]++) = comedi_to_physical( v, &Calib[DeviceInx][ChannelInx] )/gain();
      //      cerr << "convert " << v << " to " << *(fp[g]-1) << '\n';
      bufferinx[DeviceInx]++;
      pn[g]++;
      mp[g]--;
      ChannelInx++;
      if ( ChannelInx >= NChannels[DeviceInx] ) {
	ChannelInx = 0;
	DeviceInx++;
	if ( DeviceInx >= NDevices )
	  DeviceInx = 0;
      }
    }
    for ( int g=0; g<maxGrids(); g++ ) {
      if ( used( g ) ) {
	lockAI( g );
	// cerr << "PUSHED " << pn[g] << " on grid " << g << '\n';
	inputBuffer( g ).push( pn[g] );
	unlockAI( g );
      }
    }
    for ( int k=0; k<NDevices; k++ ) {
      if ( bufferinx[k] >= readn[k] ) {
	data = false;
	break;
      }
    }
  } while ( data );

  // keep not transfered data:
  for ( int j=0; j<NDevices; j++ ) {
    //    cerr << " j=" << j << " bufferinx[j]=" << bufferinx[j] << " readn[j]=" << readn[j] << " NBuffer[j]=" << NBuffer[j] << '\n';
    if ( bufferinx[j] < readn[j] ) {
      if ( bufferinx[j] > 0 ) {
	int binx = bufferinx[j]*BufferElemSize[j];
	//	cerr << "MEMMOVE binx=" << binx << " j=" << j << " NBuffer[j]=" << NBuffer[j] << " BufferSize[j]=" << BufferSize[j] << " NBuffer[j]-binx=" << NBuffer[j]-binx << '\n';
	memmove( &Buffer[j][0], &Buffer[j][binx], NBuffer[j]-binx );
	NBuffer[j] -= binx;
      }
    }
    else
      NBuffer[j] = 0;
  }

  return 0;
}

