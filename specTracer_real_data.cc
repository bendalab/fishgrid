/* This is Joergs spectrum analyzer program. We eventually want to integrate this into the spectra class. */

#include <cstdio>
#include <iostream>
#include <cmath>
#include <vector>
#include <deque>
#include <string>
#include <algorithm>
#include <fstream>

#include <unistd.h>
#include <sys/stat.h>
#include <termios.h>
#include <getopt.h>
#include <time.h>

#include <relacs/array.h>
#include <relacs/sampledata.h>
#include <relacs/datafile.h>
#include <relacs/options.h>
#include <relacs/fitalgorithm.h>
#include <relacs/spectrum.h>
#include <relacs/eventdata.h>
#include <relacs/map.h>
#include <relacs/detector.h>

using namespace std;
using namespace relacs;

// ##############################################################
// ##############################################################

// FILES AND DIRECTORIES
string resultDirName = "Detections/";
string metaData = "metadata.cfg";

// FLAGS
bool debug = false;
bool save_spectra = false;
bool simulated_data = false;

// PARAMETERS
int spectrumLength = 1*60; // [seconds]
int freqStep = 1.0; // if the frequency-resolution is smaller than this, the spectra will be reduced in resolution before saving

// DATA PARAMETERS
//~ int tOff = 3*60*60 + 23*60 + 55; // offset of analyzes [seconds]
//~ int tLength = 30; // maximum length of analyzes [seconds]
int tOff = 0; // offset of analyzes [seconds]
int tLength = 0; // maximum length of analyzes [seconds]

// SPECTRUM ANALYSIS
	// SET MINIMUM WINDOW SIZE
// double minAnalysisWindowSize = 0.3; // [seconds] // THE MINIMUM SIZE OF THE ANALYSIS WINDOW
//~ double minAnalysisWindowSize = 2.5; // [seconds] // THE MINIMUM SIZE OF THE ANALYSIS WINDOW
int fftWindowSize = 2048*4; // [seconds] //  THE SIZE OF A SINGLE FFT WINDOW WITHIN THE WHOLE ANALYSIS WINDOW
double overlap = 0.85; // OVERLAP OF THE ANALYSIS WINDOWS -- !! not of the fft windows (handled separately)
int fftWindows = 5; // how many analyzes windows should fit in one data chunk; will influence fft resolution // MINIMUM
double psdMaxFreq = 1500.; // maximum frequency to be saved to file
int sumLeftRight = 3; // data points left and right of the detected peak, which will be summed up for the power estimate
double fTol = 0.01; // frequency tolerance for ...
// 0.001 is too small! We should actually make this also dependend on the FFT resolution!

// PEAK DETECTION
double absolute_threshold = 0.05; // minimum size of detected frequencies to be accepted
double thresholdUpper = 1.0;
double thresholdLower = 0.8;

double sim_absolute_threshold = 0.005; // minimum size of detected frequencies to be accepted
double sim_thresholdUpper = 1.0;
double sim_thresholdLower = 1.0;

// DETECTION OF FREQUENCY-GROUPS
int minimumGroupSize = 2; // groupSize must be bigger than this
int	sim_minimumGroupSize = 0; // groupSize must be bigger than this

// THE FREQUENCY RANGE OF FISH FUNDAMENTAL FREQUENCIES
double fFishMin = 51.0; // Hz
double fFishMax = 1200.0; // Hz

// searching for hidden fundamentals. how many harmonies are we trying to recover?
int maxdivisor = 15; // we will not find more harmonies than this
int maxMergeDivisor = 3; // 
int maxHarmonics = (int) 2*ceil(fFishMax / fFishMin); // something about 40
double fMaxDetection = 5000.0; // do not bother to search for harmonics above this

// ##############################################################
// ##############################################################

template < typename ForwardIterX, typename ForwardIterP >
int rPSD_phases( ForwardIterX firstx, ForwardIterX lastx,
	  ForwardIterP firstp, ForwardIterP lastp,
	  ForwardIterP firstph, ForwardIterP lastph,
	  double (*window)( int j, int n ) )
{
  typedef typename iterator_traits<ForwardIterX>::value_type ValueTypeX;
  typedef typename iterator_traits<ForwardIterP>::value_type ValueTypeP;

  int np = lastp - firstp;  // size of power spectrum
	if ( lastph - firstph != np )  // wrong size of phase spectrum
		return -2;
  int nw = np*2;  // window size
  if ( nw <= 2 )
    return -1;
  
  // make sure that nw is a power of 2:
  int logn = 0;
  for ( int k=1; k<nw; k <<= 1 )
    logn++;
  nw = (1 << logn);

  // clear psd:
  for ( ForwardIterP iterp=firstp; iterp != lastp; ++iterp )
	 *iterp = 0.0;
  for ( ForwardIterP iterph=firstph; iterph != lastph; ++iterph )
    *iterph = 0.0;

  ForwardIterX iterx=firstx;
  while ( iterx != lastx ) {
    // copy chunk of data into buffer and apply window:
    ValueTypeX buffer[nw];
    int k=0;
	 for ( ; k<nw && iterx != lastx; ++k, ++iterx )
		buffer[k] = *iterx * window( k, nw );
	 // zero padding
    for ( ; k<nw; k++ )
      buffer[k] = 0.0;

    // fourier transform:
    rFFT( buffer, buffer+nw );

    // add power to psd:
    // first element:
    ForwardIterP iterp=firstp;
    ValueTypeP power = buffer[0] * buffer[0];
    *iterp = power - *iterp;
    ++iterp;
    // remaining elements:
	 for ( int k=1; iterp != lastp; ++iterp, ++k ) {
      power = buffer[k] * buffer[k] + buffer[nw-k] * buffer[nw-k];
      *iterp = 2.0*power - *iterp;
    }
	 
    // add phases:
    // first element:
    ForwardIterP iterph=firstph;
    ValueTypeP phase = 0.0;
    *iterph = phase - *iterph;
    ++iterph;
    // remaining elements:
	for ( int k=1; iterph != lastph; ++iterph, ++k ) {
		phase = atan2( buffer[nw-k], buffer[k] );
		*iterph = phase - *iterph;
	}
  }

  // normalize psd:
  ValueTypeP norm = 0.0;
  for ( int k=0; k<nw; ++k ) {
    ValueTypeP w = window( k, nw );
    norm += w*w;
  }
  norm = 1.0/norm/nw;

  ForwardIterP iterp=firstp;
  for ( int k=0; iterp != lastp && k<nw/2; ++iterp )
    *iterp *= norm;
  // last element:
  if ( iterp != lastp )
    *iterp *= 0.25;

  return 0;
}

// ##############################################


template < typename ContainerX, typename ContainerP >
int rPSD_phases( const ContainerX &x, ContainerP &p, ContainerP &ph,
	double (*window)( int j, int n ) )
{
	// next power of two (fft window size):
  int n = 1;
  for ( n = 1; n < p.size(); n <<= 1 );
  
	// set range of container
	p.setRange( 0.0, 0.5/x.stepsize()/n );
	ph.setRange( 0.0, 0.5/x.stepsize()/n );
	
	return rPSD_phases( x.begin(), x.end(), p.begin(), p.end(), ph.begin(), ph.end(), window );
}


// ##############################################################


template <typename T> 
int sign(T val) 
{
    return (T(0) < val) - (val < T(0));
}


// ##############################################################

void readArgs( int argc, char *argv[], int &filec )
{
  int c;
  if ( argc <= 1 )
  optind = 0;
  opterr = 0;
  while ( (c = getopt( argc, argv, "s" )) >= 0 )
	 switch ( c ) {
	 case 's': 
		break;
	 default : ;
	 }
	filec = optind;
}

bool FolderExists(string dir) 
{  
	struct stat st;
	stat(dir.c_str(),&st);
	return S_ISDIR(st.st_mode);
}

bool FileExists(string dir) 
{  
	struct stat st;
	stat(dir.c_str(),&st);
	return S_ISREG(st.st_mode);
}

bool sort_using_greater_than(double u, double v)
{
	return u < v;
}

bool sort_using_greater_than_for_Maps(MapD u, MapD v)
{
	u.sortByX();
	v.sortByX();
	return u.x(0) < v.x(0);
}

int PressAnyKey( void )
  {
  #define MAGIC_MAX_CHARS 18
  struct termios initial_settings;
  struct termios settings;
  unsigned char  keycodes[ MAGIC_MAX_CHARS ];
  int count;

  tcgetattr( STDIN_FILENO, &initial_settings );
  settings = initial_settings;

  /* Set the console mode to no-echo, raw input. */
  /* The exact meaning of all this jazz will be discussed later. */
  settings.c_cc[ VTIME ] = 1;
  settings.c_cc[ VMIN  ] = MAGIC_MAX_CHARS;
  settings.c_iflag &= ~(IXOFF);
  settings.c_lflag &= ~(ECHO | ICANON);
  tcsetattr( STDIN_FILENO, TCSANOW, &settings );

  count = ::read( STDIN_FILENO, (void*)keycodes, MAGIC_MAX_CHARS );

  tcsetattr( STDIN_FILENO, TCSANOW, &initial_settings );

  return (count == 1)
		 ? keycodes[ 0 ]
		 : -(int)(keycodes[ count -1 ]);
  }
  
// Tiefpassfilter
void 	LowPassFilter ( const SampleDataD &in, SampleDataD &out, const double tau ) {
	out = in;
	out = 0.0;
	for ( int i = 1; i < in.size(); i++ ){
		out[i] = out[i-1] +  (( (in.pos(i)	- in.pos(i-1)) / tau) * (-out[i-1] + in[i-1] ));
	}
}

// check data for clipping in channels
void Clipping( vector<SampleDataD> &analysis, double maxVoltage )
{
	vector<bool> clippedChannels;
	clippedChannels.resize(analysis.size());
	for ( unsigned int c = 0; c < analysis.size(); c++ )
	{
		clippedChannels[c] = false;
		double clip = maxVoltage * 0.95;
		for ( int k = 1; k < analysis[c].size(); k++ )
		{
			if ( analysis[c][k-1] > clip && analysis[c][k] > clip )
				clippedChannels[c] = true;
		}
		
		if ( clippedChannels[c] ) {
			analysis[c] = 0;
			cout << "Clipped channel: " << c << endl;
		}
	}
}


// detector class for spike detection
class Check
{
	public:
	Check( void ) {};
		
	int checkEvent
		(	SampleDataD::const_iterator first, SampleDataD::const_iterator last,
			SampleDataD::const_iterator event, LinearRange::const_iterator eventtime,
			SampleDataD::const_iterator index, LinearRange::const_iterator indextime,
			SampleDataD::const_iterator prevevent, LinearRange::const_iterator prevtime,
			EventData &outevents,
			double &threshold,
			double &minthresh, double &maxthresh,
			double &time, double &size, double &width
		)
	{
		time = *eventtime;
		return 1;
	}
};

// memory class for detection of spikes in voltage trace
class Peaks
{
	public:
	Peaks( void ) {};
	
	double peak;		// peak time
	double startT;		// start time
	double endT;		// end time
	double startA;		// value at start
	double endA;		// value at end
	int startI;			// index at start
	int endI;			// index at end
};

void ClearPeaks( SampleDataD &analysis, ArrayD &flatline) 
{
	///////////////////////// prepare data for peak detection
	double lp = 0.0002; // tau for low pass
	double tF = 25.; // threshold: factor for median multiplication
	
	// calculate square
	SampleDataD dataSquare = analysis;
	dataSquare *= dataSquare; 

	// calculate low pass of squared channel
	SampleDataD lpSquare;
	LowPassFilter(dataSquare, lpSquare, lp);

	// calculate threshold -- some x-fold of channel median
	SampleDataD thresLpSquare = lpSquare;
	
	// calculate median
	sort(thresLpSquare.begin(), thresLpSquare.end());
	double med = median(thresLpSquare);
	double threshold = tF*med;
	
	// save median and threshold
	thresLpSquare = med;
	thresLpSquare = threshold;

	///////////////////////// detect uncalled-for peaks in:
	//~ bool peaksDetected = false;
	Check check;
	EventData events( 1000, true ); // vector for the results; mit Groess
	Detector< SampleDataD::const_iterator, LinearRange::const_iterator > D;
	D.init( lpSquare.begin(), lpSquare.end(), lpSquare.rangeBegin() );
	D.peak( lpSquare.begin(), lpSquare.end(), events, threshold, threshold, threshold, 	check );
	//~ if ( events.size() > 0 )
		//~ peaksDetected = true;
	
	///////////////////////// define parameters of uncalled-for peaks:
	vector<Peaks> peaks;
	peaks.resize(events.size());
	for ( int k = 0; k < events.size(); k++ ) {
		peaks[k].peak = events[k];
		double peakI = lpSquare.index(peaks[k].peak);

			// find begin
			int m = peakI;
			while ( m >= 0 && lpSquare[m] > med )
				m--;
			peaks[k].startI = m;
			peaks[k].startT = analysis.pos(m);
			peaks[k].startA = analysis[m];
			
			// find end
			m = peakI;
			while ( m <= lpSquare.size() && lpSquare[m] > med )
				m++;
			peaks[k].endI = m;
			peaks[k].endT = analysis.pos(m);
			peaks[k].endA = analysis[m];
	}
	
	///////////////////////// cut those uncalled-for peaks:
	// peaks[].peak			// peak time
	// peaks[].startT		// start time
	// peaks[].endT			// end time
	// peaks[].startA		// value at start
	// peaks[].endA			// value at end
	// peaks[].startI		// index at start
	// peaks[].endI			// index at end

	for ( unsigned int j = 0; j < peaks.size(); j++ ) {
		for ( int i  = peaks[j].startI; i < peaks[j].endI; i++) {
			if ( i < flatline.size() )
				flatline[i] = 0.0;
		}
	}
}


void Process( ArrayD &flatline) 
{
	// int previous_start, previous_end,
	int current_start, current_end; //, next_start = 0;

	int i = 0;
	while ( i < flatline.size() )
	{
		// BROADEN ALL GAPS
		// FIND a gap
		while ( i < flatline.size() && flatline[i] != 0.0 )
			i++;
		current_start = i;
		if ( i+1 == flatline.size() )
			break;

		while ( i < flatline.size() && flatline[i] == 0.0 )
				i++;
		current_end = i;

		int broadenBy = (current_end - current_start);
		
		// BROADEN
		for ( int j = 0; j < broadenBy; j++ ) {
			if ( current_start - j > 0 )
				flatline[current_start - j] = 0.0;
		}
		
		for ( int j = 0; j < broadenBy; j++ ) {
			if ( current_start + j < flatline.size() )
				flatline[current_start + j] = 0.0;
		}
		
		current_start = current_end + 1;
	}

	// PUT A WINDOW IN THE GAP
	i = 0;
	while ( i < flatline.size() )
	{
		while ( i < flatline.size() && flatline[i] != 0.0 )
			i++;
		current_start = i;
		if ( i+1 == flatline.size() ) {
			break;
		}
		while ( i < flatline.size() && flatline[i] == 0.0 )
				i++;
		current_end = i;
		if ( current_end > flatline.size() )
			current_end = flatline.size();

		// PUT IN WINDOW
		int k = 0;
		int width = current_end - current_start;
		for ( int j = current_start; j < current_end; j++ ) 
		{
			// flatline[j] = abs(hanning(k,width)-1.0);
			// flatline[j] = abs(bartlett(k,width)-1.0);
			flatline[j] = abs(hanning(k,width)-1.0);
			k++;
		}
		
		current_start = current_end + 1;
	}
	
}

// Estimator for remaining time of program
class TimeEstimator
{
	public:
	TimeEstimator ( void ) { last = time(NULL); };
	
	// returns seconds
	double TimeRemaining( int now, int total )
	{
		double remaining = (total - now) * ( time(NULL) - last );
		last = time(NULL);
		return remaining;
	}
	
	// returns seconds
	double TimeRemaining( long long now, long long total )
	{
		double remaining = (total - now) * ( time(NULL) - last );
		cout << ( time(NULL) - last ) << endl;
		last = time(NULL);
		return remaining;
	}
	
	time_t last;
};

// CALCULATE TIME FROM INDEX AND SAMPLERATE
string timeDisplay( long long index, int samplerate, bool text)
{
	string timeString = "";
	int hours = index / (long long) samplerate /60 /60;
	if (hours >= 24)
	{
		int days = index / (long long) samplerate /60 /60 /24;
		hours = (index - (long long) days * (long long) samplerate *60 *60 *24) /(long long) samplerate /60 /60;
		int minutes = ((index - (long long) days * (long long) samplerate *60 *60*24) 
				- hours * (long long) samplerate *60 *60) /(long long) samplerate /60;
		int seconds = (((index - (long long) days * (long long) samplerate *60 *60*24) 
				- hours * (long long) samplerate *60 *60) 
				- minutes * (long long) samplerate *60)	/ (long long) samplerate;
		if (text)
			timeString +=  "dd:hh:mm:ss || ";
		timeString += Str(days,"%02d") + ":" + Str(hours,"%02d") + ":" + Str(minutes,"%02d") + ":" + Str(seconds,"%02d");
	}
	else
	{
		int minutes = (index - (long long) hours * (long long) samplerate *60 *60) /(long long) samplerate /60;
		int seconds = ((index - (long long) hours * (long long) samplerate *60 *60) - minutes * (long long) samplerate * 60) /(long long) samplerate;
		if (text)
			timeString =  "hh:mm:ss || ";
		timeString += Str(hours,"%02d") + ":" + Str(minutes,"%02d") + ":" + Str(seconds,"%02d");
	}
	return timeString;
}

// CALCULATE TIME FROM INDEX AND SAMPLERATE
string timeDisplay( int secs, bool text)
{
	string timeString = "";
	int hours = secs /60 /60;
	if (hours >= 24)
	{
		int days = secs /60 /60 /24;
		hours = (secs - days *60 *60 *24) /60 /60;
		int minutes = ((secs - days *60 *60*24) - hours *60 *60) /60;
		int seconds = (((secs - days *60 *60*24) - hours *60 *60) - minutes *60);
		if (text)
			timeString +=  "dd:hh:mm:ss || ";
		timeString += Str(days,"%02d") + ":" + Str(hours,"%02d") + ":" + Str(minutes,"%02d") + ":" + Str(seconds,"%02d");
	}
	else
	{
		int minutes = (secs - hours *60 *60) /60;
		int seconds = ((secs - hours *60 *60) - minutes *60);
		if (text)
			timeString =  "hh:mm:ss || ";
		timeString += Str(hours,"%02d") + ":" + Str(minutes,"%02d") + ":" + Str(seconds,"%02d");
	}
	return timeString;
}

// ##############################################################
// ##############################################################

void FftAll( const int windowSize, const int phase_windowSize, const int samplerate, vector<SampleDataD> &analysis, 
	SampleDataD &specsSum, vector<SampleDataD> &allSpecs, vector<SampleDataD> &allPhases, vector<SampleDataD> &allPhasesSpecs, const int i )
{
	// obtain PSD for every channel and create a sum-PSD for all channels
	SampleDataD spec( windowSize );
	allSpecs.resize(analysis.size());
	allPhases.resize(analysis.size());
	allPhasesSpecs.resize(analysis.size());
	for ( unsigned int c = 0; c < analysis.size(); c++ )
	{
		SampleDataD spec( windowSize );
		rPSD( analysis[c], spec, true , blackmanHarris );
		SampleDataD specPower = sqrt(spec);
		allSpecs[c] = specPower;
		if (c == 0)
			specsSum = specPower;
		else
			specsSum += specPower;
		
		//  calculate the phase spectrum in the center of the analysis window
		// int start_index = ( analysis[c].size() / 2 ) - ( windowSize / 2 );
		SampleDataD phases( windowSize );
		SampleDataD phases_spec( windowSize );
		SampleDataD trace_fragment( windowSize );
		//~ analysis[c].copy( analysis[c].pos( start_index ), analysis[c].pos( start_index + windowSize), trace_fragment ); // copy from middle of array
		analysis[c].copy( analysis[c].pos( 0 ), analysis[c].pos( windowSize), trace_fragment ); // copy from array-begin
		rPSD_phases( trace_fragment, phases_spec, phases, hanning );
		//~ phases.save("phases.dat", 20, 12);
		//~ phases_spec.save("phases_spec.dat", 20, 12);
		allPhases[c] = phases;
		allPhasesSpecs[c] = phases_spec;
		
	}	
	
	specsSum = specsSum / (double) analysis.size();
}

// ##############################################################
// ##############################################################


// THRESHOLDS FOR DETECTION:
// 1st ITERATION: e-6
// 2nd ITERATION e-7 or e-8

// GENERATE A LIST OF FREQUENCY-PEAKS ABOVE THE THRESHOLDS
void DetectFish( MapD &freqs, MapD &morefreqs, SampleDataD &specsSum, double thresholdUpper, double thresholdLower )
/*
freqs: to be filled up to be filled up with detected frequencies, 1st iteration
morefreqs: to be filled up with detected frequencies, 2nd iteration
specsSum: contains the summed PSD
sumWidth: range around peak to be summed up
fFishMin: lower frequency bound for detection of peaks
thresholdUpper: threshold for 1st iteration of frequency-peak finding
thresholdLower: threshold for 2nd iteration of frequency-peak finding
*/
{
	// DO DETECTION ON LOGARITHMIC SPECTRUM:
	// THIS IS MUCH BETTER FOR IGNORING SMALL SIDE PEAKS AND OTHER WEIRED STUFF!!!!
	SampleDataD logspecsum = specsSum;
	logspecsum.log10();
	//~ logspecsum.save("spec.dat", 20, 15);

	///////////////////////// DETECT PEAKS IN POWERSPECTRUM:
	double threshold = thresholdUpper;
	Check check;
	EventData freqpeaks( 1000, false ); // vector for the results; with size
	Detector< SampleDataD::const_iterator, LinearRange::const_iterator > D;
	D.init( logspecsum.begin() + logspecsum.index( fFishMin ), logspecsum.end(), logspecsum.rangeBegin() + logspecsum.index( fFishMin ) );
	if ( logspecsum.begin() + logspecsum.index( fMaxDetection ) < logspecsum.end() )
		D.peak( logspecsum.begin() + logspecsum.index( fFishMin ), logspecsum.begin() + logspecsum.index( fMaxDetection ), freqpeaks, threshold, threshold, threshold, check );
	else
		D.peak( logspecsum.begin() + logspecsum.index( fFishMin ), logspecsum.end(), freqpeaks, threshold, threshold, threshold, check );
	
	freqs.reserve( freqpeaks.size() );
	freqs.clear();
	// transfer data to a MapD and get peak amplitude
	for ( int k = 0; k < freqpeaks.size(); k++ )
	// sum up around peak in spectrum
	{
		int peakIndex = specsSum.index( freqpeaks[k] );
		double peakSum = 0.0;
		for ( int j = - sumLeftRight; j <= sumLeftRight; j++ )
		{
			if (peakIndex + j >= 0 && peakIndex + j <= specsSum.size() )
				peakSum += specsSum[ peakIndex + j ];
		}
		freqs.push( freqpeaks[ k ], peakSum );
	}
	
	///////////////////////// DETECT MORE PEAKS IN POWERSPECTRUM:
	threshold = thresholdLower;
	freqpeaks.clear();
	D.init( logspecsum.begin() + logspecsum.index( fFishMin ), logspecsum.end(), logspecsum.rangeBegin() + logspecsum.index( fFishMin ) );
	if ( logspecsum.begin() + logspecsum.index( fMaxDetection ) < logspecsum.end() )
		D.peak( logspecsum.begin() + logspecsum.index( fFishMin ), logspecsum.begin() + logspecsum.index( fMaxDetection ), freqpeaks, threshold, threshold, threshold, check );
	else
		D.peak( logspecsum.begin() + logspecsum.index( fFishMin ), logspecsum.end(), freqpeaks, threshold, threshold, threshold, check );

	morefreqs.reserve( freqpeaks.size() );
	morefreqs.clear();
	// transfer data to a MapD and get peak amplitude
	for ( int k = 0; k < freqpeaks.size(); k++ )
	// SUM UP AROUND PEAK IN SPECTRUM
	{
		int peakIndex = specsSum.index( freqpeaks[k] );
		double peakSum = 0.0;
		for ( int j = - sumLeftRight; j <= sumLeftRight; j++ )
		{
			if (peakIndex + j >= 0 && peakIndex + j <= specsSum.size() )
				peakSum += specsSum[ peakIndex + j ];
		}
		morefreqs.push( freqpeaks[ k ], peakSum );
	}
	
	cout << morefreqs.size() << " weak, and " << freqs.size() << " strong frequencies found:\n";
	for ( int g = 0; g < freqs.size(); g++ )
		cout << freqs.x(g) << '\n';
	cout << '\n';

}

// ##############################################################

// AN ALTERNATIVE WAY TO EXTRACT A GROUP OF FUNDAMENTAL WITH HARMONICS
MapD FindFrequencyGroupAlternative ( MapD &freqs, MapD &morefreqs, double fTol )
{
	// start at the strongest frequency
	double fMax = freqs( 0, freqs.y().maxIndex() );

	//~ cout << "\t\t fMAX: " << fMax << endl;
	
	ArrayI bestGroup;
	bestGroup.reserve( 100 );
	
	ArrayI bestMoreGroup;
	bestMoreGroup.reserve( 100 );
	
	double bestGroupPeakSum = 0.0;
	int bestDivisor = 0;

	// SEARCH FOR THE REST OF THE FREQUENCY GROUP
	// start with the strongest fundamental and try to gather the full group of available harmonics
	
	// EXPLAIN
	// In order to find the fundamental frequency of a fish harmonic group, we devide fMax (the strongest frequency in the spectrum)
	// by a range of integer divisors.
	// We do this, because fMax could just be a strong harmonic of the harmonic group
	for ( int i = 1; i <= maxdivisor; i++ )
	{
		ArrayI newGroup;
		newGroup.reserve( 100 );
		double newGroupPeakSum = 0.0;
		
		// here, we artificially raise the threshold for better following groups by doubling power the first frequency
		// it works, somehow, but it's not perfect ....
		//~ if ( i == 1 )
			//~ newGroupPeakSum += freqs[ freqs.y().maxIndex() ]; // add the power of fMax itself
		
		double fZero = fMax / (double)(i); // here, we define the hypothesized fundamental, which we compare to all higher frequencies
		
		if ( i > 1 && fZero < fFishMin ) // fZero is not allowed to be smaller than our chosen minimum frequency
			break;
		
		// SEARCH ALL DETECTED FREQUENCIES
      for ( int j=0; j < freqs.size(); j++ ) 
		{
			// IS FREQUENCY A A INTEGRAL MULTIPLE OF FREQUENCY B?
			double nf = freqs.x( j ) / fZero; // devide the frequency-to-be-checked with fZero: what is the multiplication factor between freq and fZero?
			int n = round( nf ); // round to integer value
			
			// !! the difference between the detection, devided by the derived integer, and fZero should be very very small: 1 resolution step of the fft
			// (freqs.x( j ) / n) = should be fZero, plus minus a little tolerance, which is the fft resolution
			double nd = fabs( (freqs.x( j ) / n) - fZero );
			// ... compare it to our tolerance
			if ( n>0 && nd <= fTol / i )
			{
					newGroup.push( j );
					newGroupPeakSum += freqs[j];
			}
		}

		ArrayI newMoreGroup;
		newMoreGroup.reserve( 100 );
		double newMoreGroupPeakSum = 0.0;
		int upperZeros = 0; // count  non-existent freqs upstream of fmax
		// SEARCH ALL DETECTED FREQUENCIES in morefreqs
      for ( int j=0; j < morefreqs.size(); j++ ) 
		{
			// IS FREQUENCY A A INTEGRAL MULTIPLE OF FREQUENCY B?
			double nf = morefreqs.x( j ) / fZero; // devide the frequency-to-be-checked with fZero: what is the multiplication factor between freq and fZero?
			int n = round( nf ); // round to integer value
			
			// !! the difference between the detection, devided by the derived integer, and fZero should be very very small: 1 resolution step of the fft
			// (morefreqs.x( j ) / n) = should be fZero, plus minus a little tolerance, which is the fft resolution
			double nd = fabs( (morefreqs.x( j ) / n) - fZero );
			// ... compare it to our tolerance
			if ( n>0 && nd <= fTol / i )
			{
				bool tooManyZeros = false;
				while ( newMoreGroup.size() < n-1 )
				{
					newMoreGroup.push( -1 ); // marker for non-existent harmonic
					
					// TOO MANY FILL-INS?
					if ( j > i ) // upstream of fMax ...
					{
						upperZeros++; // count fill-ins
						if ( upperZeros > 2 ) // if more than 2 in a row ...
						{
							tooManyZeros = true;
							while ( upperZeros > 0 ) // erase fill ins and break
							{
								newMoreGroup.erase( newMoreGroup.size() -1 ); // erase marker for non-existent harmonic
								upperZeros--;
							}
							break;
						}
					}
				}
				if ( tooManyZeros ) // stop searching for more harmonics...
					break;
				
				newMoreGroup.push( j );
				upperZeros = 0;
				newMoreGroupPeakSum += morefreqs[j];
			}
		}
		
		// RECALCULATE THE PEAKSUM, GIVEN THE UPPER LIMIT DERIVED FROM morefreqs
		newGroupPeakSum = 0.0;
		for ( int j = 0; j < newGroup.size() && freqs.x( newGroup[j] ) <= newMoreGroup.size()*fZero; j++ )
			newGroupPeakSum+= freqs.y( newGroup[j] );
		
		// CHECK FOR: with too low deviders, certain patterns of zeros will show up.
		bool noPatterns = true;
		double patternThreshold = 0.5;  // means, 50% of the expected zeros are found ...
		double divAvg = 0.0;
		if ( i > 1 )
		{
			for ( int k = 0; k < newMoreGroup.size(); k++ )
			{
				if ( (k+1) % i != 0.0 ) // if the remainder of the devision k / i is not zero ...
				{
					if ( newMoreGroup[k] == -1 )
						divAvg = ( 1.0 + (k*divAvg) ) / (double) (k+1);
					else
						divAvg = ( 0.0 + (k*divAvg) ) / (double) (k+1);
				}
			}
			if ( divAvg >= patternThreshold )
				noPatterns = false;
		}

		//~ cout << i << " " << newGroupPeakSum << " " << newGroup.size() << " \t f = " << fZero;
		//~ cout << "  noPatterns: " << noPatterns << "  divAvg: " << divAvg << endl;
		
      // FINAL CHECKS:
      if ( bestDivisor == 0 || 
			(	newGroup.size() > minimumGroupSize
				&& noPatterns
				&& newGroupPeakSum > bestGroupPeakSum ) ) // has to be 10% better than the old one
		{
			bestGroupPeakSum = newGroupPeakSum;
			bestGroup = newGroup;
			bestMoreGroup = newMoreGroup;
			bestDivisor = i;
      }
	}

	//~ cout << endl;
	
	// HARMONICS WILL BE DERIVED FROM newMoreGroup
	
	//~ cout << "Best Group:\n";
	//~ for ( int i = 0; i < bestGroup.size(); i++)
		//~ cout << "\t" << freqs.x(bestGroup[i]) << " " << freqs[bestGroup[i]]<< endl;
	//~ cout << endl;
	
	// ##############################################################
	
	MapD group;
	group.reserve( bestMoreGroup.size() );
	
	double fZero = fMax / (double)(bestDivisor);
	//~ cout << "Divider " << bestDivisor << " -> fZero=" << fZero << "Hz\n";
	
	// fill up group and erase from morefreqs
	int c = 0;
	for ( int i = 0; i < bestMoreGroup.size(); i++ )
	{
		if (bestMoreGroup[i] >= 0 )
		{
			group.push( morefreqs.x(bestMoreGroup[i]-c), morefreqs.y(bestMoreGroup[i]-c) );
			morefreqs.erase( bestMoreGroup[i]-c );
			c++;
		}
		else
		{
			group.push( fZero*(i+1) , 0.0 );
		}
	}
	
	// erase from freqs
	for ( int i = 0; i < bestGroup.size(); i++ )
		freqs.erase( bestGroup[i]-i );

	//~ cout << "\tCurrent Group:\n";
	//~ for ( int i = 0; i < group.size(); i++)
		//~ cout << "\t" << group.x(i) << " " << group[i]<< endl;
	//~ cout << endl;
	
  return group;
}


// ##############################################################

// CONNECT FREQUENCY AND LISTED FISH
vector<MapD> FishFinderPlus( MapD freqs, MapD morefreqs, SampleDataD &specs, double fTol )
{
	// !! here, freqs is a copy, not a reference. we'll be able to re-search on these frequencies later, if necessary.
	MapD backupMoreFreqs( morefreqs) ;
	
	// EXTRACT GROUP OF BIG HARMONICS:
	vector<MapD> groupList;
	while ( freqs.size() > 0 )
	{
		// EXTRACT A GROUP OF FUNDAMENTAL WITH HARMONICS
		//~ MapD group = FindFrequencyGroupPlus ( freqs, morefreqs, fTol );
		MapD group = FindFrequencyGroupAlternative ( freqs, morefreqs, fTol );

	// ##############################################################	

		// PERFORM SOME CHECKS
		// a group should consist of more than one frequency
		// and the fundamental should be within the limits of the search window, e.g. from 40 Hz to 1300 Hz or so
		
		// is any frequency in the group bigger than our absolute threshold?
		bool absThreshOK = false;
		for ( int i = 0; i < group.size(); i++ )
		{
			if ( group.y(i) > absolute_threshold )
				absThreshOK = true;
		}
		
		// count all frequencies bigger than zero ( = those frequencies, which have been detected and are not fill-ups)
		int biggerZero = 0;
		for ( int i = 0; i < group.size(); i++ )
		{
			if ( group.y(i)  > 0.0 )
				biggerZero++;
		}
		
		// check against group size and minimum frequency and maximum frequency ...
      if (	absThreshOK && 					// power is high enough
				//~ biggerZero > 2 && 				// at least 3 detected frequencies in the group
				biggerZero > minimumGroupSize && 				// at least n detected frequencies in the group
				group.x(0) > fFishMin && 	// fundamental bigger than minimum frequency
				group.x(0) < fFishMax )		// fundamental smaller than maximum frequency
		{
			groupList.push_back( group );
			//~ cout << "extracted Group " << groupList.size()-1 << ":" << endl;
			//~ for ( int j = 0; j < group.size(); j++ )
				//~ cout << group( 0, j ) << " \t" << group[j] << endl;
			//~ cout << endl;
		}
	}
	
	// ##############################################################
	
	//~ cout << endl;
	sort(groupList.begin(), groupList.end(), sort_using_greater_than_for_Maps );
	//~ for ( int g = 0; g < (int) groupList.size(); g++ ) 
	//~ {	
		//~ cout << "Group No. " << g << " -- Fundamental: " << Str( groupList[g](0,0), "%.2f" ) << " -- ";
		//~ for ( int k = 1; k < groupList[g].size(); k++ ) 
			//~ cout << ", " << Str( groupList[g](0,k), "%.2f" );
		//~ cout << endl;
		
	//~ }
	
	// ##############################################################	
	
	// CHECK IF SOME OF THE DETECTIONS ARE ACTUALLY HARMONICS OF A DETECTED FUNDAMENTAL
	// FOR ALL FUNDAMENTALS IN THE LIST
	//~ cout << endl;
	// !! SORT !!
	sort(groupList.begin(), groupList.end(), sort_using_greater_than_for_Maps );
	for ( unsigned int i = 0; i < groupList.size(); i++ )
	{
		// FOR ALL HIGHER FUNDAMENTALS IN THE LIST ...
		for ( unsigned int j = i+1; j < groupList.size(); j++ )
		{
			// FOR A RANGE OF DIVISORS, check the lower frequency against the thigher ones// do not check frequencies below fFishMin
			for ( int k = 1; ( k <= maxMergeDivisor ) && ( groupList[i].x( 0 ) / k  > fFishMin ) ; k ++ )
			{
				double nf = groupList[j].x( 0 ) / (groupList[i].x( 0 ) / k); // devide higher fundamentals by a fraction of  the current fundamental
				int n = round( nf ); // round to integer value
				double nd = fabs( (groupList[j].x( 0 ) / n) - (groupList[i].x( 0 ) / k) );
				
				// IF A MULTIPLE OF A LOWER FUNDAMENTAL IS FOUND
				if ( n>0 && n <= maxMergeDivisor && nd <= fTol / k )
				{
					double fZero = groupList[i].x( 0 ) / k; // the true fundamental
					cout << "Doublet detected and erased: " << groupList[i].x( 0 ) << " vs. " << groupList[j].x( 0 ) << endl;
					cout << "The true fundamental is: " << fZero << endl;
					
					// ##############################################################
					
					MapD newMoreGroup;
					newMoreGroup.reserve( 100 );
					int upperZeros = 0; // count  non-existent freqs upstream of fmax
					// SEARCH ALL DETECTED FREQUENCIES in morefreqs
					for ( int u=0; u < morefreqs.size(); u++ ) 
					{
						// IS FREQUENCY A A INTEGRAL MULTIPLE OF FREQUENCY B?
						double nf = morefreqs.x( u ) / fZero; // devide the frequency-to-be-checked with fZero: what is the multiplication factor between freq and fZero?
						int n = round( nf ); // round to integer value
						
						// !! the difference between the detection, devided by the derived integer, and fZero should be very very small: 1 resolution step of the fft
						// (morefreqs.x( u ) / n) = should be fZero, plus minus a little tolerance, which is the fft resolution
						double nd = fabs( (morefreqs.x( u ) / n) - fZero );
						// ... compare it to our tolerance
						if ( n>0 && nd <= fTol / k )
						{
							bool tooManyZeros = false;
							while ( newMoreGroup.size() < n-1 )
							{
								// FILL IN ...
								newMoreGroup.push( (newMoreGroup.size()+1)*fZero  , 0.0 ); // marker for non-existent harmonic
								
								// TOO MANY FILL-INS?
								if ( u > k ) // upstream of fMax ...
								{
									upperZeros++; // count fill-ins
									if ( upperZeros > 2 ) // if more than 2 in a row ...
									{
										tooManyZeros = true;
										while ( upperZeros > 0 ) // erase fill ins and break
										{
											newMoreGroup.erase( newMoreGroup.size() -1 ); // erase marker for non-existent harmonic
											upperZeros--;
										}
										break;
									}
								}
							}
							if ( tooManyZeros ) // stop searching for more harmonics...
								break;
							
							newMoreGroup.push( morefreqs.x(u), morefreqs.y(u) );
							upperZeros = 0;
						}
					}
					
					// CHECK FOR: with too low deviders, certain patterns of zeros will show up.
					bool noPatterns = true;
					double patternThreshold = 0.5;  // means, 50% of the expected zeros are found ...
					double divAvg = 0.0;
					if ( i > 1 )
					{
						for ( int k = 0; k < newMoreGroup.size(); k++ )
						{
							if ( (k+1) % i != 0.0 ) // if the remainder of the devision k / i is not zero ...
							{
								if ( newMoreGroup[k] == -1 )
									divAvg = ( 1.0 + (k*divAvg) ) / (double) (k+1);
								else
									divAvg = ( 0.0 + (k*divAvg) ) / (double) (k+1);
							}
						}
						if ( divAvg >= patternThreshold )
							noPatterns = false;
					}
					
					cout << "noPatterns: " << noPatterns << " divAvg: " << divAvg << endl;
					
					// ##############################################################
					
					// HOUSEKEEPING
					if ( newMoreGroup.size() > 0 && noPatterns ) // still, we could be wrong. Check if the new group found something
					{
						groupList.erase( groupList.begin() + j ); // delete the detected multiple
						groupList.erase( groupList.begin() + i ); // delete the detected multiple
						groupList.push_back( newMoreGroup ); // add the merged group
						i = 0; // rewind the counter
						j = groupList.size(); // rewind the counter
						k = maxMergeDivisor; // reset
						sort(groupList.begin(), groupList.end(), sort_using_greater_than_for_Maps );
					}
					else
						cout << "BUT: new group is BAD" << endl;
					
					//~ cout << "Press key to continue ..." << endl;
					//~ PressAnyKey();									
				}
			}
		}
	}
	//~ cout << endl;
	
	// SHOW
	cout << "Found Fundamentals:" << endl;
	for ( unsigned int i = 0; i < groupList.size(); i++ )
		cout << Str( groupList[i](0,0) , 8, 3, 'f' ) << " " << groupList[i][0] << endl;	
	
	return groupList;
}


void PhaseFinder( vector<MapD> &groupList, SampleDataD &spec, 
	SampleDataD &phase, SampleDataD &phaseSpec, 
	vector<MapD> &groupListPhases, vector<MapD> &groupListPhasesPower )
{
	groupListPhases.reserve( groupList.size() );
	groupListPhasesPower.reserve( groupList.size() );
	
	// extract the phases of the detected frequency-peaks
	for ( unsigned int i = 0; i < groupList.size(); i++ )
	{
		// ???: extract phases of all frequencies? if so: how do i check if the trace is clipping?
		
		// extract phases of all frequencies; some of them might have no power!
		MapD groupPhases;
		groupPhases.reserve( groupList[i].size() );
		MapD groupPhasesPower;
		groupPhasesPower.reserve( groupList[i].size() );
		
		for ( int k = 0; k < groupList[i].size(); k++ )
		{
			// check if the frequency in question has really been detected, 
			// i.e. if it has a power greater than zero
			// if not: assign it a phase of -0.0
			double current_phase = -0.0;
			double current_power = -0.0;
			if ( ! groupList[i].y(k) == 0.0 )
			{
				// 1) prepare search for power in phase-spectrum
				int power_peak_index = spec.index( groupList[i].x(k) );
				double lower_search_bound = spec.pos( power_peak_index - 1 );
				double upper_search_bound = spec.pos( power_peak_index + 1 );
				// 2) search for power-peak in phase-spectrum
				int power_peak_phasespec_index = phaseSpec.maxIndex( lower_search_bound, upper_search_bound );
				// if ( ! power_peak_phasespec_index == power_peak_index )
					// cout << "Indices of Peaks in power-spectrum and phase-spectrum do not match." << endl;
				// 3) extract phase of power-peak in phase-spectrum
				current_phase = phase[ power_peak_phasespec_index ];
				current_power = phaseSpec[ power_peak_phasespec_index ];
			}
			
			// make sure the pase is set between +/- Pi
			current_phase -= sign( current_phase ) * floor( ( M_PI + abs( current_phase ) ) / (M_PI*2.) ) * 2.*M_PI;
			
			groupPhases.push( groupList[i].x(k), current_phase );
			groupPhasesPower.push( groupList[i].x(k), current_power );
		}
		
		groupListPhases.push_back( groupPhases );
		groupListPhasesPower.push_back( groupPhasesPower );
	}
}


// ##############################################################
// ##############################################################

int main ( int argc, char *argv[] ) 
{
	// ##############################################################

	cout << "\n" << endl;
	
	// expected arguments:
	int expected_arguments = 2; // input_directory, output_directory
	
	if ( ! argc == 1+expected_arguments )
		cout << "program takes two arguments: 1) input folder, 2)output folder" << endl;
	
	bool input_folder_ok = true;
	for ( int i_args = 1; i_args < argc; i_args++ )
	{
		cout << "argument " << i_args << ": " << argv[i_args] << endl;
		if ( ! FolderExists( argv[i_args] ) )
		{
			cout << "\tFolder does not exist." << endl;
			input_folder_ok = false;
		}
	}
	if ( ! input_folder_ok )
		return 0;
	
    // CREATE ABSOLUTE PATHS
    char resolved_path[100];
    realpath(argv[0], resolved_path);
    string program_name( resolved_path );
    cout << "program name: " << program_name << endl;
    realpath(argv[1], resolved_path);
    string inDir( resolved_path);
    inDir += "/";
    realpath(argv[2], resolved_path);
	string outDir( resolved_path );
    outDir += "/";
    
    cout << "Input directory: " << inDir << endl;
    cout << "Output directory: " << outDir << endl;
	
	cout << "\n" << endl;
	
	string traceName = "traces-grid1.raw";
	string inFile = inDir + traceName;
	string configFileName = "fishgrid.cfg";
	
	// ##############################################################
	
	// INITIALIZE PARAMETERS
	if (simulated_data)
	{
		absolute_threshold = sim_absolute_threshold;
		thresholdUpper = sim_thresholdUpper;
		thresholdLower = sim_thresholdLower;
		minimumGroupSize  = sim_minimumGroupSize;
	}

	// ##############################################################

	Options cfg;
	cfg.addNumber( "Columns1", "",  0 );
	cfg.addNumber( "Rows1", "",  0 );
	cfg.addNumber( "ColumnDistance1", "",  0 );
	cfg.addNumber( "RowDistance1", "",  0 );
	cfg.addNumber( "AISampleRate", "",  0 );
	cfg.addNumber( "AIMaxVolt", "",  0 );
	cfg.addNumber( "ChannelOffset", "",  0 );
	
	// read config file
	{
		ifstream f( string( inDir +configFileName ).c_str() );
		// first, check parameter file in inpath
		if ( f.good() ) {
			cfg.read( f );
			cout << "cfg read from " << inDir+configFileName << endl;
		}
		else
		{
			cout << "Config does not exist." << endl;
			return -1;
		}
	}
	
	int  columns  = (int) cfg.number( "Columns1" );
	int  rows = (int) cfg.number( "Rows1" );
	int  colDistance = (int) cfg.number( "ColumnDistance1" );
	int  rowDistance = (int) cfg.number( "RowDistance1" );
	int  samplerate = (int) cfg.number( "AISampleRate" ) * 1000;
	int  maxVoltage = (int) cfg.number( "AIMaxVolt" ) ; // [mv]
	int  channelOffset = (int) cfg.number( "ChannelOffset" ) ; // [mv]
	int channels = rows*columns;
	
	
	// GET CFG INFORMATION ABOUT ACQUISITION -- !! TOTALLY NOT PERU-DATA COMPATIBLE !!
	Options acquisition;
	// read config file, again...
	{
		ifstream f( string( inDir +configFileName ).c_str() );
		// first, check parameter file in inpath
		if ( f.good() ) {
			string line;
			while (getline(f, line))
			{
				if ( line == "*Acquisition" ) {
					acquisition.load( f );
					break;
				}
			}
		}
	}
	
	// blacklist
	Str blacklist = acquisition.text( "blacklist1");
	vector<int> blackchannels;
	blacklist.range( blackchannels, ",", "-" );
	for ( unsigned int k=0; k<blackchannels.size(); k++ )
	cerr << "blackchannel " << blackchannels[k] << '\n';
	int channels_card_1 = 32-blackchannels.size(); // number of black channels on card 1
	// sample offsets
	int sampleOffset_1 = 0;
	int sampleOffset_2 = acquisition.integer( "offset2", 0 );
	int sampleOffset = abs(sampleOffset_2);
	
	// set offsets right
	if ( sampleOffset_2 < 0 )
	{
		sampleOffset_1 = abs( sampleOffset_2 );
		sampleOffset_2 = 0;
	}
	
	// ##############################################################
	// ##############################################################

	// DATEILESEN VORBEREITEN
	ifstream traceFile;
	traceFile.open( inFile.c_str(), ios::in | ios::binary );
	if ( !traceFile.good() ) {
		cerr << "data file does not exist." << endl;
		cerr << inFile << endl;
		return 1;
	}
	traceFile.clear();
	// DATEILAENGE
	traceFile.seekg( 0, ios::end );
	long long traceSize = traceFile.tellg()/sizeof(float); // Anzahl der Elemente in der Datei
	long long traceSamples = traceSize / channels; // Anzahl der Samples pro Kanal
	
	// INDEX ZUM LESEN -- EIN GEMULTIPLEXTES DATENSAMPLE ALLER KANÄLE GILT HIER ALS EIN ELEMENT
	long long maxRead = traceSamples; // Limit fuer die Analyse

	// ##############################################################
	// ##############################################################
	
	// BERECHNE DIE LIMITS DER ANALYSE
	long long index = 0; 
	if ( ! tOff == 0 )
		 index = (long long) tOff * (long long) ( samplerate );

	if ( ! tLength == 0)
		maxRead = index +  (long long) tLength * (long long) ( samplerate );

	// ##############################################################
	// ##############################################################	
	
	/*
	double analysisWindowSize = minAnalysisWindowSize;
	
	// now, adjust the number of fftWindows within the analysis window
	int k = fftWindows;
	while ( subWindowSize+(subWindowSize/2*k) < analysisWindowSize )
		k++;
	fftWindows	= k+1;
	// now, adjust the analysisWindowSize to fit the number of subWindows
	analysisWindowSize = subWindowSize/2.0 + (fftWindows)*subWindowSize/2.0;
	
	// determine the size of the fft window based on the analysis window size and how often the fft window should fit the analysis window
	double p = 0.0;
	while ( ( analysisWindowSize * samplerate   /  relacs::pow( 2.0, p ) ) >  fftWindows )
	{
		p += 1.0;
	}
	int fftWindowSize = (int) relacs::pow(2.0, p);
	*/
	
	// DETERMINE THE NUMBER OF DATA POINTS IN THE ANALYSIS WINDOW
	int windowElements = fftWindowSize + (fftWindows-1)*fftWindowSize/2; // consider overlap of analysis windows
	double analysisWindowSize = (double) windowElements / samplerate;
	int phase_windowSize = fftWindowSize; // for calculation of phases in fft
	// DETERMINE THE STEPSIZE
	double stepsize = analysisWindowSize*(1.0-overlap); // in Sekunden
	
	// the spectrum prints will be prepared for a certain time span, defined by "spectrumLength" 
	int spectrumSteps = (int) (spectrumLength / stepsize);
	
	// DETERMINE FFT RESOLUTION
	// create dummy vectors for input and output
	SampleDataD outSpec( fftWindowSize );
	SampleDataD inData;
		inData.resize ( windowElements, 0.0, 1.0 / (double) samplerate, 0.0F );
	// calculate dummy psd
	rPSD( inData, outSpec, true , hanning );
	double fftResolution = outSpec.stepsize(); // get fft-resolution from result spectrum
	double peakSumWidth = (2*sumLeftRight)*fftResolution;
	
	// ##############################################################

	cout << endl;
	cout << "This is specView -- it offers a first glance on fishgrid data in a time-spectogram-view" << endl;
	cout << endl;

	cout << "\n\n" << endl;
	cout << "Columns1: " << columns << endl;
	cout << "Rows1:" << rows << endl;
	cout << "Channels: " << channels << endl;
	cout << "ColumnDistance1:" << colDistance << " cm" << endl;
	cout << "RowDistance1:" << rowDistance << " cm" << endl;
	cout << "AISampleRate:" << samplerate / 1000. << " KHz" << endl;
	cout << "maxVoltage:" << maxVoltage << " mV" << endl;
	cout << "CHANNEL-OFFSET:" << channelOffset << " mV" << endl;
	cout << "SAMPLE OFFSETS:  #2:" << sampleOffset_2 << endl;
	cout << endl;
	cout << "Analysis Window Size: " << Str( analysisWindowSize, "%.2f" ) << " seconds" << endl;
	cout << "fft Window Size: " << fftWindowSize << endl;
	cout << "fft-resolution: " << fftResolution << " [Hz]" << endl;
	cout << "summing over spectrum: " << peakSumWidth << " [Hz]" << endl;
	cout << "Number of FFT Windows: " << fftWindows << endl;
	cout << "Window Elements: " << windowElements << endl;
	cout << "Spectrum-Steps " << spectrumSteps  << endl;	
	cout << "Stepsize: " << Str( stepsize, "%.2f" ) << " seconds" << endl;
	cout << endl;
	cout << "Channels of IO card #1 used: " << channels_card_1 << endl;
	cout << "Sample Offset 1: " << sampleOffset_1 << endl;
	cout << "Sample Offset 2: " << sampleOffset_2 << endl;
	
	cout << "\n" << endl;
	
	// ##############################################################
	// ##############################################################

	//~ cout << "samples in file: " << Str( traceSize, "%.1f") << endl;
	//~ cout << "Bytes: " << Str( traceSize * sizeof(float), "%.1f") << endl;
	cout << "Gigabytes: " << Str( traceSize * sizeof(float) /1e9, "%.1f") << endl;
	cout << endl;	
	cout << "multiplexed-elements in file: " << traceSamples << endl;
	cout << endl;
	
	// TOTAL RECORDING TIME
	cout << timeDisplay( traceSamples, samplerate, true ) << endl;
	
	cout << endl;
	cout << "samples to analyze: " << maxRead - index << endl;
	cout << endl;

	// BUFFER VORBEREITEN: ANALYSIS UND STORAGE
	vector< SampleDataD > analysis;
	analysis.resize( channels );	
	for ( int c=0; c<channels; c++ ) 
	{
		analysis[c].resize ( windowElements, 0.0, 1.0 / (double) samplerate, 0.0F ); // length, offset, stepsize, initial value
	}
	
	cout << "PRESS ANY KEY TO CONTINUE" << endl;
	cout << endl;
	PressAnyKey();

	// ##############################################################
	// ##############################################################
	
	// CHECK FOR RESULTS DIRECTORY
	// given the result directory is not present ... make it
	if (! FolderExists( (outDir + "results/").c_str() ))
	{
		string makeDir = "mkdir " + outDir + "results/";
		system ( makeDir.c_str() );
	}
	
	if (!debug)
	{
		// given the target directory is present ... remove it and remake it
		if ( FolderExists( (outDir + "results/" + resultDirName).c_str() ))
		{
			string removeDir = "rm -rf " + outDir + "results/" + resultDirName;
			system ( removeDir.c_str() );
			string makeDir = "mkdir " + outDir + "results/" + resultDirName;
			system ( makeDir.c_str() );
		}
		else // ... just make it
		{
			string makeDir = "mkdir " + outDir + "results/" + resultDirName;
			system ( makeDir.c_str() );
		}
	}
		
	// ##############################################################
	// ##############################################################

	// WRITE METADATA
	if (!debug)
	{
		cout << "\nWriting metadata of analysis ... ";
		string filename = outDir + "results/" + resultDirName + metaData;
		fstream metadata;
		metadata.open( filename.c_str() ,ios::out);
		metadata << "Input Directory: " << inDir << "\n";
		metadata << "Analysis Window Size: " << Str( analysisWindowSize, "%.2f" ) << " [s]" << "\n";
		metadata << "fft Window Size: " << fftWindowSize << "\n";
		metadata << "fft-resolution: " << fftResolution << " [Hz]" << "\n";
		cout << "summing over spectrum: " << Str( peakSumWidth, "%.2f" ) << " [Hz]" << endl;
		metadata << "Number of FFT Windows: " << fftWindows << "\n";
		metadata << "Window Elements: " << windowElements << "\n";
		metadata << "Spectrum-Steps: " << spectrumSteps  << "\n";	
		metadata << "Stepsize: " << Str( stepsize, "%.2f" ) << " [s]" << "\n";
		metadata << "analysis program: " << program_name  << "\n" << endl;
		metadata.close();
		cout << "Done.\n" << endl;
	}
	
    // COPY PROGRAM AND SOURCECODE IN OUTPUT DIRECTORY
    if ( ! FolderExists( outDir + "/analyzer") )
    {
        cout << "Creating analyzer-folder." << endl;
        string makeDir = "mkdir " + outDir + "analyzer";
        system ( makeDir.c_str() );
    }
    
    if ( FolderExists( outDir + "/analyzer") )
    {
        cout << "Copy analysis program to analyzer-folder." << endl;
        string copy_string = "rsync -a  " + program_name + " " + outDir + "analyzer/";
        system ( copy_string.c_str() );
        copy_string = "rsync -a  " + program_name + ".cc " + outDir + "analyzer/";
        system ( copy_string.c_str() );
        cout << endl;
    }
    
	// ##############################################################
	// ##############################################################

	int elements = (windowElements+sampleOffset)*channels; // Gesamt-Anzahl der Elemente die in jedem Durchlauf gelesen werden
	
	long long maxSteps = -1 + (maxRead - index) / (long long) (stepsize * samplerate); // Anzahl der Analyse-Schritte
	long long stepCounter = 1;

	// ##############################################################
	// ##############################################################
	
	// MAIN LOOP
	for ( ; index < maxRead &&  index < traceSamples-elements/channels; index += (long long) (stepsize * samplerate) )
	// solange weder Analyse-Limit noch Dateiende erreicht sind ...
	{
		//~ cout << index << " " << maxRead << " " << index + (long long) (channels * stepsize * samplerate) << endl;
		// INFORMATIONEN 
		cout << stepCounter << " of " << maxSteps << endl;
			cout << timeDisplay( index, samplerate, false ) << endl;
		//~ cout << "Time Remaining: " << timeDisplay( (int) timeEst.TimeRemaining( j, maxSteps ), false ) << endl;
		
		stepCounter++;
		
	// ##############################################################
	// ##############################################################

	// READ DATA
		
		cout << "Reading ..." << endl;
		float  *buffer = new float[ elements*sizeof( float ) ];
		// GO TO INDEX
		traceFile.seekg( (index* (long long) channels + channelOffset) * (long long) sizeof( float ) );
		// READ FRAGMENT
		traceFile.read( (char *)buffer, elements*sizeof( float ) );
		// TRANSFER DATA TO SAMPLEDATA
		for ( int c=0; c<channels; c++ ) 
		{
			analysis[c].clear();  // clear old contents
		}
		int inx = 0;
		while ( inx < windowElements*channels )
		{
			for ( int c=0; c<channels; c++ ) 
			{
				if ( sampleOffset_1 == 0 )
				{
					if ( c < channels_card_1)
						analysis[c].push( 1000.0*buffer[ inx++ ] );  // transfer and convert to millivolt
					else
						analysis[c].push( 1000.0*buffer[ sampleOffset_2*channels + inx++ ] );  // transfer and convert to millivolt
				}
				else
				{
					if ( c < channels_card_1)
					{
						//~ cout << "inx: " << sampleOffset_1 + inx++ << endl;
						analysis[c].push( 1000.0*buffer[ sampleOffset_1*channels + inx++ ] );  // transfer and convert to millivolt
					}
					else
					{
						//~ cout << "inx: " << sampleOffset_1 + inx++ << endl;
						analysis[c].push( 1000.0*buffer[ inx++ ] );  // transfer and convert to millivolt					
					}
				}
			}
		}
		delete [] buffer;

		// ##############################################################
		
		cout << "Processing ..." << endl;
		// PRE-PROCESSING
		// substract channel mean
		for ( int c=0; c<channels; c++ )
			analysis[c]-= mean( analysis[c] );
		
		// debug:
		//~ for ( int c=0; c<channels; c++ )
			//~ analysis[c].save( "trace_" + Str( c+1, "%02d" ) + ".dat" );
		//~ return 0;
		
		// check for clipping in voltage traces
		//~ Clipping( analysis, maxVoltage );

		// HOW TO MODIFY RAW DATA
		ArrayD flatline(analysis[0].size(), 1);
		for ( unsigned int c = 0; c < analysis.size(); c++ ) 
		{
			ClearPeaks( analysis[c], flatline );
		}
		Process(flatline);
	
		// CUT ACCORDING TO FLATLINE-Vector
		for ( int i = 0; i < flatline.size(); i++ ) 
		{
			for ( unsigned int c = 0; c < analysis.size(); c++ )
				if (flatline[i] != 1.0)
					analysis[c][i] = analysis[c][i] * flatline[i];
		}

		// ##############################################################
		// ##############################################################
		
		cout << "Calculating PSDs ..." << endl;
		// CREATE SPECTRA
		// FFT
		SampleDataD specsSum;
		vector<SampleDataD> allSpecs;
		vector<SampleDataD> allPhases;
		vector<SampleDataD> allPhasesSpecs;
		FftAll( fftWindowSize, phase_windowSize, samplerate, analysis, specsSum, allSpecs, allPhases, allPhasesSpecs, stepCounter );
		
		// ##############################################################
		// ##############################################################
		/*
		
		FREQUENCY DETECTION IS PERFORMED SEPARATELY ON EVERY ELECTRODE
		HARMONIC GROUP DETECTION (= FISH DETECTION) IS PERFORMED FOR EACH ROW SEPARATELY
			IN ORDER TO 
				1st - MAINTAIN SPATIAL STRUCTURE OF FISH AND PRESERVE OVERLAPPING FREQUENCIES
				2nd - GATHER AS MUCH HARMONIC INFORMATION AS POSSIBLE
				
				IT'S A TRADE OFF !!!
		
		*/
		// ##############################################################
		// ##############################################################
		
		cout << "Detection ..." << endl;

/*
create list of detected frequencies for each row for each detection cycle
check: if a frequency +- tolerance has been found on other electrodes of this row
*/

		// ##############################################################
		
		vector < MapD > freqsList;
		vector < MapD > moreFreqsList;
		freqsList.reserve( allSpecs.size() );
		moreFreqsList.reserve( allSpecs.size() );
		
		// PERFORM DETECTION ON EVERY SINGLE ELECTRODE
		for ( unsigned int e = 0; e < allSpecs.size(); e++ )
		{
			// DETECT FREQUENCY GROUPS AND IDENTIFY FUNDAMENTALS
			MapD freqs;
			MapD morefreqs;				
			
			cout << "\n####### Detecting frequencies ... Electrode: " <<  e << "\n" << endl;
			DetectFish( freqs, morefreqs, allSpecs[e], thresholdUpper, thresholdLower );

			// push frequency lists into a new storage vector for later processing
			freqsList.push_back( freqs );
			moreFreqsList.push_back( morefreqs );
		}
		cout << endl;
		
		// ##############################################################
		// ##############################################################

		// prepare structure for all detections in the whole grid
		vector < MapD > detectionsList;
		detectionsList.reserve( rows );
		
		// FOR EACH ELECTRODE ...
		bool fishFound = false;
		for ( unsigned int e = 0; e < allSpecs.size(); e++ )
		{
			cout << "ELECTRODE " << e+1 << endl;
			fTol = specsSum.stepsize();
			vector<MapD> groupList = FishFinderPlus( freqsList[e], moreFreqsList[e], specsSum,fTol);

			// FOR ALL GROUPS, FIND PHASES FOR DETECTED FREQUENCIES
			vector<MapD> groupListPhases;
			vector<MapD> groupListPhasesPower;
			PhaseFinder( groupList, allSpecs[e], allPhases[e], allPhasesSpecs[e], groupListPhases, groupListPhasesPower );
			
		// ##############################################################
		// ##############################################################
			
			// for output, calculate sum of power for each group
			vector<double> powerSum;
			powerSum.reserve( groupList.size() );
			for ( unsigned int u = 0; u < groupList.size(); u++ )
			{
				double powerOfGroup = 0.0;
				for ( int v = 0; v < groupList[u].size(); v++ )
					powerOfGroup += groupList[u][v];
				powerSum.push_back( powerOfGroup );
			}
			
			// ELECTRODE-SPECIFIC
			{
				string filename = outDir + "results/" + resultDirName + "Detections.dat";
				
				fstream data;
				data.open( filename.c_str() ,ios::out|ios::app);
				
				// SAVE DATA
				// OUTPUT FORMAT: time; electrode-no; sum-of-power; frequency-1 , power-1; frequency-2 , power-2; ...
				for ( unsigned int k = 0; k < groupList.size(); k++ )
				{
					data << Str( (double) index / (double) samplerate /60. ,11, 6, 'f' ) << ";\t "  // time
											<< index << ";\t "  // index
											<< e+1 << ";\t "  // electrode number
											<< powerSum[k] << ";\t ";  // total power of harmonic group
					// OUTPUT DATA: frequency && power of long spec, power and phase of short spec
					for ( int h = 0; h < groupList[k].size(); h++ )
						data << Str( groupList[k].x(h) , 8, 3, 'f' ) << ", " << Str( groupList[k].y(h) , 11, 9, 'f' ) << ", " 
									<< Str( groupListPhasesPower[k].y(h) , 11, 9, 'f' ) << ", " 
									<< Str( groupListPhases[k].y(h) , 6, 4, 'f' ) << ";\t " ; // fundamental frequency
					data << "\n";
				}
				if ( groupList.size() > 0 )
				{
					fishFound = true;
					data << "\n";
				}
					data.close();
				
				
				cout << endl;
			}
			
			// ##############################################################				
			// ##############################################################				
			
			//~ cout << "Press key to continue ..." << endl;
			//~ PressAnyKey();				
			
			//~ j++;
		}

		
		// INSERT A LINE BETWEEN EACH TIME BLOCK
		if (!debug)
		{
			if ( fishFound )
			{
				string filename = outDir + "results/" + resultDirName + "Detections.dat";
				fstream data;
				data.open( filename.c_str() ,ios::out|ios::app);
				data << "\n";
				data.close();
			}
		}
		fishFound = false;

		
		// SAVE SPECTRUM-DATA FOR EVALUATION
		if (!debug)
		{
			if (save_spectra)
			{
				for ( unsigned int a = 0; a < allSpecs.size(); a++)
				{
					// REDUCTION OF SPECTRUM RESOLUTION, IF NECESSARY
					// check ...
					SampleDataD interpolatedSpec;
					if ( allSpecs[a].stepsize() < freqStep ) // if the resolution is smaller than freqStep: reduce resolution to freqStep
					{
						// MAKE SURE THAT RANGE OF SPECSSUM IS BIGGER THAN RANGE OF INTERPOLATEDSPEC
						interpolatedSpec = SampleDataD(0.0, psdMaxFreq, freqStep);
						for ( int k = 0; k < interpolatedSpec.size(); k++ )
							interpolatedSpec[k] = allSpecs[a].interpolate(interpolatedSpec.pos(k));
					}
					else // otherwise, keep the old spectrum and save it
					{
						interpolatedSpec = allSpecs[a];
					}
					
					// CREATE OUTPUT STREAMS				
					string filename = outDir + "results/" + resultDirName + "spectrum" + Str( a+1, "%02d" ) + ".fft";
					fstream spectrum;
					spectrum.open( filename.c_str() ,ios::out|ios::app);
					// SAVE SPECTRUM
					int s = 0;
					while ( interpolatedSpec.pos(s) < psdMaxFreq && s < interpolatedSpec.size() )
					{
						spectrum << Str( (double) index / (double) samplerate /60. , 9, 4, 'f' ) << "\t "  // time
												<< Str( interpolatedSpec.pos(s), 8, 3, 'f' ) << "\t " // frequency
												<< Str( interpolatedSpec[s], 14, 12, 'f' ) << "\n";  // value
						s++;
					}
					spectrum << "\n";
					spectrum.close();
				}		
			}
		}
		cout << "\n#######\n"<<  endl;
		
		if (debug)
		{
			return 0;
		}
		
	}
		// ##############################################################
		// ##############################################################
	
	cout << "\nDoneDone!!\n" << endl;
	
	return 0;
		
}
