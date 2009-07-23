/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __math_SH_h__
#define __math_SH_h__

#include "point.h"
#include "math/legendre.h"
#include "math/matrix.h"
#include "math/least_squares.h"

#define MAX_DIR_CHANGE 0.2
#define ANGLE_TOLERANCE 1e-4

namespace MR {
  namespace Math {
    namespace SH {

      inline size_t NforL (int lmax) { return ((lmax+1)*(lmax+2)/2); }
      inline size_t index (int l, int m) { return (l*(l+1)/2 + m); }

      inline size_t NforL_mpos (int lmax) { return ((lmax/2+1)*(lmax/2+1)); }
      inline size_t index_mpos (int l, int m) { return (l*l/4 + m); }

      inline size_t LforN (int N) { return (N ? 2 * floor<size_t>((sqrt(float(1+8*N))-3.0)/4.0) : 0); }

      template <typename T> Math::Matrix<T>& init_transform (Math::Matrix<T>& SHT, const Math::Matrix<T>& dirs, int lmax)
      {
        if (dirs.columns() != 2) throw Exception ("direction matrix should have 2 columns: [ azimuth elevation ]");
        SHT.allocate (dirs.rows(), NforL (lmax));
        T AL [lmax+1];
        for (size_t i = 0; i < dirs.rows(); i++) {
          T x = cos (dirs(i,1));
          Legendre::Plm_sph (AL, lmax, 0, x);
          for (int l = 0; l <= lmax; l+=2) SHT(i,index(l,0)) = AL[l];
          for (int m = 1; m <= lmax; m++) {
            Legendre::Plm_sph (AL, lmax, m, x);
            for (int l = ((m&1) ? m+1 : m); l <= lmax; l+=2) {
              SHT(i,index(l, m)) = AL[l]*cos(m*dirs(i,0));
              SHT(i,index(l,-m)) = AL[l]*sin(m*dirs(i,0));
            }
          }
        }
        return (SHT);
      }


      template <typename T> class Transform {
        public:
          Transform (const Math::Matrix<T>& dirs, int lmax)   { init_transform (SHT, dirs, lmax); iSHT.allocate (SHT.columns(), SHT.rows()); Math::pinv (iSHT, SHT); }
          void set_filter (const Math::Vector<T>& filter)
          { 
            int l = 0;
            uint nl = 1;
            for (size_t n = 0; n < iSHT.rows(); n++) {
              if (n >= nl) { l++; nl = NforL(2*l); }
              for (size_t i = 0; i < iSHT.columns(); i++) {
                iSHT(n,i) *= filter[l];
                SHT(i,n) = filter[l] == 0.0 ? 0.0 : SHT(i,n)/filter[l];
              }
            }
          }
          void A2SH (Math::Vector<T>& SH, const Math::Vector<T>& amplitudes)  { Math::mult (SH, iSHT, amplitudes); }
          void SH2A (Math::Vector<T>& amplitudes, const Math::Vector<T>& SH)  { Math::mult (amplitudes, SHT, SH); }

          size_t n_SH () const { return (SHT.columns()); }
          size_t n_amp () const  { return (SHT.rows()); }

          const Math::Matrix<T>& mat_A2SH () const { return (iSHT); }
          const Math::Matrix<T>& mat_SH2A () const { return (SHT); }

        protected:
          Math::Matrix<T> SHT, iSHT;
      };



      template <typename T> inline T value (const T* val, const Point& unit_dir, int lmax)
      {
        T value = 0.0;
        T az = atan2 (unit_dir[1], unit_dir[0]);
        T AL [lmax+1];
        Legendre::Plm_sph (AL, lmax, 0, T(unit_dir[2]));
        for (int l = 0; l <= lmax; l+=2) value += AL[l] * val[index(l,0)];
        for (int m = 1; m <= lmax; m++) {
          Legendre::Plm_sph (AL, lmax, m, T(unit_dir[2]));
          T c = Math::cos (m*az);
          T s = Math::sin (m*az);
          for (int l = ((m&1) ? m+1 : m); l <= lmax; l+=2) 
            value += AL[l] * (c * val[index(l,m)] + s * val[index(l,-m)]);
        }
        return (value);
      }


      template <typename T> inline Math::VectorView<T>& delta (Math::VectorView<T>& D, const Point& unit_dir, int lmax)
      {
        T az = Math::atan2 (unit_dir[1], unit_dir[0]);
        T AL [lmax+1];
        Legendre::Plm_sph (AL, lmax, 0, T(unit_dir[2]));
        for (int l = 0; l <= lmax; l+=2) D[index(l,0)] = AL[l];
        for (int m = 1; m <= lmax; m++) {
          Legendre::Plm_sph (AL, lmax, m, T(unit_dir[2]));
          T c = Math::cos (m*az);
          T s = Math::sin (m*az);
          for (int l = ((m&1) ? m+1 : m); l <= lmax; l+=2) {
            D[index(l,m)]  = 2.0 * AL[l] * c;
            D[index(l,-m)] = 2.0 * AL[l] * s;
          }
        }
        return (D);
      }


      template <typename T> inline Math::Vector<T>& delta (Math::Vector<T>& D, const Point& unit_dir, int lmax)
      {
        D.allocate (NforL (lmax));
        delta (D.view(), unit_dir, lmax);
        return (D);
      }


      template <typename T> inline Math::Vector<T>& SH2RH (Math::Vector<T>& RH, const Math::Vector<T>& SH)
      {
        RH.allocate (SH.size());
        int lmax = 2*SH.size()+1;
        T AL [lmax+1];
        Legendre::Plm_sph (AL, lmax, 0, T(1.0));
        for (size_t l = 0; l < SH.size(); l++) RH[l] = SH[l]/ AL[2*l]; 
        return (RH);
      }



      template <typename T> inline Math::Vector<T>& FA2SH (Math::Vector<T>& SH, T FA, T ADC, T bvalue, int lmax, int precision = 100)
      {
        T a = FA/sqrt(3.0 - 2.0*FA*FA);
        T ev1 = ADC*(1.0+2.0*a), ev2 = ADC*(1.0-a);

        Math::Vector<T> sigs (precision);
        Math::Matrix<T> SHT (precision, lmax/2+1);
        T AL [lmax+1];

        for (int i = 0; i < precision; i++) {
          T el = i*M_PI/(2.0*(precision-1));
          sigs[i] = exp(-bvalue*(ev1*cos(el)*cos(el) + ev2*sin(el)*sin(el)));
          Legendre::Plm_sph (AL, lmax, 0, cos(el));
          for (int l = 0; l < lmax/2+1; l++) SHT(i,l) = AL[2*l];
        }

        Math::Matrix<T> SHinv (SHT.columns(), SHT.rows());
        return (Math::mult (SH, pinv(SHinv, SHT), sigs));
      }




      template <typename T > class PrecomputedFraction 
      {
        public:
          PrecomputedFraction () : f1 (0.0), f2 (0.0), p1 (NULL), p2 (NULL) { }
          T f1, f2, *p1, *p2;
      };


      template <typename T> class PrecomputedAL 
      {
        public:
          PrecomputedAL (int up_to_lmax, int num_dir) :
            lmax (up_to_lmax), 
            ndir (num_dir),
            nAL (NforL_mpos(lmax)),
            inc (M_PI/(ndir-1)),
            AL (new T [ndir*nAL]) {
              T buf [lmax+1];
              for (int n = 0; n < ndir; n++) {
                T* p = AL + n*nAL;
                T cos_el = Math::cos (n*inc);
                for (int m = 0; m <= lmax; m++) {
                  Legendre::Plm_sph (buf, lmax, m, cos_el);
                  for (int l = ((m&1)?m+1:m); l <= lmax; l+=2) 
                    p[index_mpos(l,m)] = buf[l];
                }
              }
            }

          ~PrecomputedAL () { delete [] AL; }

          bool operator! () const { return (!AL); }
          bool ready () const { return (AL); }

          void set (PrecomputedFraction<T>& f, const T elevation) const {
            f.f2 = elevation / inc;
            int i = int (f.f2);
            if (i < 0) { i = 0; f.f1 = 1.0; f.f2 = 0.0; }
            else if (i >= ndir-1) { i = ndir-1; f.f1 = 1.0; f.f2 = 0.0; }
            else { f.f2 -= i; f.f1 = 1.0 - f.f2; }

            f.p1 = AL + i*nAL;
            f.p2 = f.p1 + nAL;
          }

          T get (const PrecomputedFraction<T>& f, int i) const { T v = f.f1*f.p1[i]; if (f.f2) v += f.f2*f.p2[i]; return (v); }
          T get (const PrecomputedFraction<T>& f, int l, int m) const { return (get (f, index_mpos(l,m))); }

          void get (T* dest, const PrecomputedFraction<T>& f) const
          {
            for (int l = 0; l <= lmax; l+=2) {
              for (int m = 0; m < l; m++) {
                int i = index_mpos(l,m);
                dest[i] = get (f,i);
              }
            }
          }

          T value (const T* val, const Point& unit_dir)
          {
            PrecomputedFraction<T> f;
            set (f, Math::acos (unit_dir[2]));
            T az = Math::atan2 (unit_dir[1], unit_dir[0]);
            T v = 0.0;
            for (int l = 0; l <= lmax; l+=2) v += get (f,l,0) * val[index(l,0)];
            for (int m = 1; m <= lmax; m++) {
              T c = Math::cos (m*az);
              T s = Math::sin (m*az);
              for (int l = ((m&1) ? m+1 : m); l <= lmax; l+=2) 
                v += get(f,l,m) * (c * val[index(l,m)] + s * val[index(l,-m)]);
            }
            return (v);
          }

        protected:
          int lmax, ndir, nAL;
          T inc, *AL;
      };






      template <typename T> inline T get_peak (const T* SH, int lmax, Point& unit_init_dir, PrecomputedAL<T>* precomputer = NULL)
      {
        assert (unit_init_dir.valid());
        for (int i = 0; i < 50; i++) {
          T az = atan2 (unit_init_dir[1], unit_init_dir[0]);
          T el = acos (unit_init_dir[2]);
          T amplitude, dSH_del, dSH_daz, d2SH_del2, d2SH_deldaz, d2SH_daz2;
          derivatives (SH, lmax, el, az, amplitude, dSH_del, dSH_daz, d2SH_del2, d2SH_deldaz, d2SH_daz2, precomputer);

          T del = sqrt (dSH_del*dSH_del + dSH_daz*dSH_daz);
          T daz = dSH_daz/del;
          del = dSH_del/del;

          T dSH_dt = daz*dSH_daz + del*dSH_del;
          T d2SH_dt2 = daz*daz*d2SH_daz2 + 2.0*daz*del*d2SH_deldaz + del*del*d2SH_del2;
          T dt = - dSH_dt / d2SH_dt2;

          if (dt < 0.0 || dt > MAX_DIR_CHANGE) dt = MAX_DIR_CHANGE;

          del *= dt;
          daz *= dt;

          unit_init_dir += Point (del*cos(az)*cos(el) - daz*sin(az), del*sin(az)*cos(el) + daz*cos(az), -del*sin(el));
          unit_init_dir.normalise();

          if (dt < ANGLE_TOLERANCE) return (amplitude);
        }

        unit_init_dir.invalidate();
        info ("failed to find SH peak!");
        return (NAN);
      }







      template <typename T> inline void derivatives (const T *SH, const int lmax, const T elevation, const T azimuth, T &amplitude,
          T &dSH_del, T &dSH_daz, T &d2SH_del2, T &d2SH_deldaz, T &d2SH_daz2, PrecomputedAL<T>* precomputer)
      {
        T sel = sin (elevation);
        T cel = cos (elevation);
        bool atpole = sel < 1e-4;

        dSH_del = dSH_daz = d2SH_del2 = d2SH_deldaz = d2SH_daz2 = 0.0;
        T AL [NforL_mpos(lmax)];

        if (precomputer) {
          PrecomputedFraction<T> f;
          precomputer->set (f, elevation);
          precomputer->get (AL, f);
        }
        else {
          T buf [lmax+1];
          for (int m = 0; m <= lmax; m++) {
            Legendre::Plm_sph (buf, lmax, m, cel);
            for (int l = ((m&1)?m+1:m); l <= lmax; l+=2) 
              AL[index_mpos(l,m)] = buf[l];
          }
        }

        amplitude = SH[0] * AL[0];
        for (int l = 1; l <= (int) lmax; l+=2) {
          const T& v (SH[index(l,0)]);
          amplitude += v * AL[index_mpos(l,0)];
          dSH_del += v * sqrt(T(l*(l+1))) * AL[index_mpos(l,1)];
          d2SH_del2 += v * (sqrt(T(l*(l+1)*(l-1)*(l+2))) * AL[index_mpos(l,2)] - l*(l+1) * AL[index_mpos(l,0)])/2.0;
        }

        for (int m = 1; m <= lmax; m++) {
          T caz = cos (m*azimuth);
          T saz = sin (m*azimuth);
          for (int l = ((m&1) ? m+1 : m); l <= lmax; l+=2) {
            const T& vp (SH[index(l,m)]);
            const T& vm (SH[index(l,-m)]);
            amplitude += (vp*caz + vm*saz) * AL[index_mpos(l,m)];

            T tmp = sqrt(T((l+m)*(l-m+1))) * AL[index_mpos(l,m-1)];
            if (l > m) tmp -= sqrt(T((l-m)*(l+m+1))) * AL[index_mpos(l,m+1)];
            tmp /= -2.0;
            dSH_del += (vp*caz + vm*saz) * tmp;

            T tmp2 = - ((l+m)*(l-m+1) + (l-m)*(l+m+1)) * AL[index_mpos(l,m)];
            if (m == 1) tmp2 -= sqrt(T((l+m)*(l-m+1)*(l+m-1)*(l-m+2))) * AL[index_mpos(l,1)];
            if (l > m+1) tmp2 += sqrt(T((l-m)*(l+m+1)*(l-m-1)*(l+m+2))) * AL[index_mpos(l,m+2)];
            tmp2 /= 4.0;
            d2SH_del2 += (vp*caz + vm*saz) * tmp2;

            if (atpole) dSH_daz += (vm*caz - vp*saz) * tmp;
            else {
              d2SH_deldaz += m * (vm*caz - vp*saz) * tmp;
              dSH_daz += m * (vm*caz - vp*saz) * AL[index_mpos(l,m)];
              d2SH_daz2 -= (vp*caz + vm*saz) * m*m * AL[index_mpos(l,m)];
            }

          }
        }

        if (!atpole) {
          dSH_daz /= sel;
          d2SH_deldaz /= sel;
          d2SH_daz2 /= sel*sel;
        }
      }


    }
  }
}

#endif
