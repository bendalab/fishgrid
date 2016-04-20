/*
  comedithread.h
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

#ifndef _COMEDITHREAD_H_
#define _COMEDITHREAD_H_ 1

#include <comedilib.h>
#include "datathread.h"

using namespace std;


/*! 
\class ComediThread
\brief DataThread implementation for acquisition of data using Comedi
\author Jan Benda
*/

class ComediThread : public DataThread
{

public:

  ComediThread( ConfigData *cd );


protected:

  virtual int initialize( double duration=0.0 );
  virtual void finish( void );
  virtual int read( void );


private:

    /*! Number of comedi devices in use. */
  int NDevices;
    /*! Maximum number of comedi device supportet. */
  static const int MaxDevices = 4;
    /*! Pointer to the comedi device. */
  comedi_t *DeviceP[MaxDevices];
    /*! The comedi subdevice number. */
  unsigned int SubDevice[MaxDevices];
    /*! Calibration info. */
  comedi_calibration_t *Calibration[MaxDevices];
    /*! Calibration data. */
  comedi_polynomial_t *Calib[MaxDevices];
    /*! The number of channels that are sampled from each device. */
  int NChannels[MaxDevices];
    /*! The number of samples that are to be sampled from each device. */
  int MaxSamples[MaxDevices];
    /*! The number of samples that are already sampled from each device. */
  int Samples[MaxDevices];
    /*! The grid index for each channel. */
  int GridChannel[MaxDevices][100];
    /*! True if the sample type is lsampl_t. */
  bool LongSampleType[MaxDevices];
    /*! The size of a single sample in bytes. */
  unsigned int BufferElemSize[MaxDevices];
    /*! Size of the internal buffers used for getting the data from the driver (in bytes). */
  int BufferSize[MaxDevices];
    /*! Number of bytes in the internal buffers used for getting the data from the driver. */
  int NBuffer[MaxDevices];
    /*! The internal buffers used for getting the data from the driver. */
  char *Buffer[MaxDevices];
    /*! Index of current device for writing data into the buffer. */
  int DeviceInx;
    /*! Index of current channel of current device for writing data into the buffer. */
  int ChannelInx;

};


#endif /* ! _COMEDITHREAD_H_ */

