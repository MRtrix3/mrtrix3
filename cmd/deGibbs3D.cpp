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
using OutputImageType = Image<float>;



// gives proper index according to fourier indices
inline double indexshift(ssize_t n, ssize_t size) {
  if (n > size/2) n -= size;
  return n;
}



// ensures n is never out-of-bounds
inline int wraparound(ssize_t n, ssize_t size) {
	n = (n+size)%size;
	return n;
}




// Filter for axis 0
class Filter {
  public:
    Filter(size_t axis) : axis(axis){}
    void operator() (ImageType& in, ImageType& out){
      //apply filter
      const double x[3] = {
        1.0 + std::cos (2.0 * Math::pi * indexshift (in.index(0), in.size(0)) / in.size(0)),
        1.0 + std::cos (2.0 * Math::pi * indexshift (in.index(1), in.size(1)) / in.size(1)),
        1.0 + std::cos (2.0 * Math::pi * indexshift (in.index(2), in.size(2)) / in.size(2))
      };
      const double w[3] = { x[1]*x[2], x[0]*x[2], x[0]*x[1] };
      const double denom = w[0] + w[1] + w[2];

      out.value() = cdouble (in.value()) * ( denom ? w[axis] / denom : 0.0 );

    }

  protected:
    const size_t axis;
};








class LineProcessor {
  public:
    LineProcessor (size_t axis, ImageType& input, OutputImageType& output, const int minW, const int maxW, const int num_shifts) :
      axis (axis),
      input (input),
      output (output),
      minW (minW),
      maxW (maxW),
      num_shifts (num_shifts),
      fft (input.size(axis), FFTW_FORWARD),
      ifft (2*num_shifts+1, Math::FFT1D(input.size(axis), FFTW_BACKWARD)) { }


    void operator() (const Iterator& pos)
    {
      assign_pos_of (pos).to (input, output);


      for (auto l = Loop (axis, axis+1)(input); l; ++l)
        fft[input.index(axis)] = input.value();
      fft.run();


      const int lsize = input.size(axis);


      // creating zero-centred shift array
      double shift_ind [2*num_shifts+1];
      shift_ind[0] = 0;
      for (int j = 0; j < num_shifts; j++) {
        shift_ind[j+1] = (j+1) / (2.0 * num_shifts + 1.0);
        shift_ind[1+num_shifts+j] = -shift_ind[j+1];
      }


      // applying shift and inverse fourier transform back line-by-line
      const std::complex<double> j(0.0,1.0);
      for (int f = 0; f < 2*num_shifts+1; f ++) {
        for (int n = 0; n < lsize; ++n)
          ifft[f][n] = fft[n] * exp(j * 2.0 * indexshift(n,lsize) * Math::pi * shift_ind[f] / double(lsize));
        if (!(lsize&1))
          ifft[f][lsize/2] = 0.0;
        ifft[f].run();
      }


      for (int n = 0; n < lsize; ++n) {
        output.index(axis) = n;

        // calculating value for optimum shift
        const int optshift_ind = optimumshift(n,lsize);
        const double shift = shift_ind[optshift_ind];

        // calculating current, previous and next (real and imaginary) values
        double a0r = ifft[optshift_ind][wraparound(n-1,lsize)].real();
        double a1r = ifft[optshift_ind][n].real();
        double a2r = ifft[optshift_ind][wraparound(n+1,lsize)].real();

        const double scale = input.size(0)*input.size(1)*input.size(2) * lsize;

        // interpolate particular ifft back to right place
        if (shift > 0.0)
          output.value() += (a1r - shift*(a1r-a0r)) / scale;
        else
          output.value() += (a1r + shift*(a1r-a2r)) / scale;

      }

    }


  protected:
    const size_t axis;
    ImageType input;
    OutputImageType output;
    const int minW, maxW, num_shifts;
    Math::FFT1D fft;
    std::vector<Math::FFT1D> ifft;


    int optimumshift(int n, int lsize) {
      int ind = 0;
      double opt_var = std::numeric_limits<double>::max();

      // calculating oscillation measure for subsequent shifts
      for (int f = 0; f < 2*num_shifts+1; f++) {
        double sum_left = 0.0, sum_right = 0.0;

        // calculating oscillation measure within given window
        for (int k = minW; k <= maxW; ++k) {
          sum_left += abs(ifft[f][wraparound(n-k,lsize)].real() - ifft[f][wraparound(n-k-1,lsize)].real());
          sum_left += abs(ifft[f][wraparound(n-k,lsize)].imag() - ifft[f][wraparound(n-k-1,lsize)].imag());

          sum_right += abs(ifft[f][wraparound(n+k,lsize)].real() - ifft[f][wraparound(n+k+1,lsize)].real());
          sum_right += abs(ifft[f][wraparound(n+k,lsize)].imag() - ifft[f][wraparound(n+k+1,lsize)].imag());
        }

        double tot_var = std::min(sum_left,sum_right);

        if (tot_var < opt_var) {
          opt_var = tot_var;
          ind = f;
        }

      }

      return ind;
    }

};






inline vector<size_t> strides_for_axis (int axis)
{
  switch (axis) {
    case 0: return { 0, 1, 2 };
    case 1: return { 1, 2, 0 };
    case 2: return { 2, 0, 1 };
  }
  return { };
}








void run()
{
  const int minW = 1;
  const int maxW = 3;
  const int num_shifts = 20;


  // reading input and assigning output
  auto input = ImageType::open(argument[0]);
  Header header (input);
  header.datatype() = DataType::CFloat32;
  auto image_FT = ImageType::scratch (header, "FFT of input image");
  auto image_filtered = ImageType::scratch (header, "filtered image");

  header.datatype() = DataType::Float32;
  auto output = OutputImageType::create (argument[1], header);

  ProgressBar progress ("performing 3D Gibbs ringing removal", 3);

  // full 3D FFT of input:
  INFO ("performing initial 3D forward Fourier transform...");
  Math::FFT (input, image_FT, 0, FFTW_FORWARD);
  Math::FFT (image_FT, 1, FFTW_FORWARD);
  Math::FFT (image_FT, 2, FFTW_FORWARD);

  for (int axis = 0; axis < 3; ++axis) {

    // filter along x:
    INFO ("filtering for axis "+str(axis)+"...");
    ThreadedLoop(image_FT).run (Filter(axis), image_FT, image_filtered);

    // then inverse FT back to image domain:
    INFO ("applying 3D backward Fourier transform...");
    Math::FFT (image_filtered, 0, FFTW_BACKWARD);
    Math::FFT (image_filtered, 1, FFTW_BACKWARD);
    Math::FFT (image_filtered, 2, FFTW_BACKWARD);

    // apply unringing operation on desired axis:
    INFO ("performing unringing along axis "+str(axis)+"...");
    ThreadedLoop (image_filtered, strides_for_axis (axis))
      .run_outer (LineProcessor (axis, image_filtered, output, minW, maxW, num_shifts));

    ++progress;
  }

}
