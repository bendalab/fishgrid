/*
  configdata.cc
  All the configuration data for the fishgrid recordings.

  FishGrid
  Copyright (C) 2009-2011 Jan Benda <benda@bio.lmu.de> & Joerg Henninger <henninger@bio.lmu.de>

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

#include <QDateTime>
#include <relacs/str.h>
#include "configdata.h"

using namespace std;
using namespace relacs;


ConfigData::ConfigData( const string &configfile )
  : ConfigClass( "FishGrid" ),
    Grid( 0 ),
    MaxVolts( 0.0 ),
    SampleRate( 0.0 ),
    Gain( 0.0 ),
    Unit( "mV" ),
    DataTime( 0.0 ),
    DataInterval( 0.0 ),
    ProcessInterval( 1000 ),
    BufferTime( 0.0 ),
    CFG()
{
  Used[0] = true;
  for ( int g=1; g<MaxGrids; g++ )
    Used[g] = false;
  for ( int g=0; g<MaxGrids; g++ ) {
    Row[g] = 0;
    Column[g] = 0;
  }

  // configuration files:
  CFG.addConfigFile( Str::homePath() + "." + configfile );
  CFG.addConfigFile( configfile );
  addConfig();

  // options:
  // flags: 1 startup dialog, 2 startup readonly, 4 metadata dialog, 8 metadata readonly, 
  // 16 save in configuration file,
  // XML flags: 32 Setup, 64 Recording, 128 DAQ Settings, 256 Amplifier Settings,
  //            512 DAQ Properties, 1024 Amplifier Properties, 2048 Experiment 

  for ( int g=0; g<MaxGrids; g++ ) {
    string ns = Str( g+1 );
    newSection( "Grid &"+ns ).setFlag( 1+4+16 );
    addText( "Name"+ns, "Name of the setup", "FishGrid-V1" ).setFlags( 32 );
    addText( "Configuration"+ns ).setFlags( 32 );
    addBoolean( "Used"+ns, "Use this grid", Used[g], 0, 0 ).setFlags( 1+4+8+16+32 );
    addInteger( "Columns"+ns, "Number of columns", 4 ).setFlags( 1+4+8+16+32 );
    addInteger( "Rows"+ns, "Number of rows", 4 ).setFlags( 1+4+8+16+32 );
    addNumber( "ColumnDistance"+ns, "Distance between columns",  50.0, 0.0, 100000.0, 5.0, "cm", "cm", "%.1f" ).setFlags( 1+4+16+32 );
    addNumber( "RowDistance"+ns, "Distance between rows",  50.0, 0.0, 100000.0, 5.0, "cm", "cm", "%.1f" ).setFlags( 1+4+16+32 );
    addInteger( "ChannelOffset"+ns, "Offset for read in channels", 0 ).setFlags( 1+2+16+32 );
    addText( "ElectrodeType"+ns, "Type of electrode tips", "plain" ).setFlags( 16+32 );
    int ro = g == 0 ? 2+8 : 0;
    if ( g > 0 ) {
      addNumber( "GridPosX"+ns, "X position of the grid",  0.0, -1000.0, 1000.0, 0.1, "m", "m", "%.2f" ).setFlags( 1+4+ro+16+32 );
      addNumber( "GridPosY"+ns, "Y position of the grid",  0.0, -1000.0, 1000.0, 0.1, "m", "m", "%.2f" ).setFlags( 1+4+ro+16+32 );
      addNumber( "GridOrientation"+ns, "Orientation of the grid",  0.0, -360.0, 360.0, 1.0, "Degree", "Degree", "%.0f" ).setFlags( 1+4+ro+16+32 );
    }
    else {
      addText( "RefElectrodeType"+ns, "Type of reference electrode", "plain|grid 1|grid 2|grid 3| grid 4" ).setFlags( 1+4+16+32 );
      addNumber( "RefElectrodePosX"+ns, "X position of reference electrode",  0.0, -1000.0, 1000.0, 0.1, "m", "m", "%.2f" ).setFlags( 1+4+16+32 );
      addNumber( "RefElectrodePosY"+ns, "Y position of reference electrode",  0.0, -1000.0, 1000.0, 0.1, "m", "m", "%.2f" ).setFlags( 1+4+16+32 );
    }
    addNumber( "WaterDepth"+ns, "Depth of water",  1.0, 0.0, 1000.0, 0.1, "m", "m", "%.2f" ).setFlags( 1+4+16+32 );
  }

  newSection( "Hardware Settings" ).setFlag( 1+16 );
  newSubSection( "DAQ board" ).setFlag( 1+16 );
  //  addText( "DAQName", "Name", "NI-USB6211-1" ).setFlags( 512 );
  addText( "DAQName", "Name", "NI-PCI-6259-1" ).setFlags( 512 );
  addText( "DAQVendor", "Vendor", "National Instruments" ).setFlags( 512 );
  addText( "DAQModel", "Model", "NI-PCI-6259" ).setFlags( 512 );
  addInteger( "AIChannelCount", 32 ).setFlags( 512 );
  addInteger( "AIResolution", 16, "bit" ).setFlags( 512 );
  addNumber( "AIMaxSampleRate", 1000000.0, "Hz", "kHz" ).setFlags( 512 );
  addNumber( "AISampleRate", "Sample rate per electrode",  10000.0, 0.0, 10000000.0, 1000.0, "Hz", "kHz", "%.3f" ).setFlags( 1+16+128 );
  addInteger( "AIUsedChannelCount" ).setFlags( 128 );
  addNumber( "AIMaxVolt", "Maximum voltage to be expected",  1.0, 0.0, 100.0, 0.0001, "V", "mV", "%.1f" ).setFlags( 1+16+128 );
  newSubSection( "Amplifier" ).setFlag( 1+16 );
  //  addText( "AmplModel", "Name", "16-channel-outdoor-USB-1|16-channel-EPMS-module" ).setFlags( 1+16+1024 );
  //  addText( "AmplName", "Name", "16-channel-EPMS-module" ).setFlags( 1+16+1024 );
  addText( "AmplName", "Name", "64-channel-amplifier" ).setFlags( 1+16+1024 );
  addText( "AmplVendor", "Vendor", "npi electronic" ).setFlags( 1024 );
  //  addText( "AmplModel", "Model", "EXT-16|EXT-16M" ).setFlags( 1+16+1024 );
  addText( "AmplModel", "Model", "EM-64" ).setFlags( 1+16+1024 );
  addText( "Type", "Differential" ).setFlags( 1+16+1024 );
  addInteger( "ChannelCount", 16 ).setFlags( 1024 );
  addNumber( "Gain", "Gain factor of the amplifier", 1.0, 0.0, 1000000.0, 100.0 ).setFlags( 1+16+256 );
  addInteger( "HighpassOrder", 1 ).setFlags( 1024 );
  addInteger( "LowpassOrder", 1 ).setFlags( 1024 );
  addNumber( "HighpassCutoff", "Cutoff frequency of the high-pass filter", 100.0, 0.0, 100000.0, 50.0, "Hz", "Hz", "%.0f" ).setFlags( 1+16+256 );
  addNumber( "LowpassCutoff", "Cutoff frequency of the low-pass filter", 5000.0, 0.0, 100000.0, 1000.0, "Hz", "kHz", "%.2f" ).setFlags( 1+16+256 );
  newSection( "Recording" ).setFlag( 1+4+16 );
  newSubSection( "General" ).setFlag( 1+16 );
  addText( "Experiment.Name", "An identifier for the experiment", "recording fish behavior|testing with real fish|testing with artificial fish" ).setFlags( 1+4+16+2048 );
  addText( "Experiment.ProjectName", "FishGrid" ).setFlags( 2048 );
  addDate( "StartDate", "Date" ).setFlags( 16+64 );
  addTime( "StartTime", "Time" ).setFormat( "%02H:%02M:%02S.%03Z" ).setFlags( 16+64 );
  addText( "Location", "Location of the grid", "" ).setFlags( 1+4+16+64 );
  addText( "Position", "Position of grid origin", "" ).setFlags( 1+4+16+64 );
  addNumber( "WaterTemperature", "Water temperature",  30.0, -100.0, 100.0, 1.0, "C", "C", "%.1f" ).setFlags( 1+4+16+64 );
  addNumber( "WaterConductivity", "Water conductivity",  100.0, 0.0, 100000.0, 10.0, "uS/cm", "uS/cm", "%.1f" ).setFlags( 1+4+16+64 );
  addNumber( "WaterpH", "Water pH",  7.0, 0.0, 30.0, 0.1, "", "", "%.1f" ).setFlags( 1+4+16+64 );
  addNumber( "WaterOxygen", "Water oxygen",  20.0, 0.0, 1000.0, 1.0, "mg/l", "mg/l", "%.2f" ).setFlags( 1+4+16+64 );
  addText( "Comment", "Comment", "" ).setFlags( 1+4+16+64 );
  addText( "Experimenter", "Experimenter", "" ).setFlags( 1+4+16+64 );
  newSubSection( "Buffers and timing" ).setFlag( 1+16 );
  addNumber( "DataTime", "Length of data buffer used for analysis",  0.1, 0.0, 10000.0, 0.1, "s", "ms", "%.0f" ).setFlags( 1+16+64 );
  addNumber( "DataInterval", "Interval between data buffer updates",  1.0, 0.0, 10000.0, 0.1, "s", "ms", "%.0f" ).setFlags( 1+16+64 );
  addNumber( "BufferTime", "Length of input buffer",  60.0, 1.0, 100000.0, 10.0, "s", "s", "%.0f" ).setFlags( 1+16+64 );
  setConfigSelectMask( 16 );
}


ConfigData::~ConfigData( void )
{
}


void ConfigData::setup( void )
{
  ProcessInterval = (int)::rint( 1000.0*DataInterval );

  // data analysis buffer:
  for ( int g=0; g<MaxGrids; g++ ) {
    if ( Used[g] ) {
      Data[g].resize( Rows[g] );
      for ( unsigned int r=0; r<Data[g].size(); r++ ) {
	Data[g][r].resize( Columns[g] );
	for ( unsigned int c=0; c<Data[g][r].size(); c++ )
	  Data[g][r][c].resize( DataTime, 1.0/SampleRate, 0.0F );
      }
    }
  }
}


void ConfigData::printlog( const string &message ) const
{
  cerr << QTime::currentTime().toString().toAscii().data() << " "
       << message << endl;
}


