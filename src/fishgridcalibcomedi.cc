#include <iostream>
#include <fstream>
#include <deque>
#include <relacs/str.h>
#include <relacs/strqueue.h>
#include <relacs/options.h>
#include <comedilib.h>
using namespace std;
using namespace relacs;


void writeCalib( ofstream &df, StrQueue &sq, double range )
{
  df << "*Acquisition\n";
  sq.erase( 0 );
  if ( sq.back().empty() )
    sq.erase( sq.size()-1 );
  Options opt( sq );

  static int MaxDevices = 4;

  for ( int j=0; j<MaxDevices; j++ ) {
    // open comedi device:
    string devicefile = opt.text( "device" + Str( j+1 ) );
    if ( devicefile.empty() )
      continue;
    comedi_t *dev = comedi_open( devicefile.c_str() );
    if ( dev == NULL )
      continue;
    
    // get AI subdevice:
    int subdev = comedi_find_subdevice_by_type( dev, COMEDI_SUBD_AI, 0 );
    if ( subdev < 0 ) {
      comedi_close( dev );
      continue;
    }

    // lock AI subdevice:
    if ( comedi_lock( dev, subdev ) != 0 ) {
      comedi_close( dev );
      continue;
    }  

    // get calibration:
    cout << "Get calibration for " << devicefile << " ...\n";
    comedi_calibration_t *calibration = 0;
    {
      char *calibpath = comedi_get_default_calibration_path( dev );
      cout << "Read in calibration from " + Str( calibpath ) << " ...\n";
      ifstream cf( calibpath );
      if ( cf.good() )
	calibration = comedi_parse_calibration_file( calibpath );
      else
	calibration = NULL;
      if ( calibration == NULL )
	cerr << "calibration file does not exist, or reading failed.\n";
      delete [] calibpath;
    }

    bool softcal = ( ( comedi_get_subdevice_flags( dev, subdev ) &
		       SDF_SOFT_CALIBRATED ) > 0 );
    comedi_polynomial_t calib;
    int rangeinx = comedi_find_range( dev, subdev, 0, UNIT_volt,
				      -range, range );
    if ( rangeinx < 0 ) {
      cerr << "! error -> comedi_find_range: no range for "
	   << range << " V found.\n";
      comedi_close( dev );
      continue;
    }
    if ( softcal && calibration != 0 )
      comedi_get_softcal_converter( subdev, 0, rangeinx, COMEDI_TO_PHYSICAL,
				    calibration, &calib );
    else
      comedi_get_hardcal_converter( dev, subdev, 0, rangeinx,
				    COMEDI_TO_PHYSICAL, &calib );

    opt.setInteger( "caliborder" + Str( j+1 ), calib.order );
    opt.setNumber( "caliborigin" + Str( j+1 ), calib.expansion_origin );
    for ( unsigned int k=0; k<=calib.order; k++ ) {
      opt.setFormat( "calibcoeff" + Str( k ) + Str( j+1 ), "%.20f" );
      opt.setNumber( "calibcoeff" + Str( k ) + Str( j+1 ), calib.coefficients[k] );
    }
    cout << "Found calibration for " << range << " V range:\n";
    cout << "     Order=" << calib.order << '\n';
    cout << "    Origin=" << calib.expansion_origin << '\n';
    for ( unsigned int k=0; k<=calib.order; k++ )
      cout << "  Coeff[" << k << "]=" << calib.coefficients[k] << '\n';
    cout << '\n';
  }

  opt.save( df, "  " );

}


int main( void )
{
  cout << "This is fishgridcalibcomedi\n\n";

  cout << "Read in 'fishgid.cfg' ...\n";
  double gain = 1.0;
  double maxvolt = 10.0;
  ifstream sf( "fishgrid.cfg" );
  deque< StrQueue > config;
  string line = "";
  StrQueue sq;
  while ( true ) {
    sq.clear();
    sq.load( sf, "*", &line ).good();
    config.push_back( sq );
    if ( sq.size() > 0 && sq[0] == "*FishGrid" ) {
      Options opt( sq );
      gain = opt.number( "Gain", gain );
      maxvolt = opt.number( "AIMaxVolt", "V", maxvolt );
    }
    if ( ! sf.good() )
      break;
  }
  sf.close();

  cout << "Write out 'fishgid.cfg' ...\n\n";
  ofstream df( "fishgrid.cfg" );
  for ( unsigned int k=0; k<config.size(); k++ ) {
    if ( config[k].size() > 0 && config[k][0] == "*Acquisition" ) {
      writeCalib( df, config[k], gain*maxvolt );
    }
    else {
      config[k].save( df );
    }
  }

  return 0;
}
