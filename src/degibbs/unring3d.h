/* Copyright (c) 2008-2022 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __degibbs_unring_3d_h__
#define __degibbs_unring_3d_h__

#include "axes.h"
#include "image.h"
#include "progressbar.h"
#include "algo/threaded_loop.h"
#include "math/hermite.h"

#include "degibbs/unring1d.h"


namespace MR {
  namespace Degibbs {



    using ImageType = Image<cdouble>;



    extern const char* interp_types[];
    enum class interp_t { LINEAR_MAG, LINEAR_COMPLEX, LINEAR_PSEUDOPOLAR, LINEAR_POLAR, HERMITE_MAG, HERMITE_COMPLEX };



    namespace {

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



      // perform filtering along required axis:
      class Filter
      { MEMALIGN (Filter)
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



      template <class OutputImageType>
      class LineProcessor
      { MEMALIGN (LineProcessor<OutputImageType>)
        public:
          LineProcessor (size_t axis, ImageType& input, OutputImageType& output, const interp_t interp, const int minW, const int maxW, const int num_shifts) :
            axis (axis),
            lsize (input.size(axis)),
            scale (1.0 / (input.size(0)*input.size(1)*input.size(2) * lsize)),
            input (input),
            output (output),
            interp (interp),
            hermite_mag (0.0),
            hermite_complex (0.0),
            minW (minW),
            maxW (maxW),
            num_shifts (num_shifts),
            shift_ind (2*num_shifts+1),
            fft (input.size(axis), FFTW_FORWARD),
            ifft (2*num_shifts+1, Math::FFT1D(input.size(axis), FFTW_BACKWARD))
          { 
            // creating zero-centred shift array
            shift_ind[0] = 0;
            for (int j = 0; j < num_shifts; j++) {
              shift_ind[j+1] = (j+1) / (2.0 * num_shifts + 1.0);
              shift_ind[1+num_shifts+j] = -shift_ind[j+1];
            }
          }


          void operator() (const Iterator& pos)
          {
            assign_pos_of (pos).to (input, output);


            for (auto l = Loop (axis, axis+1)(input); l; ++l)
              fft[input.index(axis)] = input.value();
            fft.run();



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

              switch (interp) {
                case interp_t::LINEAR_MAG:
                  {
                    // Calculate magnitudes of shifted values, then linear interpolation on those
                    // TODO Should linear interpolation when input data are magnitude only be
                    //   done strictly on magnitude by default, rather than complex and then taking the magnitude?
                    const double a1r = abs(ifft[optshift_ind][n]);
                    if (shift > 0.0) {
                      const double a0r = abs(ifft[optshift_ind][wraparound(n-1,lsize)]);
                      output.value() += scale * ((1.0-shift)*a1r + shift*a0r);
                    } else {
                      const double a2r = abs(ifft[optshift_ind][wraparound(n+1,lsize)]);
                      output.value() += scale * ((1.0+shift)*a1r - shift*a2r);
                    }
                  }
                  break;
                case interp_t::LINEAR_COMPLEX:
                  {
                    // Code from Donald's original implementation
                    const cdouble a1r = ifft[optshift_ind][n];
                    if (shift > 0.0) {
                      const cdouble a0r = ifft[optshift_ind][wraparound(n-1,lsize)];
                      output.value() += scale * ((1.0-shift)*a1r + shift*a0r);
                    } else {
                      const cdouble a2r = ifft[optshift_ind][wraparound(n+1,lsize)];
                      output.value() += scale * ((1.0+shift)*a1r - shift*a2r);
                    }
                  }
                  break;
                case interp_t::LINEAR_PSEUDOPOLAR:
                  // Effectively linear interpolation in polar coordinates (mag + phase) rather than real + imaginary
                  // However doesn't actually convert to this coordinate system; just scales the result
                  //   of interpolation to get the appropriate magniutude... so technically the interpolation is
                  //   not truly linear in phase...
                  {
                    const cdouble a1r = ifft[optshift_ind][n];
                    cdouble sum_complex = (1.0 - abs(shift)) * a1r;
                    double sum_mag = abs(sum_complex);
                    if (shift > 0.0) {
                      const cdouble a0r = ifft[optshift_ind][wraparound(n-1,lsize)];
                      sum_mag += shift*abs(a0r);
                      sum_complex += shift*a0r;
                    } else {
                      const cdouble a2r = ifft[optshift_ind][wraparound(n+1,lsize)];
                      sum_mag -= shift*abs(a2r);
                      sum_complex -= shift*a2r;
                    }
                    output.value() += scale * sum_complex * sum_mag / abs(sum_complex);
                  }
                  break;
                case interp_t::LINEAR_POLAR:
                  // Do genuine linear interpolations of magnitude and phase separately,
                  //   then reconstruct the final complex value from that
                  {
                    const cdouble a1r = ifft[optshift_ind][n];
                    const double phase_a1r = std::arg (a1r);
                    double sum_mag = (1.0 - abs(shift)) * abs(a1r);
                    double sum_phase = (1.0 - abs(shift)) * phase_a1r;
                    if (shift > 0.0) {
                      const cdouble a0r = ifft[optshift_ind][wraparound(n-1,lsize)];
                      double phase_a0r = std::arg (a0r);
                      // TODO Need to double-check this
                      // Not good enough: Need to consider *which of the two terms
                      //   is greater*, considering both magnitude and shift (?),
                      //   and then add / subtract 2pi to get the two within pi of one another
                      while (phase_a0r - phase_a1r > Math::pi)
                        phase_a0r -= 2.0*Math::pi;
                      while (phase_a0r - phase_a1r < Math::pi)
                        phase_a0r += 2.0*Math::pi;
                      sum_mag += shift*abs(a0r);
                      sum_phase += shift*phase_a0r;
                    } else {
                      const cdouble a2r = ifft[optshift_ind][wraparound(n+1,lsize)];
                      double phase_a2r = std::arg (a2r);
                      while (phase_a2r - phase_a1r > Math::pi)
                        phase_a2r -= 2.0*Math::pi;
                      while (phase_a2r - phase_a1r < Math::pi)
                        phase_a2r += 2.0*Math::pi;
                      sum_mag -= shift*abs(a2r);
                      sum_phase -= shift*phase_a2r;
                    }
                    output.value() += std::polar (scale * sum_mag, sum_phase);
                  }
                  break;
                case interp_t::HERMITE_MAG:
                  // Perform Hermite interpolation on magnitude data only
                  // Should recapitulate ringing in input magnitude data with insufficient tension
                  {
                    if (shift > 0.0) {
                      hermite_mag.set (shift);
                      output.value() += scale * hermite_mag.value (abs(ifft[optshift_ind][wraparound(n+1,lsize)]),
                                                                   abs(ifft[optshift_ind][wraparound(n,  lsize)]),
                                                                   abs(ifft[optshift_ind][wraparound(n-1,lsize)]),
                                                                   abs(ifft[optshift_ind][wraparound(n-2,lsize)]));
                    } else {
                      hermite_mag.set (1.0 + shift);
                      output.value() += scale * hermite_mag.value (abs(ifft[optshift_ind][wraparound(n+2,lsize)]),
                                                                   abs(ifft[optshift_ind][wraparound(n+1,lsize)]),
                                                                   abs(ifft[optshift_ind][wraparound(n,  lsize)]),
                                                                   abs(ifft[optshift_ind][wraparound(n-1,lsize)]));
                    }
                  }
                  break;
                case interp_t::HERMITE_COMPLEX:
                  // TODO Attempt a Hermite interpolator on real & imaginary components
                  // May be only appropriate to do this if the input data are complex;
                  //   real & imaginary components of iFFT can't be trusted if the FFT was done on magnitude only
                  // Can then have opportunity to modulate tension parameter of hermite interpolator:
                  //   smoothly vary between cubic and linear
                  //   (access via command-line, but also consider seeking optimal as a function of shift)
                  {
                    if (shift > 0.0) {
                      hermite_complex.set (shift);
                      output.value() += scale * hermite_complex.value (ifft[optshift_ind][wraparound(n+1,lsize)],
                                                                       ifft[optshift_ind][wraparound(n,  lsize)],
                                                                       ifft[optshift_ind][wraparound(n-1,lsize)],
                                                                       ifft[optshift_ind][wraparound(n-2,lsize)]);
                    } else {
                      hermite_complex.set (1.0 + shift);
                      output.value() += scale * hermite_complex.value (ifft[optshift_ind][wraparound(n+2,lsize)],
                                                                       ifft[optshift_ind][wraparound(n+1,lsize)],
                                                                       ifft[optshift_ind][wraparound(n,  lsize)],
                                                                       ifft[optshift_ind][wraparound(n-1,lsize)]);
                    }
                  }
                  break;
              }
            }

          }


        protected:
          const size_t axis;
          const int lsize;
          const double scale;
          ImageType input;
          OutputImageType output;
          interp_t interp;
          Math::Hermite<double> hermite_mag;
          Math::Hermite<cdouble> hermite_complex;
          const int minW, maxW, num_shifts;
          vector<double> shift_ind;
          Math::FFT1D fft;
          vector<Math::FFT1D> ifft;


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

              const double tot_var = std::min(sum_left,sum_right);

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

    }




      class Unring3DFunctor
      { MEMALIGN (Unring3DFunctor)
        public:
          Unring3DFunctor (Header header, const interp_t interp, const int minW = 1, const int maxW = 3, const int num_shifts = 20) :
            interp (interp), minW (minW), maxW (maxW), num_shifts (num_shifts) {
              header.ndim() = 3;
              header.datatype() = DataType::CFloat32;

              // set up scratch images:
              vol_FT = ImageType::scratch (header, "FFT of input volume");
              vol_filtered = ImageType::scratch (header, "filtered volume");
            }

          template <class VolumeIn, class VolumeOut>
            void operator() (ProgressBar& progress, VolumeIn& input, VolumeOut& output) {
              // full 3D FFT of input:
              INFO ("performing initial 3D forward Fourier transform...");
              Math::FFT (input, vol_FT, 0, FFTW_FORWARD);
              Math::FFT (vol_FT, vol_FT, 1, FFTW_FORWARD);
              Math::FFT (vol_FT, vol_FT, 2, FFTW_FORWARD);

              for (int axis = 0; axis < 3; ++axis) {

                // filter along x:
                INFO ("filtering for axis "+str(axis)+"...");
                ThreadedLoop(vol_FT).run (Filter(axis), vol_FT, vol_filtered);

                // then inverse FT back to image domain:
                INFO ("applying 3D backward Fourier transform...");
                Math::FFT (vol_filtered, vol_filtered, 0, FFTW_BACKWARD);
                Math::FFT (vol_filtered, vol_filtered, 1, FFTW_BACKWARD);
                Math::FFT (vol_filtered, vol_filtered, 2, FFTW_BACKWARD);

                // apply unringing operation on desired axis:
                INFO ("performing unringing along axis "+str(axis)+"...");
                ThreadedLoop (vol_filtered, strides_for_axis (axis))
                  .run_outer (LineProcessor<VolumeOut> (axis, vol_filtered, output, interp, minW, maxW, num_shifts));

                ++progress;
              }

            }


        private:
          const interp_t interp;
          const int minW, maxW, num_shifts;
          ImageType vol_FT, vol_filtered;

      };



    template <class ImageIn, class ImageOut>
      void unring3D (ImageIn& input, ImageOut& output, const interp_t interp, const int minW = 1, const int maxW = 3, const int num_shifts = 20)
      {
        size_t nvol = 1;
        for (size_t n = 3; n < input.ndim(); ++n)
          nvol *= input.size(n);
        ProgressBar progress ("performing 3D Gibbs ringing removal", 3*nvol);

        // need a quick adapter to fool FFT into operating on single volume:
        class Volume : public ImageType { MEMALIGN (Volume)
          public:
            Volume (const ImageType& parent) : ImageType (parent) { }
            size_t ndim () const { return 3; }
        };

        Unring3DFunctor unring (Header (input), interp, minW, maxW, num_shifts);

        if (input.ndim() <= 3) {
          Volume vol_in (input);
          unring (progress, vol_in, output);
          return;
        }

        // loop over all volumes in input dataset:
        for (auto l = Loop (3, input.ndim())(input, output); l; l++) {

            std::string vol_idx;
            for (size_t n = 3; n < input.ndim(); ++n)
            vol_idx += str(input.index(n)) + " ";
          if (vol_idx.size())
            INFO ("processing volume [ " + vol_idx + "]");

          Volume vol_in (input);
          unring (progress, vol_in, output);
        }


      }
  }
}

#endif

