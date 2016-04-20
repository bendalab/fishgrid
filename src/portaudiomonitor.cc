#include <iostream>
#include <cmath>
#include <unistd.h>
#include <sys/stat.h>
#include <termios.h>
#include <portaudiomonitor.h>

using namespace std;
using namespace relacs;


PortAudioMonitor::PortAudioMonitor( void )
  : Running( false )
{
};

  
int PortAudioMonitor::init( const BrowseDataWidget *data )
{
  Offset = 0;
  Data = data;
  Gain = 0.001/Data->MaxBoardVolts;
		
  // INITIALIZE PORTAUDIO
  PaError err = Pa_Initialize();
  if( err != paNoError )
    goto error;
		
  // OUTPUT PARAMETERS
  OutputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
  if ( OutputParameters.device == paNoDevice ) {
    cerr << "Error: No default output device.\n";
    goto error;
  }
  OutputParameters.channelCount = 2; /* stereo output */
  OutputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
  OutputParameters.suggestedLatency = Pa_GetDeviceInfo( OutputParameters.device )->defaultLowOutputLatency;
  OutputParameters.hostApiSpecificStreamInfo = NULL;		
		
  // open stream
  err = Pa_OpenStream( &Stream,
		       NULL, /* no input */
		       &OutputParameters,
		       (int)::rint( 1.0/buffer.stepsize() ),
		       paFramesPerBufferUnspecified,  /* OR: paFramesPerBufferUnspecified */
		       paClipOff, /* we won't output out of range samples so don't bother clipping them */
		       callback,
		       this );
  cerr << err << '\n';
  if( err != paNoError )
    goto error;
		
  /* register callback for finished stream: 
  //sprintf( data.message, "No Message" );
  err = Pa_SetStreamFinishedCallback( Stream, &StreamFinished );
  if( err != paNoError )
    goto error;
  */

  // START STREAM
  err = Pa_StartStream( Stream );
  if( err != paNoError )
    goto error;

  Mutex.lock();
  Running = true;
  Mutex.unlock();
		
  return paNoError;

 error:
  Pa_Terminate();
  cerr << "An error occured while using the portaudio stream\n";
  cerr << "Error number: " << err << '\n';
  cerr << "Error message: " << Pa_GetErrorText( err ) << '\n';
  return err;
}

	
int PortAudioMonitor::close( void )
{
  // stop stream
  PaError err = Pa_StopStream( Stream );
  if( err != paNoError ) 
    goto error;
		
  // close stream
  err = Pa_CloseStream( Stream );
  if( err != paNoError ) 
    goto error;
		
  // terminate port audio
  Pa_Terminate();

  Mutex.lock();
  Running = false;
  Mutex.unlock();
		
  return 0;
		
 error:
  Pa_Terminate();
  cerr << "An error occured while using the portaudio stream\n";
  cerr << "Error number: " << err << '\n';
  cerr << "Error message: " << Pa_GetErrorText( err ) << '\n';
  return err;
}


bool PortAudioMonitor::running( void ) const
{
  Mutex.lock();
  bool r = Running;
  Mutex.unlock();
  return r;
}
	

void PortAudioMonitor::streamFinished( void* userData )
{
  PortAudioMonitor *data = (PortAudioMonitor*)userData;
  data->Mutex.lock();
  data->Running = false;
  data->Mutex.unlock();
}	


int PortAudioMonitor::callback( const void *inputBuffer, void *outputBuffer,
				unsigned long framesPerBuffer,
				const PaStreamCallbackTimeInfo* timeInfo,
				PaStreamCallbackFlags statusFlags,
				void *userData )
{
  PortAudioMonitor *data = (PortAudioMonitor*)userData;
  float *out = (float*)outputBuffer;

  /*
  (void) timeInfo; // Prevent unused variable warnings. 
  (void) statusFlags;
  (void) inputBuffer;
  */

  int grid = data->Data->Grid;
  int row = data->Data->Row[grid];
  int col = data->Data->Column[grid];
  const SampleDataF &data = data->Data->Data[grid][row][col]
  int index = data->Offset;
  double gain = data->Gain;
  for( int i = 0; i < (int)framesPerBuffer; i++ ) {
    *out++ = data[index] * gain; /* left */
    *out++ = data[index] * gain; /* right */
    index++;
  }
  data->Offset = index;
		
  return paContinue;
}

/*
int main ( int argc, char *argv[] ) 
{
  PlayAudio pa = PlayAudio();
  pa.init();
  pa.close();
	
  return 0;
}
*/
