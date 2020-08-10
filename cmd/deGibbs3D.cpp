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



using ImageType = Image<cdouble>;



inline double indexshift(ssize_t n, ssize_t size) {
  if (n > size/2) n -= size;
  return n;
}



void run() {


  // reading input and assigning output
  auto input = ImageType::open(argument[0]);
  Header header (input);
  header.datatype() = DataType::CFloat32;
  auto output = ImageType::create (argument[1], header);



  // Filter for axis 0
  class Filter {
  	public:
  		Filter(size_t axis, double sigma) : axis(axis), sigma(sigma) {}
  		void operator() (ImageType& image){
  			//apply filter
  			const double x[3] = {
          indexshift(image.index(0), image.size(0)),
          indexshift(image.index(1), image.size(1)),
          indexshift(image.index(2), image.size(2))
        };
        image.value() *= exp (-(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]) / sigma);
  		}

  	protected: 
  		const size_t axis;
  		const double sigma;
  };



  // first FFT from input to output:
  Math::FFT (input, output, 0, FFTW_FORWARD);
  // subsequent FFTs in-place:
  Math::FFT (output, 1, FFTW_FORWARD);
  Math::FFT (output, 2, FFTW_FORWARD);

  //filter along x:
  ThreadedLoop(output).run(Filter(0,0.1),output);

  // then inverse FT back to image domain
  Math::FFT (output, 0, FFTW_BACKWARD);
  Math::FFT (output, 1, FFTW_BACKWARD);
  Math::FFT (output, 2, FFTW_BACKWARD);



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

  //line-by-line single-threaded processing along x:
  Shift1D shifter (0.1); 
  Math::FFT1D fft (output.size(0), FFTW_FORWARD);
  Math::FFT1D ifft (output.size(0), FFTW_BACKWARD);


 	//ThreadedLoop (output).run (Shift1D(shift), output); <--- this runs it for the entire image
  


  for (output.index(2) = 0; output.index(2) < output.size(2); ++output.index(2)) {
   for (output.index(1) = 0; output.index(1) < output.size(1); ++output.index(1)) {

    for (output.index(0) = 0; output.index(0) < output.size(0); ++output.index(0))
     fft[output.index(0)] = output.value();
    fft.run();


    const std::complex<double> j(0.0,1.0);
    double shift = 0.1;
    for (ssize_t n = 0; n < output.size(0); ++n)
      ifft[n] = fft[n] * exp(j * 2.0 * indexshift(n,output.size(0)) * Math::pi * shift);

    ifft.run();

    for (output.index(0) = 0; output.index(0) < output.size(0); ++output.index(0))
      output.value() = ifft[output.index(0)];

   }
  }
 	

 	// equivalent implementation using multi-threading:
  class LineProcessor {
    public:
      LineProcessor (size_t axis, ImageType& image) :
        axis (axis),
        image (image),
        fft (image.size(axis), FFTW_FORWARD),
        ifft (20, Math::FFT1D(image.size(axis), FFTW_BACKWARD)) { }


      void operator() (const Iterator& pos)
      {
        assign_pos_of (pos).to (image);

        for (image.index(axis) = 0; image.index(axis) < image.size(axis); ++image.index(axis))
          fft[image.index(axis)] = image.value();
        fft.run();

        int num_shifts = 20;
        const std::complex<double> j(0.0,1.0);
        const double shift = 1.0/(image.size(axis)*double(num_shifts));
        for (int f = 0; f < num_shifts; f ++) {
          for (int n = 0; n < image.size(axis); ++n)
            ifft[f][n] = fft[n] * exp(j * 2.0 * indexshift(n,image.size(axis)) * Math::pi * f * shift);
          ifft[f].run();
        }

        image.index(axis) = 0;
        do {
          for (int n = 0; n < image.size(axis); ++n) {
            optshift_ind = optimumshift(n);
            double optshift = (optshift_ind+1) * shift;

            //interpolate particular ifft back to right place
            if (!(n == 0 | n == image.size(axis)-1)) 
              image.value() = (ifft[0][n+1] - ifft[0][n]) * optshift + image.value();

            ++image.index(axis);
          }
        }while (image.index(axis) < image.size(axis));

        // for (image.index(axis) = 0; image.index(axis) < image.size(axis); ++image.index(axis))
        //   image.value() = ifft[image.index(axis)];
      }


    protected:
      const size_t axis;
      ImageType image;
      Math::FFT1D fft;
      std::vector<Math::FFT1D> ifft;
      int optimumshift(int n) {

        int num_shifts = 20;
        double opt_var = 0;
        int ind;

        for (int f = 0; f < num_shifts; ++f) {
          double sum_left = 0;
          double sum_right = 0;

          for (ssize_t k = -3 ; k < 3; ++k) {
            sum_left += abs(ifft[f][n+k] - ifft[f][n+(k-1)]);
            sum_right += abs(ifft[f][n-k] - ifft[f][n-(k-1)]);
          }

          double tot_var;
          if (sum_left < sum_right) 
            tot_var = sum_left;
          else
            tot_var = sum_right;

          if (opt_var > tot_var)
            ind = f;
        }
        return ind;
      }
  };

  

  // FFT back to check we get back the original image:
  Math::FFT (output, 0, FFTW_BACKWARD);
  //Math::FFT (output, 1, FFTW_BACKWARD);
  //Math::FFT (output, 2, FFTW_BACKWARD);


  // This rescales the output because the FFT is unnormalised
  // http://www.fftw.org/fftw3_doc/The-1d-Discrete-Fourier-Transform-_0028DFT_0029.html#The-1d-Discrete-Fourier-Transform-_0028DFT_0029

  // This is done using multi-threading - it's about the simplest example of
  // multi-threading you'll come across...

  // This class will perform the per-voxel operation (divide by N):
  // struct RescaleFn {
  //   RescaleFn (double N) : N (N) { } // intialiser list
  //   void operator() (decltype(output)& out) { out.value() /= N; }
  //   const double N;
  // };

  // This sets up the loop to run over the output image (with a progress status
  // message), then directly runs the RescaleFn on the output image:
  // ThreadedLoop ("rescaling", output).run (RescaleFn (output.size(0)*output.size(1)*output.size(2)), output);

  // To better appreciate the above, this is what it might look like when it's
  // more fully expanded:
  //
  // RescaleFn func (output.size(0)*output.size(1)*output.size(2));
  // auto loop = ThreadedLoop ("rescaling", output);
  // loop.run (func, output);
}
