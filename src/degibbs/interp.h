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

#ifndef __degibbs_interp_h__
#define __degibbs_interp_h__


#include "types.h"
#include "math/fft.h"
#include "math/hermite.h"


namespace MR {
  namespace Degibbs {
    namespace Interp {



      namespace
      {
        // ensures n is never out-of-bounds
        inline int wraparound(ssize_t n, ssize_t size) {
          n = (n+size)%size;
          return n;
        }
      }
    


      class Base
      { NOMEMALIGN
        public:
          virtual cdouble operator() (const Math::FFT1D&, const size_t, const double) const = 0;
      };

      class LinearMagnitude : public Base
      { NOMEMALIGN
        public:
          cdouble operator() (const Math::FFT1D& ifft, const size_t n, const double shift) const final
          {
            // Calculate magnitudes of shifted values, then linear interpolation on those
            // TODO Should linear interpolation when input data are magnitude only be
            //   done strictly on magnitude by default, rather than complex and then taking the magnitude?
            const double a1r = abs(ifft[n]);
            if (shift > 0.0) {
              const double a0r = abs(ifft[wraparound(n-1, ifft.size())]);
              return ((1.0-shift)*a1r + shift*a0r);
            } else {
              const double a2r = abs(ifft[wraparound(n+1, ifft.size())]);
              return ((1.0+shift)*a1r - shift*a2r);
            }
          }
      };

      class LinearComplex : public Base
      { NOMEMALIGN
        public:
          cdouble operator() (const Math::FFT1D& ifft, const size_t n, const double shift) const final
          {
            // Code from Donald's original implementation
            const cdouble a1r = ifft[n];
            if (shift > 0.0) {
              const cdouble a0r = ifft[wraparound(n-1, ifft.size())];
              return ((1.0-shift)*a1r + shift*a0r);
            } else {
              const cdouble a2r = ifft[wraparound(n+1, ifft.size())];
              return ((1.0+shift)*a1r - shift*a2r);
            }
          }
      };

      class LinearPseudopolar : public Base
      { NOMEMALIGN
        public:
          cdouble operator() (const Math::FFT1D& ifft, const size_t n, const double shift) const final
          {
            const cdouble a1r = ifft[n];
            cdouble sum_complex = (1.0 - abs(shift)) * a1r;
            double sum_mag = abs(sum_complex);
            if (shift > 0.0) {
              const cdouble a0r = ifft[wraparound(n-1, ifft.size())];
              sum_mag += shift*abs(a0r);
              sum_complex += shift*a0r;
            } else {
              const cdouble a2r = abs(ifft[wraparound(n+1, ifft.size())];
              sum_mag -= shift*abs(a2r);
              sum_complex -= shift*a2r;
            }
            return (sum_complex * sum_mag / abs(sum_complex));
          }
      };

      // Do genuine linear interpolations of magnitude and phase separately,
      //   then reconstruct the final complex value from that
      class LinearPolar : public Base
      { NOMEMALIGN
        public:
          cdouble operator() (const Math::FFT1D& ifft, const size_t n, const double shift) const final
          {
            
            {
              const cdouble a1r = ifft[n];
              const double phase_a1r = std::arg (a1r);
              double sum_mag = (1.0 - abs(shift)) * abs(a1r);
              double sum_phase = (1.0 - abs(shift)) * phase_a1r;
              if (shift > 0.0) {
                const cdouble a0r = ifft[wraparound(n-1, ifft.size())];
                double phase_a0r = std::arg (a0r);
                // TODO Need to double-check this
                if (phase_a0r - phase_a1r > Math::pi)
                  phase_a0r -= 2.0*Math::pi;
                else if (phase_a0r - phase_a1r < Math::pi)
                  phase_a0r += 2.0*Math::pi;
                sum_mag += shift*abs(a0r);
                sum_phase += shift*phase_a0r;
              } else {
                const cdouble a2r = ifft[wraparound(n+1, ifft.size())];
                double phase_a2r = std::arg (a2r);
                if (phase_a2r - phase_a1r > Math::pi)
                  phase_a2r -= 2.0*Math::pi;
                else if (phase_a2r - phase_a1r < Math::pi)
                  phase_a2r += 2.0*Math::pi;
                sum_mag -= shift*abs(a2r);
                sum_phase -= shift*phase_a2r;
              }
              return std::polar (sum_mag, sum_phase);
          }
      };

      // TODO Implement alternative polar interpolator where a weighted _geometric_ mean of magnitudes is taken

      // Perform Hermite interpolation on magnitude data only
      // Should recapitulate ringing in input magnitude data with insufficient tension
      class HermiteMagnitude : public Base
      { NOMEMALIGN
        public:
          HermiteMagnitude (const double T) : hermite (T) { }
          cdouble operator() (const Math::FFT1D& ifft, const size_t n, const double shift) const final
          {
            if (shift > 0.0) {
              hermite.set (shift);
              return hermite.value (abs(ifft[wraparound(n-1, ifft.size())]),
                                    abs(ifft[wraparound(n,   ifft.size())]),
                                    abs(ifft[wraparound(n+1, ifft.size())]),
                                    abs(ifft[wraparound(n+2, ifft.size())]));
            } else {
              hermite.set (1.0 + shift);
              return hermite.value (abs(ifft[wraparound(n-2, ifft.size())]),
                                    abs(ifft[wraparound(n-1, ifft.size())]),
                                    abs(ifft[wraparound(n,   ifft.size())]),
                                    abs(ifft[wraparound(n+1, ifft.size())]));
            }
          }
        private:
          Math::Hermite<double> hermite;
      };

      class HermiteComplex : public Base
      { NOMEMALIGN
        public:
          HermiteComplex (const double T) : hermite (T) { }
          cdouble operator() (const Math::FFT1D& ifft, const size_t n, const double shift) const final
          {
            if (shift > 0.0) {
              hermite.set (shift);
              return hermite.value (ifft[wraparound(n-1, ifft.size())],
                                    ifft[wraparound(n,   ifft.size())],
                                    ifft[wraparound(n+1, ifft.size())],
                                    ifft[wraparound(n+2, ifft.size())]);
            } else {
              hermite.set (1.0 + shift);
              return hermite.value (ifft[wraparound(n-2, ifft.size())],
                                    ifft[wraparound(n-1, ifft.size())],
                                    ifft[wraparound(n,   ifft.size())],
                                    ifft[wraparound(n+1, ifft.size())]);
            }
          }
        private:
          Math::Hermite<cdouble> hermite;
      };



    }
  }
}

#endif

