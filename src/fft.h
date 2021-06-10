#ifndef __math_fft_h__
#define __math_fft_h__

#include <fftw3.h>
#include "image.h"

namespace MR {
  namespace Math {


    //! a class to perform an in-place 1D FFT
    /*! This class expects its data buffer of size \a N to be filled in using
     * its operator[] method, then the run() method can be invoked, and the
     * results read out using the same operator[] method.
     *
     * The \a direction parameter should be either FFTW_FORWARD or
     * FFTW_BACKWARD */
    class FFT1D {
      public:
        FFT1D (size_t N, int direction) :
          _data (N),
          direction (direction) {
            fftw_complex* p = reinterpret_cast<fftw_complex*> (&(_data[0]));
            _plan = fftw_plan_dft_1d (N, p, p, direction, FFTW_MEASURE);
          }

        FFT1D (const FFT1D& other) :
          _data (other._data.size()),
          direction (other.direction) {
            fftw_complex* p = reinterpret_cast<fftw_complex*> (&(_data[0]));
            _plan = fftw_plan_dft_1d (other._data.size(), p, p, direction, FFTW_MEASURE);
          }

        const cdouble& operator[] (size_t n) const { return _data[n]; }
        cdouble& operator[] (size_t n) { return _data[n]; }

        void run () { fftw_execute (_plan); }

      protected:
        Eigen::VectorXcd _data;
        fftw_plan _plan;
        int direction;
    };

    namespace {
      inline std::string direction_str (int direction) { return ( direction == FFTW_FORWARD ? "forward" : "backward" ); }
    }




    //! perform FFT in-place along specified axis
     /* The \a direction parameter should be either FFTW_FORWARD or
      * FFTW_BACKWARD */
    template <class ImageType>
      inline void FFT (ImageType& image, size_t axis, int direction)
      {
        class FFTFunctor {
          public:
            FFTFunctor (const ImageType& image, size_t axis, int direction) :
              image (image),
              fft (image.size(axis), direction),
              axis (axis) { }

            void operator() (const Iterator& pos)
            {
              assign_pos_of (pos).to (image);
              for (auto l = Loop (axis, axis+1) (image); l; ++l)
                fft[image.index(axis)] = cdouble (image.value());
              fft.run();
              for (auto l = Loop (axis, axis+1) (image); l; ++l)
                image.value() = fft[image.index(axis)];
            }

          protected:
            ImageType image;
            FFT1D fft;
            size_t axis;
        };

        auto outer_axes = Stride::order (image);
        outer_axes.erase (std::find (outer_axes.begin(), outer_axes.end(), axis));
        outer_axes.insert (outer_axes.begin(), axis);

        ThreadedLoop ("performing " + direction_str (direction) + " FFT along slice " + str(axis), image, outer_axes, 1)
          .run_outer (FFTFunctor (image, axis, direction));
      }






    //! perform FFT of input image \a in along specified axis, writing results into output image \a out
     /* The \a direction parameter should be either FFTW_FORWARD or
      * FFTW_BACKWARD */
    template <class ImageTypeIn, class ImageTypeOut>
      inline void FFT (ImageTypeIn& in, ImageTypeOut& out, size_t axis, int direction)
      {
        class FFTFunctor {
          public:
            FFTFunctor (const ImageTypeIn& in, const ImageTypeOut& out, size_t axis, int direction) :
              in (in),
              out (out),
              fft (in.size(axis), direction),
              axis (axis) { }

            void operator() (const Iterator& pos)
            {
              assign_pos_of (pos).to (in, out);
              for (auto l = Loop (axis, axis+1) (in); l; ++l)
                fft[in.index(axis)] = cdouble (in.value());
              fft.run();
              for (auto l = Loop (axis, axis+1) (out); l; ++l)
                out.value() = fft[out.index(axis)];
            }

          protected:
            ImageTypeIn in;
            ImageTypeOut out;
            FFT1D fft;
            size_t axis;
        };

        auto outer_axes = Stride::order (in);
        outer_axes.erase (std::find (outer_axes.begin(), outer_axes.end(), axis));
        outer_axes.insert (outer_axes.begin(), axis);

        ThreadedLoop ("performing " + direction_str (direction) + " FFT along slice " + str(axis), in, outer_axes, 1)
          .run_outer (FFTFunctor (in, out, axis, direction));
      }


  }
}


#endif
