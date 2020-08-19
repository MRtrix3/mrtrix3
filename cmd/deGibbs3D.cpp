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

inline int wraparound(ssize_t n, ssize_t size) {
	if (n > size || n < 0) n = (n+size)%size;
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
  // auto tmp = Image<cdouble>::scratch(header);


  // class Shift1D {
  //  public:
  //    Shift1D(double shift) : shift(shift) {}
  //    void operator() (decltype(output)& output)
  //    {
  //      const std::complex<double> j(0.0,1.0);
  //      output.value() *= exp(j * 2.0 * indexshift(output.index(0),output.size(0)) * Math::pi * shift);
  //    }
        
  //  protected:
  //    const double shift;
  // };

  // //line-by-line single-threaded processing along x:
  // Shift1D shifter (0.1); 
  // Math::FFT1D fft (output.size(0), FFTW_FORWARD);
  // Math::FFT1D ifft (output.size(0), FFTW_BACKWARD);


 	// //ThreadedLoop (output).run (Shift1D(shift), output); <--- this runs it for the entire image
  


  // for (output.index(2) = 0; output.index(2) < output.size(2); ++output.index(2)) {
  //  for (output.index(1) = 0; output.index(1) < output.size(1); ++output.index(1)) {

  //   for (output.index(0) = 0; output.index(0) < output.size(0); ++output.index(0))
  //    fft[output.index(0)] = output.value();
  //   fft.run();


  //   const std::complex<double> j(0.0,1.0);
  //   double shift = 0.1;
  //   for (ssize_t n = 0; n < output.size(0); ++n)
  //     ifft[n] = fft[n] * exp(j * 2.0 * indexshift(n,output.size(0)) * Math::pi * shift);

  //   ifft.run();

  //   for (output.index(0) = 0; output.index(0) < output.size(0); ++output.index(0))
  //     output.value() = ifft[output.index(0)];

  //  }
  // }
 	
  const int minW = 1;
  const int maxW = 3;

 	// equivalent implementation using multi-threading:
  class LineProcessor {
    public:
      LineProcessor (size_t axis, ImageType& image, const int minW, const int maxW) :
        axis (axis),
        image (image),
        minW (minW),
        maxW (maxW), 
        fft (image.size(axis), FFTW_FORWARD),
        ifft (20, Math::FFT1D(image.size(axis), FFTW_BACKWARD)) { }


      void operator() (const Iterator& pos)
      {
        assign_pos_of (pos).to (image);

        for (image.index(axis) = 0; image.index(axis) < image.size(axis); ++image.index(axis))
          fft[image.index(axis)] = image.value();
        fft.run();
        std::cout << "ifft created" << std::endl;

        int num_shifts = 20;
        const std::complex<double> j(0.0,1.0);
        const double shift = 1.0/(image.size(axis)*double(num_shifts));
        int lsize = image.size(axis);
        
        // creating zero-centred shift array
       	long shift_ind[2*num_shifts+1];
        shift_ind[0] = 0;
        for (int j = 0; j < num_shifts; j++) {
          shift_ind[j+1] = j+1;
          shift_ind[1+num_shifts+j] = -(j+1);
        }


        for (int f = 0; f < 2*num_shifts+1; f ++) {
          for (int n = 0; n < lsize; ++n)
            ifft[f][n] = fft[n] * exp(j * 2.0 * indexshift(n,image.size(axis)) * Math::pi * double(shift_ind[f]) * shift);
          ifft[f].run();
        }

        for (int n = 0; n < lsize; ++n) {
          image.index(axis) = n;
          // calculating value for optimum shift
          int optshift_ind = optimumshift(n, lsize);

          std::cout << "Optimum shift found" << std::endl;

          double shift = double(shift_ind[optshift_ind]/num_shifts);

          // calculating current, previous and next (real and imaginary) values
          double a0r = ifft[optshift_ind][wraparound(n-1,lsize)].real();
          double a1r = ifft[optshift_ind][n].real();
          double a2r = ifft[optshift_ind][wraparound(n+1,lsize)].real();
          double a0i = ifft[optshift_ind][wraparound(n-1,lsize)].imag();
          double a1i = ifft[optshift_ind][n].imag();
          double a2i = ifft[optshift_ind][wraparound(n+1,lsize)].imag();

          //interpolate particular ifft back to right place
          if (shift < 0.0)
            //image.value() = ifft[optshift_ind][n-1+] + (ifft[optshift_ind][n-1] - ifft[optshift_ind][n]) * shift_ind[optshift_ind];
            image.value() = cdouble (a1r - shift*(a0r-a1r), a1i - shift*(a0i-a1i));
          else
          	image.value() = cdouble (a1r + shift*(a2r-a1r), a1i + shift*(a2i-a1i));
        }
        // for (image.index(axis) = 0; image.index(axis) < image.size(axis); ++image.index(axis))
        //   image.value() = ifft[image.index(axis)];
      }


    protected:
      const size_t axis;
      const int minW, maxW;
      ImageType image;
      Math::FFT1D fft;
      std::vector<Math::FFT1D> ifft;
      int optimumshift(int n, int lsize) {

        int num_shifts = 20;
        double opt_var = 0;
        int ind;

        for (int f = 0; f < 2*num_shifts+1; f++) {
          double sum_left = 0;
          double sum_right = 0;

          // calculating oscillation measure within given window
          for (int k = minW; k <= maxW; ++k) {
              sum_left += abs(ifft[f][wraparound(n-k,lsize)].real() - ifft[f][wraparound(n-k-1,lsize)].real());
              sum_left += abs(ifft[f][wraparound(n-k,lsize)].imag() - ifft[f][wraparound(n-k-1,lsize)].imag());

              sum_right += abs(ifft[f][wraparound(n+k,lsize)].real() - ifft[f][wraparound(n+k+1,lsize)].real());
              sum_right += abs(ifft[f][wraparound(n+k,lsize)].imag() - ifft[f][wraparound(n+k+1,lsize)].imag());
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

    ThreadedLoop (output, { 0, 1, 2 }).run_outer (LineProcessor (0, output, minW, maxW));
  //ThreadedLoop (output, { 1, 0, 2 }).run_outer (LineProcessor (1, output, minW, maxW));
  //ThreadedLoop (output, { 2, 0, 1 }).run_outer (LineProcessor (2, output, minW, maxW));


  // FFT back to check we get back the original image:
  // Math::FFT (output, 0, FFTW_BACKWARD); 
  // Math::FFT (output, 1, FFTW_BACKWARD);
  // Math::FFT (output, 2, FFTW_BACKWARD);


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
