/*
  comedithread.cc
  DataThread implementation for acquisition of data using Comedi

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
#include "comedithread.h"


// Here are some defines for new, experimental features.
// Disable them with '//' to activate the old code.
// Any changes made here require recompilation and installation
// using make && make install !

// CALIBQTFIX reads in calibration data from the fishgrid.cfg file
// instead from the comedi calibration file.
// Use the program fishgridcalibcomedi to write the calibration data
// into the fishgrid.cfg file.
// Calibration is working properly if you see on every channel 
// (i) a zero voltage if the amplifier is off and (ii) a properly scaled
// signal if a signal is applied.
// #define CALIBQTFIX 1
// #define PRINTCALIB 1


// READNONBLOCK make the reads non blocking:
#define READNONBLOCK 1


// The following RTSI syncrhonizing code comes from 
// http://www.comedi.org/doc/experimentalfunctionality.html#RTSI

// SYNCAIMASTER enables basic framework for syncing the devices.
#define SYNCAIMASTER 1

// SYNCAISTART triggers all analog input devices on the first device.
// Requires SYNCAIMASTER to be enabled.
#define SYNCAISTART 1

// SYNCAICONVERT triggers all scan starts on the ones of the first device.
// Requires SYNCAIMASTER (and SYNCAICLOCK?) to be enabled.
// WARNING! DOES NOT WORK! comedi_test does not like the convert_arg value.
// #define SYNCAICONVERT 1

// SYNCAICLOCK uses clock of the first device for all other devices.
// Requires SYNCAIMASTER to be enabled.
#define SYNCAICLOCK 1


ComediThread::ComediThread( ConfigData *cd )
  : DataThread( "Acquisition", cd )
{
  for ( int j=0; j<MaxDevices; j++ ) {
    addText( "device" + Str( j+1 ), "/dev/comedi" + Str( j ) );
    addText( "blacklist" + Str( j+1 ), "" );
#ifdef CALIBQTFIX
    addInteger( "caliborder" + Str( j+1 ), 1 );
    addNumber( "caliborigin" + Str( j+1 ), 0.0 );
    addNumber( "calibcoeff0" + Str( j+1 ), 0.0, "", "%.20f" );
    addNumber( "calibcoeff1" + Str( j+1 ), 1.0, "", "%.20f" );
    addNumber( "calibcoeff2" + Str( j+1 ), 0.0, "", "%.20f" );
    addNumber( "calibcoeff3" + Str( j+1 ), 0.0, "", "%.20f" );
#endif
  }
  addSelection( "reference", "RSE|DIFF|RSE|NRSE" );
}


string cmd_src( int src )
{
  string buf = "";

  if ( src & TRIG_NONE ) buf += "none|";
  if ( src & TRIG_NOW ) buf += "now|";
  if ( src & TRIG_FOLLOW ) buf += "follow|";
  if ( src & TRIG_TIME ) buf += "time|";
  if ( src & TRIG_TIMER ) buf += "timer|";
  if ( src & TRIG_COUNT ) buf += "count|";
  if ( src & TRIG_EXT ) buf += "ext|";
  if ( src & TRIG_INT ) buf += "int|";
#ifdef TRIG_OTHER
  if ( src & TRIG_OTHER ) buf +=  "other|";
#endif

  if ( buf.empty() )
    //    buf = "unknown(" << setw( 8 ) << src ")";
    buf="unknown";
  else
    buf.erase( buf.size()-1 );
  
  return buf;
}


void dump_cmd( comedi_cmd *cmd )
{
  cerr << "subdevice:      " << cmd->subdev << '\n';
  cerr << "start:      " << Str( cmd_src(cmd->start_src), 8 ) << "  " << cmd->start_arg << '\n';
  cerr << "scan_begin: " << Str( cmd_src(cmd->scan_begin_src), 8 ) << "  " << cmd->scan_begin_arg << '\n';
  cerr << "convert:    " << Str( cmd_src(cmd->convert_src), 8 ) << "  " << cmd->convert_arg << '\n';
  cerr << "scan_end:   " << Str( cmd_src(cmd->scan_end_src), 8 ) << "  " << cmd->scan_end_arg << '\n';
  cerr << "stop:       " << Str( cmd_src(cmd->stop_src), 8 ) << "  " << cmd->stop_arg << '\n';
}


int ComediThread::initialize( double duration )
{
  printlog( "Number of channels needed: " + Str( channels() ) );
  int nchan = channels();

  // reference:
  int ref = index( "reference" );
  printlog( "Comedi reference: " + Str( ref ) + " (0=DIFF, 1=RSE, 2=NRSE)" );

  NDevices = 0;
  int grid = 0;
  while ( grid < maxGrids() && ! used(grid) )
    grid++;
  int gridchannels = 0;

#ifdef SYNCAIMASTER
  comedi_t *masterdevice = 0;
  unsigned int  mastersubdev = -1;
#endif
  unsigned int masterscanbeginarg = 0;

  for ( int j=0; j<MaxDevices; j++ ) {
    // open comedi device:
    string devicefile = text( "device" + Str( j+1 ) );
    if ( devicefile.empty() )
      continue;
    DeviceP[NDevices] = comedi_open( devicefile.c_str() );
    if ( DeviceP[NDevices] == NULL ) {
      if ( NDevices == 0 )
	printlog( "! error: ComediThread::initialize() -> Device-file "
		  + text( "device" + Str( j+1 ) ) + " could not be opened!" );
      else
	printlog( "ComediThread::initialize() -> Device-file "
		  + text( "device" + Str( j+1 ) ) + " could not be opened!" );
      continue;
    }
    
    // get AI subdevice:
    int subdev = comedi_find_subdevice_by_type( DeviceP[NDevices], COMEDI_SUBD_AI, 0 );
    if ( subdev < 0 ) {
      printlog( "! error: ComediThread::initialize() -> No subdevice for AI found on device "
		+ devicefile );
      comedi_close( DeviceP[NDevices] );
      DeviceP[NDevices] = NULL;
      continue;
    }
    SubDevice[NDevices] = subdev;

    // lock AI subdevice:
    if ( comedi_lock( DeviceP[NDevices], SubDevice[NDevices] ) != 0 ) {
      printlog( "! error: ComediThread::initialize() -> Locking of AI subdevice failed on device "
		+ devicefile );
      comedi_close( DeviceP[NDevices] );
      DeviceP[NDevices] = NULL;
      SubDevice[NDevices] = 0;
      continue;
    }  

    // check for async. command support:
    if ( ( comedi_get_subdevice_flags( DeviceP[NDevices], SubDevice[NDevices] ) & SDF_CMD_READ ) == 0 ) {
      printlog( "! error: ComediThread::initialize() -> Device "
		+ devicefile + " not supported! SubDevice needs to support async. commands!" );
      comedi_unlock( DeviceP[NDevices],  SubDevice[NDevices] );
      comedi_close( DeviceP[NDevices] );
      DeviceP[NDevices] = NULL;
      SubDevice[NDevices] = 0;
      continue;
    }

    printlog( "Opened comedi device: " + devicefile );

#ifndef READNONBLOCK
    // set non blocking mode:
      if ( fcntl( comedi_fileno(DeviceP[NDevices]), F_SETFL, O_NONBLOCK ) < 0 ) {
      printlog( "! error: ComediThread::initialize() -> Setting nonblcoking mode failed on device"
		+ devicefile );
    }
#endif

    // set size of comedi-internal buffer to maximum:
    int readbuffersize = comedi_get_max_buffer_size( DeviceP[NDevices], SubDevice[NDevices] );
    comedi_set_buffer_size( DeviceP[NDevices], SubDevice[NDevices], readbuffersize );
    comedi_get_buffer_size( DeviceP[NDevices], SubDevice[NDevices] );

    // get calibration:
#ifndef CALIBQTFIX
    {
      char *calibpath = comedi_get_default_calibration_path( DeviceP[NDevices] );
      printlog( "read in calibration from " + Str( calibpath ) );
      ifstream cf( calibpath );
      if ( cf.good() )
	Calibration[NDevices] = comedi_parse_calibration_file( calibpath );
      else
	Calibration[NDevices] = NULL;
      if ( Calibration[NDevices] == NULL )
	printlog( "calibration file does not exist, or reading failed." );
      delete [] calibpath;
    }
#else
	Calibration[NDevices] = NULL;
#endif

    // get size of datatype for sample values:
    LongSampleType[NDevices] = ( comedi_get_subdevice_flags( DeviceP[NDevices], SubDevice[NDevices] ) &
				 SDF_LSAMPL );
    if ( LongSampleType[NDevices] )
      BufferElemSize[NDevices] = sizeof( lsampl_t );
    else
      BufferElemSize[NDevices] = sizeof( sampl_t );

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
      printlog( "! error: ComediThread::initialize() -> invalid reference" );
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
	printlog( "! error in ComediThread::initialize() -> comedi_find_range: no range for "
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
#ifdef CALIBQTFIX
      Calib[NDevices][NChannels[NDevices]].order =
	integer( "caliborder" + Str( NDevices+1 ) );
      Calib[NDevices][NChannels[NDevices]].expansion_origin =
	number( "caliborigin" + Str( NDevices+1 ) );
      Calib[NDevices][NChannels[NDevices]].coefficients[0] =
	number( "calibcoeff0" + Str( NDevices+1 ) );
      Calib[NDevices][NChannels[NDevices]].coefficients[1] =
	number( "calibcoeff1" + Str( NDevices+1 ) );
      Calib[NDevices][NChannels[NDevices]].coefficients[2] =
	number( "calibcoeff2" + Str( NDevices+1 ) );
      Calib[NDevices][NChannels[NDevices]].coefficients[3] =
	number( "calibcoeff3" + Str( NDevices+1 ) );
#ifdef PRINTCALIB
      cerr << "  Calibration order=" << Calib[NDevices][NChannels[NDevices]].order << '\n';
      cerr << "    origin=" << Calib[NDevices][NChannels[NDevices]].expansion_origin << '\n';
      for ( unsigned int k=0; k<=Calib[NDevices][NChannels[NDevices]].order; k++ )
	cerr << "    coeff[" << k << "]=" << Calib[NDevices][NChannels[NDevices]].coefficients[k] << '\n';
#endif
#endif
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
      printlog( "! error in ComediThread::initialize() -> no channels to be configured" );
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
      printlog( "! error in ComediThread::initialize() -> comedi_get_cmd_generic_timed failed: "
		+ Str( comedi_strerror( comedi_errno() ) ) + ")" );
      comedi_close( DeviceP[NDevices] );
      DeviceP[NDevices] = NULL;
      SubDevice[NDevices] = 0;
      delete [] Calib[NDevices];
      Calib[NDevices] = 0;
      continue;
    }
    if ( cmd.scan_begin_src != TRIG_TIMER ) {
      printlog( "! error in ComediThread::initialize() -> acquisition timed by a daq-board counter not possible" );
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

	/* not used....
        configCmd.chanspec = 1;
        configCmd.data[1] = NI_RTSI_OUTPUT_ADR_START2;
        retVal = comedi_do_insn( masterdevice, &configCmd );
        if ( retVal < 0 ) {
	  comedi_perror( "comedi_do_insn: INSN_CONFIG set RTSI_OUTPUT_ADR_START2" );
	  failed = true;
        }
        retVal = comedi_dio_config( masterdevice, rtsi_subdev, 1,
				    INSN_CONFIG_DIO_OUTPUT );
        if ( retVal < 0 ) {
	  comedi_perror( "comedi_dio_config RTSI_OUTPUT_ADR_START2 for output" );
	  failed = true;
        }
	*/

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
	  printlog( "! error in ComediThread::initialize() -> failed to set up master device" );
	  comedi_close( DeviceP[NDevices] );
	  DeviceP[NDevices] = NULL;
	  SubDevice[NDevices] = 0;
	  delete [] Calib[NDevices];
	  Calib[NDevices] = 0;
	  continue;
	}

      }
      else {
	printlog( "! error in ComediThread::initialize() -> trigger INT not supported" );
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
	  printlog( "! error in ComediThread::initialize() -> failed to set up slave device" );
	  comedi_close( DeviceP[NDevices] );
	  DeviceP[NDevices] = NULL;
	  SubDevice[NDevices] = 0;
	  delete [] Calib[NDevices];
	  Calib[NDevices] = 0;
	  continue;
	}

      }
      else {
	printlog( "! error in ComediThread::initialize() -> trigger EXT not supported" );
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
      printlog( "! error in ComediThread::initialize() -> trigger NOW not supported" );
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
	printlog( "! error in ComediThread::initialize() -> continuous mode not supported!" );
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
	printlog( "! error in ComediThread::initialize() -> unsupported trigger" );
	if ( cmd.start_src != testCmd.start_src )
	  printlog( "! error in ComediThread::initialize() -> unsupported trigger in start_src" );
	if ( cmd.scan_begin_src != testCmd.scan_begin_src )
	  printlog( "! error in ComediThread::initialize() -> unsupported trigger in scan_begin_src" );
	if ( cmd.convert_src != testCmd.convert_src )
	  printlog( "! error in ComediThread::initialize() -> unsupported trigger in convert_src" );
	if ( cmd.scan_end_src != testCmd.scan_end_src )
	  printlog( "! error in ComediThread::initialize() -> unsupported trigger in scan_end_src" );
	if ( cmd.stop_src != testCmd.stop_src )
	  printlog( "! error in ComediThread::initialize() -> unsupported trigger in stop_src" );
	break;
      case 2: // invalid trigger in *_src:
	printlog( "! error in ComediThread::initialize() -> invalid trigger" );
	if ( cmd.start_src != testCmd.start_src )
	  printlog( "! error in ComediThread::initialize() -> invalid trigger in start_src" );
	if ( cmd.scan_begin_src != testCmd.scan_begin_src )
	  printlog( "! error in ComediThread::initialize() -> invalid trigger in scan_begin_src" );
	if ( cmd.convert_src != testCmd.convert_src )
	  printlog( "! error in ComediThread::initialize() -> invalid trigger in convert_src" );
	if ( cmd.scan_end_src != testCmd.scan_end_src )
	  printlog( "! error in ComediThread::initialize() -> invalid trigger in scan_end_src" );
	if ( cmd.stop_src != testCmd.stop_src )
	  printlog( "! error in ComediThread::initialize() -> invalid trigger in stop_src" );
	break;
      case 3: // *_arg out of range:
	printlog( "! error in ComediThread::initialize() -> _arg out of range:" );
	if ( cmd.start_arg != testCmd.start_arg )
	  printlog( "! error in ComediThread::initialize() -> start_arg out of range" );
	if ( cmd.scan_begin_arg != testCmd.scan_begin_arg ) {
	  printlog( "! warning in ComediThread::initialize() -> requested sampling period of "
		    + Str( testCmd.scan_begin_arg ) + "ns smaller than supported! min "
		    + Str( cmd.scan_begin_arg ) + "ns sampling interval possible." );
	  // XXX	setSampleRate( 1.0e9 / cmd.scan_begin_arg );
	}
	if ( cmd.convert_arg != testCmd.convert_arg )
	  printlog( "! error in ComediThread::initialize() -> convert_arg out of range" );
	if ( cmd.scan_end_arg != testCmd.scan_end_arg )
	  printlog( "! error in ComediThread::initialize() -> scan_end_arg out of range" );
	if ( cmd.stop_arg != testCmd.stop_arg )
	  printlog( "! error in ComediThread::initialize() -> stop_arg out of range" );
	break;
      case 4: // adjusted *_arg:
	printlog( "! warning in ComediThread::initialize() -> _arg adjusted" );
	if ( cmd.start_arg != testCmd.start_arg )
	  printlog( "! error in ComediThread::initialize() -> start_arg adjusted" );
	if ( cmd.scan_begin_arg != testCmd.scan_begin_arg ) {
	  printlog( "! warning in ComediThread::initialize() -> scan_begin_arg adjusted" );
	  clearError();
	  // XXX setSampleRate( 1.0e9 / cmd.scan_begin_arg );
	}
	if ( cmd.convert_arg != testCmd.convert_arg )
	  printlog( "! error in ComediThread::initialize() -> convert_arg adjusted" );
	if ( cmd.scan_end_arg != testCmd.scan_end_arg )
	  printlog( "! error in ComediThread::initialize() -> scan_end_arg adjusted" );
	if ( cmd.stop_arg != testCmd.stop_arg )
	  printlog( "! error in ComediThread::initialize() -> stop_arg adjusted" );
	break;
      case 5: // invalid chanlist:
	printlog( "! error in ComediThread::initialize() -> invalid chanlist" );
	break;
      }
    }

    if ( error() ) {
      printlog( "! error in ComediThread::initialize() -> setting up command failed!" );
      comedi_close( DeviceP[NDevices] );
      DeviceP[NDevices] = NULL;
      SubDevice[NDevices] = 0;
      delete [] Calib[NDevices];
      Calib[NDevices] = 0;
      continue;
    }

    if ( masterscanbeginarg > 0 && cmd.scan_begin_arg != masterscanbeginarg ) {
      printlog( "! error in ComediThread::initialize() -> sampling rates not identical!" );
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
	  printlog( "! warning in ComediThread::initialize() -> apply calibration for channel "
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
      printlog( "! error in ComediThread::initialize() -> " + devicefile
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
    printlog( "! error in ComediThread::initialize() -> cannot record from all " + Str( channels() ) + " channels." );

#ifdef SYNCAIMASTER
  // start masterdevice:
  if ( masterdevice == 0 )
    printlog( "! error in ComediThread::initialize() -> no master device set." );
  comedi_insn masterstart;
  lsampl_t insndata[1];
  insndata[0] = 0;
  masterstart.insn = INSN_INTTRIG;
  masterstart.subdev = mastersubdev;
  masterstart.data = insndata;
  masterstart.n = 1;
  cerr << "executing masterdevice " << (void*)masterdevice << "\n";
  if ( comedi_do_insn( masterdevice, &masterstart ) < 0 ) {
    printlog( "! error in ComediThread::initialize() -> cannot execute instruction for internal trigger to start master device." );
    comedi_perror( "reason is" );
  }
#endif

  DeviceInx = 0;
  ChannelInx = 0;

  cerr << "NDevices = " << NDevices << '\n';

  return NDevices > 0 ? 0 : -1;
}


void ComediThread::finish( void )
{
  for ( int j=0; j<NDevices; j++ ) {
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


int ComediThread::read( void )
{
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
    readn[j] = 0;   // XXX This was missing! (10.5.2014)
    bufferinx[j] = 0;

    // get data from daq driver:
    //    cerr << "READ " << BufferSize[j]-NBuffer[j] << '\n';
    ssize_t m = ::read( comedi_fileno( DeviceP[j] ),
			&Buffer[j][NBuffer[j]], BufferSize[j]-NBuffer[j] );
    int ern = errno;
    //    cerr << "READ j=" << j << " m=" << m << " ern=" << ern << " NBuffer[j]=" << NBuffer[j] << " bufferinx[j]=" << bufferinx[j] << " readn[j]=" << readn[j] << '\n';
    if ( m < 0 && ern != EAGAIN && ern != EINTR ) {
      printlog( "ComediThread::read(): error on device " + Str( j )
		+ " -> " + Str( errno ) + ": " + Str( strerror( ern ) ) );
      return -1;
    }

    // no more data to be read:
    if ( m <= 0 && ( comedi_get_subdevice_flags( DeviceP[j], SubDevice[j] )
		     & SDF_RUNNING ) == 0 ) {
      printlog( "! error in ComediThread::read(): no data and not running on device " + Str( j ) );
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

