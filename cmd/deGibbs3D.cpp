#include "fft.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>


#include "command.h"
#include "image.h"
#include "header.h"


using namespace MR;
using namespace App;

void usage() {
  AUTHOR = "Thea Bautista";

  SYNOPSIS = "Removal of Gibbs Ringing in 3D";

  DESCRIPTION
    + "This reads an input nifti file and outputs an image after running fft function.";

  ARGUMENTS
    + Argument ("inImg", "input image to be read").type_image_in()
    + Argument ("outImg", "outuput image").type_image_out();

}


inline double indexshift(ssize_t n, ssize_t size) {
  if (n > size/2) n -= size;
  return n;
}


inline std::vector<double> make_range(double begin, double end, double num_shifts) {

	std::vector<double> new_range(num_shifts);
	new_range.push_back(begin);

	double shift = (end - begin)/num_shifts;

	for (int r = 0; r < new_range.size()-1; ++r)
		new_range.push_back(new_range[r]+shift);
	
	new_range.push_back(end);

	return new_range;
}


void run() {


  // reading input and assigning output
  auto input = Image<cdouble>::open(argument[0]);
  Header header (input);
  header.datatype() = DataType::CFloat32;
  auto output = Image<cdouble>::create (argument[1], header);

  // creating temp image to store gaussian filter
  auto gauss = Image<cdouble>::scratch(header);

  // first FFT from input to output:
  Math::FFT (input, output, 0, FFTW_FORWARD);
  // subsequent FFTs in-place:
  Math::FFT (output, 1, FFTW_FORWARD);
  Math::FFT (output, 2, FFTW_FORWARD);

  // creating temporary image to store intermediate image shifts
  //auto tmp = Image<cdouble>::scratch(header);

  class Shift1D {
   public:
     Shift1D(double shift) : shift(shift) {}
     void operator() (decltype(output)& output)
     {
       const std::complex<double> j(0.0,1.0);
       output.value() *= exp(j * 2.0 * indexshift(output.index(0),output.size(0)) * Math::pi * shift);
     }
        
   protected:
     const double shift;
  };


  //ThreadedLoop (output).run (Shift1D(shift), output);
  
  // initialising interval vector to loop over
  const std::vector<double> interval = make_range(1.0,3.0,20.0); // K=[1,3]; 20 shifts

  // intialising variables
  (Shift1D(interval[0]));
  std::complex<double> out1 = output.value();
  (Shift1D(interval[0]-1));
  std::complex<double> out2 = output.value();
  double osc_msr_left = abs(out1 - out2);

  (Shift1D(-interval[0]));
  out1 = output.value();
  (Shift1D(1-interval[0]));
  out2 = output.value();
  double osc_msr_right = abs(out1 - out2);

  double opt_left, opt_right, opt_shift;


  // looping over single axis to find optimum shift 
  for (auto l = Loop (output,0,3)(output); l ; ++l) {

  	// FIND OPTIMAL SHIFT FOR GIVEN POINT
  	for (int r = 1; r < interval.size(); ++r){

  		// calculate optimal to the left of the point
  		(Shift1D(interval[r]));
  		out1 = output.value();
  		(Shift1D(interval[r]-1));
  		out2 = output.value();
  		double osc= abs(out1 - out2);

  		if (osc < osc_msr_left) {
  			osc_msr_left = osc;
  			opt_left = interval[r];
  		}

  		// similarly for the right side of the voxel
  		(Shift1D(-interval[r]));
  		out1 = output.value();
  		(Shift1D(1-interval[r]));
  		out2 = output.value();
  		osc = abs(out1 - out2);

  		if (osc < osc_msr_right) {
        osc_msr_right = osc;
  			opt_right = interval[r];
      }

  	}

  	// finding min between left and right sides
  	if (opt_right > opt_left)
  		opt_shift = -1 * opt_right;
  	else
  		opt_shift = opt_left;

  }


  // APPLYING GAUSSIAN FILTER
  // const double sigma = 2;
  // for (auto l = Loop (output,0,3)(output); l ; ++l) {
  // 	const double r = pow(output.index(0),2.0) + pow(output.index(1),2.0) + pow(output.index(2),2.0);
  // 	output.value() *= exp(-2.0* pow(Math::pi,2.0) * pow(sigma,2.0) * pow(r,2.0));
 	// }


 	// 	class FiltGauss {
 	// 	public:
 	// 		FiltGauss(double sigma) : sigma(sigma){}
 	// 		void operator() (decltype(output)& output)
 	// 		{
 	// 			const double r = pow(output.index(0),2.0) + pow(output.index(1),2.0) + pow(output.index(2),2.0);
  // 			output.value() *= exp(-2.0* pow(Math::pi,2.0) * pow(sigma,2.0) * pow(r,2.0));
 	// 		}
 	// 	protected: 
 	// 		const double sigma;
 	// 	};

 	// ThreadedLoop (output).run (FiltGauss(sigma),output);


  // FFT back to check we get back the original image:
  Math::FFT (output, 0, FFTW_BACKWARD);
  Math::FFT (output, 1, FFTW_BACKWARD);
  Math::FFT (output, 2, FFTW_BACKWARD);


  // This rescales the output because the FFT is unnormalised
  // http://www.fftw.org/fftw3_doc/The-1d-Discrete-Fourier-Transform-_0028DFT_0029.html#The-1d-Discrete-Fourier-Transform-_0028DFT_0029

  // This is done using multi-threading - it's about the simplest example of
  // multi-threading you'll come across...

  // This class will perform the per-voxel operation (divide by N):
  struct RescaleFn {
    RescaleFn (double N) : N (N) { } // intialiser list
    void operator() (decltype(output)& out) { out.value() /= N; }
    const double N;
  };

  // This sets up the loop to run over the output image (with a progress status
  // message), then directly runs the RescaleFn on the output image:
  ThreadedLoop ("rescaling", output).run (RescaleFn (output.size(0)*output.size(1)*output.size(2)), output);

  // To better appreciate the above, this is what it might look like when it's
  // more fully expanded:
  //
  // RescaleFn func (output.size(0)*output.size(1)*output.size(2));
  // auto loop = ThreadedLoop ("rescaling", output);
  // loop.run (func, output);
}
