#include "fft.h"
#include <fstream>
#include <iostream>
#include <vector>


#include "command.h"
#include "image.h"
#include "header.h"


using namespace std;
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



void run() {


  // reading input and assigning output
  auto input = Image<cdouble>::open(argument[0]);
  Header header (input);
  header.datatype() = DataType::CFloat32;
  auto output = Image<cdouble>::create (argument[1], header);

  // first FFT from input to output:
  Math::FFT (input, output, 0, FFTW_FORWARD);
  // subsequent FFTs in-place:
  Math::FFT (output, 1, FFTW_FORWARD);
  Math::FFT (output, 2, FFTW_FORWARD);

  // creating temporary image to store intermediate image shifts
  //auto tmp = Image<cdouble>::scratch(header)

  // performing a shift in the image domain is equivalent to multiplying the image in the fourier domain with an exponential
  double shift = 100.0;
  const double pi = 3.14159265358979323846;
  const complex<double> j(0.0,-1.0);

  // performing shift in 1D
  for (int x = 0; x < output.size(0); x++){
  	double ind = output.get_index(0);
  	cdouble val = output.get_value() * exp(j * 2.0 * ind * pi * shift);
  	output.set_value(val);
  	output.move_index(0, shift);
  };


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
