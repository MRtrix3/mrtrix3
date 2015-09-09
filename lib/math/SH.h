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

#ifdef USE_NON_ORTHONORMAL_SH_BASIS
# warning using non-orthonormal SH basis
#endif

#include "point.h"
#include "math/legendre.h"
#include "math/versor.h"
#include "math/matrix.h"
#include "math/least_squares.h"

#define MAX_DIR_CHANGE 0.2
#define ANGLE_TOLERANCE 1e-4

namespace MR
{
  namespace Math
  {
    namespace SH
    {

      /** \defgroup spherical_harmonics Spherical Harmonics
       * \brief Classes & functions to manage spherical harmonics. */

      /** \addtogroup spherical_harmonics
       * @{ */


      //! a string containing a description of the SH storage convention
      /*! This can used directly in the DESCRIPTION field of a command's
       * usage() function. */
      extern const char* encoding_description;

      //! the number of (even-degree) coefficients for the given value of \a lmax
      inline size_t NforL (int lmax)
      {
        return (lmax+1) * (lmax+2) /2;
      }

      //! compute the index for coefficient (l,m)
      inline size_t index (int l, int m)
      {
        return l* (l+1) /2 + m;
      }

      //! same as NforL(), but consider only non-negative orders \e m
      inline size_t NforL_mpos (int lmax)
      {
        return (lmax/2+1) * (lmax/2+1);
      }
      
      //! same as index(), but consider only non-negative orders \e m
      inline size_t index_mpos (int l, int m)
      {
        return l*l/4 + m;
      }

      //! returns the largest \e lmax given \a N parameters 
      inline size_t LforN (int N)
      {
        return N ? 2 * std::floor<size_t> ( (std::sqrt (float (1+8*N))-3.0) /4.0) : 0;
      }

      //! form the SH->amplitudes matrix 
      /*! This computes the matrix \a SHT mapping spherical harmonic
       * coefficients up to maximum harmonic degree \a lmax onto directions \a
       * dirs (in spherical coordinates, with columns [ azimuth elevation ]). */
      template <typename ValueType>
        Matrix<ValueType>& init_transform (Matrix<ValueType>& SHT, const Matrix<ValueType>& dirs, int lmax)
        {
          if (dirs.columns() != 2) throw Exception ("direction matrix should have 2 columns: [ azimuth elevation ]");
          SHT.allocate (dirs.rows(), NforL (lmax));
          VLA_MAX (AL, ValueType, lmax+1, 64);
          for (size_t i = 0; i < dirs.rows(); i++) {
            ValueType x = std::cos (dirs (i,1));
            Legendre::Plm_sph (AL, lmax, 0, x);
            for (int l = 0; l <= lmax; l+=2) SHT (i,index (l,0)) = AL[l];
            for (int m = 1; m <= lmax; m++) {
              Legendre::Plm_sph (AL, lmax, m, x);
              for (int l = ( (m&1) ? m+1 : m); l <= lmax; l+=2) {
#ifndef USE_NON_ORTHONORMAL_SH_BASIS
                SHT (i,index (l, m)) = Math::sqrt2 * AL[l]*std::cos (m*dirs (i,0));
                SHT (i,index (l,-m)) = Math::sqrt2 * AL[l]*std::sin (m*dirs (i,0));
#else
                SHT (i,index (l, m)) = AL[l]*std::cos (m*dirs (i,0));
                SHT (i,index (l,-m)) = AL[l]*std::sin (m*dirs (i,0));
#endif
              }
            }
          }
          return SHT;
        }


      //! \copydoc init_transform()
      template <typename ValueType>
        Matrix<ValueType> init_transform (const Matrix<ValueType>& dirs, int lmax)
        {
          Matrix<ValueType> SHT;
          init_transform (SHT, dirs, lmax);
          return SHT;
        }




      //! scale the coefficients of each SH degree by the corresponding value in \a coefs
      template <typename ValueType>
        void scale_degrees_forward (Matrix<ValueType>& SH2amp_mapping, const Vector<ValueType>& coefs) 
        {
          size_t l = 0, nl = 1;
          for (size_t col = 0; col < SH2amp_mapping.columns(); ++col) {
            if (col >= nl) {
              l++;
              nl = NforL (2*l);
            }
            SH2amp_mapping.column(col) *= coefs[l];
          }
        }



      //! scale the coefficients of each SH degree by the corresponding value in \a coefs
      template <typename ValueType>
        void scale_degrees_inverse (Matrix<ValueType>& amp2SH_mapping, const Vector<ValueType>& coefs) 
        {
          size_t l = 0, nl = 1;
          for (size_t row = 0; row < amp2SH_mapping.rows(); ++row) {
            if (row >= nl) {
              l++;
              nl = NforL (2*l);
            }
            amp2SH_mapping.row(row) *= coefs[l];
          }
        }

      //! invert any non-zero coefficients in \a coefs
      template <typename ValueType>
        Vector<ValueType> invert (const Vector<ValueType>& coefs)
        {
          Vector<ValueType> ret (coefs.size());
          for (size_t n = 0; n < coefs.size(); ++n)
            ret[n] = ( coefs[n] ? 1.0 / coefs[n] : 0.0 );
          return ret;
        }


      template <typename ValueType>
        class Transform {
          public:
            Transform (const Matrix<ValueType>& dirs, int lmax) :
              SHT (init_transform (dirs, lmax)), 
              iSHT (pinv (SHT)) { }

            void set_filter (const Vector<ValueType>& filter) {
              scale_degrees_forward (SHT, invert (filter));
              scale_degrees_inverse (iSHT, filter);
            }
            void A2SH (Vector<ValueType>& sh, const Vector<ValueType>& amplitudes) const {
              mult (sh, iSHT, amplitudes);
            }
            void SH2A (Vector<ValueType>& amplitudes, const Vector<ValueType>& sh) const {
              mult (amplitudes, SHT, sh);
            }

            size_t n_SH () const {
              return SHT.columns();
            }
            size_t n_amp () const  {
              return SHT.rows();
            }

            const Matrix<ValueType>& mat_A2SH () const {
              return iSHT;
            }
            const Matrix<ValueType>& mat_SH2A () const {
              return SHT;
            }

          protected:
            Matrix<ValueType> SHT, iSHT;
        };


        
      template <typename ValueType, typename CoefType>
        inline ValueType value (const CoefType& coefs, ValueType cos_elevation, ValueType cos_azimuth, ValueType sin_azimuth, int lmax)
        {
          ValueType amplitude = 0.0;
          VLA_MAX (AL, ValueType, lmax+1, 64);
          Legendre::Plm_sph (AL, lmax, 0, ValueType (cos_elevation));
          for (int l = 0; l <= lmax; l+=2) 
            amplitude += AL[l] * coefs[index (l,0)];
          ValueType c0 (1.0), s0 (0.0);
          for (int m = 1; m <= lmax; m++) {
            Legendre::Plm_sph (AL, lmax, m, ValueType (cos_elevation));
            ValueType c = c0 * cos_azimuth - s0 * sin_azimuth;  // std::cos(m*azimuth)
            ValueType s = s0 * cos_azimuth + c0 * sin_azimuth;  // std::sin(m*azimuth)
            for (int l = ( (m&1) ? m+1 : m); l <= lmax; l+=2) {
#ifndef USE_NON_ORTHONORMAL_SH_BASIS
              amplitude += AL[l] * Math::sqrt2 * (c * coefs[index (l,m)] + s * coefs[index (l,-m)]);
#else
              amplitude += AL[l] * (c * coefs[index (l,m)] + s * coefs[index (l,-m)]);
#endif
            }
            c0 = c;
            s0 = s;
          }
          return amplitude;
        }

      template <typename ValueType, typename CoefType>
        inline ValueType value (const CoefType& coefs, ValueType cos_elevation, ValueType azimuth, int lmax)
        {
          return value (coefs, cos_elevation, std::cos(azimuth), std::sin(azimuth), lmax);
        }

      template <typename ValueType, typename CoefType>
        inline ValueType value (const CoefType& coefs, const Point<ValueType>& unit_dir, int lmax)
        {
          ValueType rxy = std::sqrt ( pow2(unit_dir[1]) + pow2(unit_dir[0]) );
          ValueType cp = (rxy) ? unit_dir[0]/rxy : 1.0;
          ValueType sp = (rxy) ? unit_dir[1]/rxy : 0.0;
          return value (coefs, unit_dir[2], cp, sp, lmax);
        }

      template <typename ValueType, typename CoefType>
        inline ValueType value (const CoefType* coefs, const Point<ValueType>& unit_dir, int lmax)
        {
          ValueType rxy = std::sqrt ( pow2(unit_dir[1]) + pow2(unit_dir[0]) );
          ValueType cp = (rxy) ? unit_dir[0]/rxy : 1.0;
          ValueType sp = (rxy) ? unit_dir[1]/rxy : 0.0;
          return value (coefs, unit_dir[2], cp, sp, lmax);
        }


      template <typename ValueType>
        inline Vector<ValueType>& delta (Vector<ValueType>& delta_vec, const Point<ValueType>& unit_dir, int lmax)
        {
          delta_vec.allocate (NforL (lmax));
          ValueType rxy = std::sqrt ( pow2(unit_dir[1]) + pow2(unit_dir[0]) );
          ValueType cp = (rxy) ? unit_dir[0]/rxy : 1.0;
          ValueType sp = (rxy) ? unit_dir[1]/rxy : 0.0;
          VLA_MAX (AL, ValueType, lmax+1, 64);
          Legendre::Plm_sph (AL, lmax, 0, ValueType (unit_dir[2]));
          for (int l = 0; l <= lmax; l+=2)
            delta_vec[index (l,0)] = AL[l];
          ValueType c0 (1.0), s0 (0.0);
          for (int m = 1; m <= lmax; m++) {
            Legendre::Plm_sph (AL, lmax, m, ValueType (unit_dir[2]));
            ValueType c = c0 * cp - s0 * sp;
            ValueType s = s0 * cp + c0 * sp;
            for (int l = ( (m&1) ? m+1 : m); l <= lmax; l+=2) {
#ifndef USE_NON_ORTHONORMAL_SH_BASIS
              delta_vec[index (l,m)]  = AL[l] * Math::sqrt2 * c;
              delta_vec[index (l,-m)] = AL[l] * Math::sqrt2 * s;
#else
              delta_vec[index (l,m)]  = AL[l] * 2.0 * c;
              delta_vec[index (l,-m)] = AL[l] * 2.0 * s;
#endif
            }
            c0 = c;
            s0 = s;
          }
          return delta_vec;
        }



      template <typename ValueType>
        inline Vector<ValueType>& SH2RH (Vector<ValueType>& RH, const Vector<ValueType>& sh)
        {
          RH.allocate (sh.size());
          int lmax = 2*sh.size() +1;
          VLA_MAX (AL, ValueType, lmax+1, 64);
          Legendre::Plm_sph (AL, lmax, 0, ValueType (1.0));
          for (size_t l = 0; l < sh.size(); l++)
            RH[l] = sh[l]/ AL[2*l];
          return RH;
        }

      template <typename ValueType>
        inline Vector<ValueType> SH2RH (const Vector<ValueType>& sh)
        {
          Vector<ValueType> RH (sh.size());
          int lmax = 2*sh.size() +1;
          ValueType AL [lmax+1];
          Legendre::Plm_sph (AL, lmax, 0, ValueType (1.0));
          for (size_t l = 0; l < sh.size(); l++)
            RH[l] = sh[l]/ AL[2*l];
          return RH;
        }



      //! perform spherical convolution
      /*! perform spherical convolution of SH coefficients \a sh with response
       * function \a RH, storing the results in vector \a C. */
      template <typename ValueType>
        inline Vector<ValueType>& sconv (Vector<ValueType>& C, const Vector<ValueType>& RH, const Vector<ValueType>& sh)
        {
          assert (sh.size() >= NforL (2* (RH.size()-1)));
          C.allocate (NforL (2* (RH.size()-1)));
          for (int i = 0; i < int (RH.size()); ++i) {
            int l = 2*i;
            for (int m = -l; m <= l; ++m)
              C[index (l,m)] = RH[i] * sh[index (l,m)];
          }
          return C;
        }


      namespace {
        template <typename> struct __dummy { typedef int type; };
      }


      //! convert spherical coordinates to Cartesian coordinates
      template <class VectorType1, class VectorType2>
        inline void s2c (const VectorType1& az_el_r, VectorType2&& xyz)
        {
          if (az_el_r.size() == 3) {
            xyz[0] = az_el_r[2] * std::sin (az_el_r[1]) * std::cos (az_el_r[0]);
            xyz[1] = az_el_r[2] * std::sin (az_el_r[1]) * std::sin (az_el_r[0]);
            xyz[2] = az_el_r[2] * cos (az_el_r[1]);
          }
          else {
            xyz[0] = std::sin (az_el_r[1]) * std::cos (az_el_r[0]);
            xyz[1] = std::sin (az_el_r[1]) * std::sin (az_el_r[0]);
            xyz[2] = cos (az_el_r[1]);
          }
        }


      //! convert matrix of spherical coordinates to Cartesian coordinates
      template <typename ValueType>
        inline void S2C (const Matrix<ValueType>& az_el, Matrix<ValueType>& cartesian)
        {
          cartesian.allocate (az_el.rows(), 3);
          for (size_t dir = 0; dir < az_el.rows(); ++dir) 
            s2c (az_el.row (dir), cartesian.row (dir));
        }
     
      //! convert matrix of spherical coordinates to Cartesian coordinates
      template <typename ValueType>
        inline void S2C (const Matrix<ValueType>& az_el, Matrix<ValueType>&& cartesian) {
          S2C (az_el, cartesian);
        }

      //! convert matrix of spherical coordinates to Cartesian coordinates
      template <typename ValueType>
        inline Math::Matrix<ValueType> S2C (const Matrix<ValueType>& az_el)
        {
          Math::Matrix<ValueType> tmp;
          Math::SH::S2C (az_el, tmp);
          return tmp;
        }



      //! convert Cartesian coordinates to spherical coordinates
      template <class VectorType1, class VectorType2>
        inline void c2s (const VectorType1& xyz, VectorType2&& az_el_r)
        {
          typename std::remove_reference<decltype(az_el_r[0])>::type r = std::sqrt (Math::pow2(xyz[0]) + Math::pow2(xyz[1]) + Math::pow2(xyz[2]));
          az_el_r[0] = std::atan2 (xyz[1], xyz[0]);
          az_el_r[1] = std::acos (xyz[2] / r);
          if (az_el_r.size() == 3) 
            az_el_r[2] = r;
        }

      //! convert matrix of Cartesian coordinates to spherical coordinates
      template <typename ValueType>
        inline void C2S (const Matrix<ValueType>& cartesian, Matrix<ValueType>& az_el, bool include_r = false)
        {
          az_el.allocate (cartesian.rows(), include_r ? 3 : 2);
          for (size_t dir = 0; dir < cartesian.rows(); ++dir) 
            c2s (cartesian.row (dir), az_el.row (dir));
        }
      //! convert matrix of Cartesian coordinates to spherical coordinates
      template <typename ValueType>
        inline void C2S (const Matrix<ValueType>& cartesian, Matrix<ValueType>&& az_el, bool include_r = false) {
          C2S (cartesian, az_el, include_r);
        }

      //! convert matrix of Cartesian coordinates to spherical coordinates
      template <typename ValueType>
        inline Math::Matrix<ValueType> C2S (const Matrix<ValueType>& cartesian, bool include_r = false)
        {
          Math::Matrix<ValueType> az_el;
          Math::SH::C2S (cartesian, az_el, include_r);
          return az_el;
        }




      //! compute axially-symmetric SH coefficients corresponding to specified tensor
      template <typename ValueType>
        inline Vector<ValueType>& FA2SH (
            Vector<ValueType>& sh, ValueType FA, ValueType ADC, ValueType bvalue, int lmax, int precision = 100)
      {
        ValueType a = FA/sqrt (3.0 - 2.0*FA*FA);
        ValueType ev1 = ADC* (1.0+2.0*a), ev2 = ADC* (1.0-a);

        Vector<ValueType> sigs (precision);
        Matrix<ValueType> SHT (precision, lmax/2+1);
        VLA_MAX (AL, ValueType, lmax+1, 64);

        for (int i = 0; i < precision; i++) {
          ValueType el = i*Math::pi / (2.0* (precision-1));
          sigs[i] = exp (-bvalue* (ev1*std::cos (el) *std::cos (el) + ev2*std::sin (el) *std::sin (el)));
          Legendre::Plm_sph (AL, lmax, 0, std::cos (el));
          for (int l = 0; l < lmax/2+1; l++) SHT (i,l) = AL[2*l];
        }

        Matrix<ValueType> SHinv (SHT.columns(), SHT.rows());
        return mult (sh, pinv (SHinv, SHT), sigs);
      }




      //! used to speed up SH calculation
      template <typename ValueType > class PrecomputedFraction
      {
        public:
          PrecomputedFraction () : f1 (0.0), f2 (0.0) { }
          ValueType f1, f2;
          typename std::vector<ValueType>::const_iterator p1, p2;
      };

#ifndef USE_NON_ORTHONORMAL_SH_BASIS
#define SH_NON_M0_SCALE_FACTOR (m ? Math::sqrt2 : 1.0)*
#else
#define SH_NON_M0_SCALE_FACTOR
#endif

      //! Precomputed Associated Legrendre Polynomials - used to speed up SH calculation
      template <typename ValueType> class PrecomputedAL
      {
        public:
          typedef ValueType value_type;

          PrecomputedAL () : lmax (0), ndir (0), nAL (0), inc (0.0) { }
          PrecomputedAL (int up_to_lmax, int num_dir = 512) {
            init (up_to_lmax, num_dir);
          }

          bool operator! () const {
            return AL.empty();
          }
          operator bool () const {
            return AL.size();
          }

          void init (int up_to_lmax, int num_dir = 512) {
            lmax = up_to_lmax;
            ndir = num_dir;
            nAL = NforL_mpos (lmax);
            inc = Math::pi / (ndir-1);
            AL.resize (ndir*nAL);
            VLA_MAX (buf, value_type, lmax+1, 64);

            for (int n = 0; n < ndir; n++) {
              typename std::vector<value_type>::iterator p = AL.begin() + n*nAL;
              value_type cos_el = std::cos (n*inc);
              for (int m = 0; m <= lmax; m++) {
                Legendre::Plm_sph (buf, lmax, m, cos_el);
                for (int l = ( (m&1) ?m+1:m); l <= lmax; l+=2)
                  p[index_mpos (l,m)] = SH_NON_M0_SCALE_FACTOR buf[l];
              }
            }
          }

          void set (PrecomputedFraction<ValueType>& f, const ValueType elevation) const {
            f.f2 = elevation / inc;
            int i = int (f.f2);
            if (i < 0) {
              i = 0;
              f.f1 = 1.0;
              f.f2 = 0.0;
            }
            else if (i >= ndir-1) {
              i = ndir-1;
              f.f1 = 1.0;
              f.f2 = 0.0;
            }
            else {
              f.f2 -= i;
              f.f1 = 1.0 - f.f2;
            }

            f.p1 = AL.begin() + i*nAL;
            f.p2 = f.p1 + nAL;
          }

          ValueType get (const PrecomputedFraction<ValueType>& f, int i) const {
            ValueType v = f.f1*f.p1[i];
            if (f.f2) v += f.f2*f.p2[i];
            return v;
          }
          ValueType get (const PrecomputedFraction<ValueType>& f, int l, int m) const {
            return get (f, index_mpos (l,m));
          }

          void get (ValueType* dest, const PrecomputedFraction<ValueType>& f) const {
            for (int l = 0; l <= lmax; l+=2) {
              for (int m = 0; m <= l; m++) {
                int i = index_mpos (l,m);
                dest[i] = get (f,i);
              }
            }
          }

          template <class ValueContainer>
            ValueType value (const ValueContainer& val, const Point<ValueType>& unit_dir) const {
              PrecomputedFraction<ValueType> f;
              set (f, std::acos (unit_dir[2]));
              ValueType rxy = std::sqrt ( pow2(unit_dir[1]) + pow2(unit_dir[0]) );
              ValueType cp = (rxy) ? unit_dir[0]/rxy : 1.0;
              ValueType sp = (rxy) ? unit_dir[1]/rxy : 0.0;
              ValueType v = 0.0;
              for (int l = 0; l <= lmax; l+=2)
                v += get (f,l,0) * val[index (l,0)];
              ValueType c0 (1.0), s0 (0.0);
              for (int m = 1; m <= lmax; m++) {
                ValueType c = c0 * cp - s0 * sp;
                ValueType s = s0 * cp + c0 * sp;
                for (int l = ( (m&1) ? m+1 : m); l <= lmax; l+=2)
                  v += get (f,l,m) * (c * val[index (l,m)] + s * val[index (l,-m)]);
                c0 = c;
                s0 = s;
              }
              return v;
            }

        protected:
          int lmax, ndir, nAL;
          ValueType inc;
          std::vector<ValueType> AL;
      };






      //! estimate direction & amplitude of SH peak
      /*! find a peak of an SH series using Gauss-Newton optimisation, modified
       * to operate directly in spherical coordinates. The initial search
       * direction is \a unit_init_dir. If \a precomputer is not nullptr, it
       * will be used to speed up the calculations, at the cost of a minor
       * reduction in accuracy. */
      template <typename ValueType>
        inline ValueType get_peak (const ValueType* sh, int lmax, 
            Point<ValueType>& unit_init_dir, PrecomputedAL<ValueType>* precomputer = nullptr)
        {
          assert (unit_init_dir.valid());
          for (int i = 0; i < 50; i++) {
            ValueType az = atan2 (unit_init_dir[1], unit_init_dir[0]);
            ValueType el = acos (unit_init_dir[2]);
            ValueType amplitude, dSH_del, dSH_daz, d2SH_del2, d2SH_deldaz, d2SH_daz2;
            derivatives (sh, lmax, el, az, amplitude, dSH_del, dSH_daz, d2SH_del2, d2SH_deldaz, d2SH_daz2, precomputer);

            ValueType del = sqrt (dSH_del*dSH_del + dSH_daz*dSH_daz);
            ValueType daz = dSH_daz/del;
            del = dSH_del/del;

            ValueType dSH_dt = daz*dSH_daz + del*dSH_del;
            ValueType d2SH_dt2 = daz*daz*d2SH_daz2 + 2.0*daz*del*d2SH_deldaz + del*del*d2SH_del2;
            ValueType dt = d2SH_dt2 ? (-dSH_dt / d2SH_dt2) : 0.0;

            if (dt < 0.0) dt = -dt;
            if (dt > MAX_DIR_CHANGE) dt = MAX_DIR_CHANGE;

            del *= dt;
            daz *= dt;

            unit_init_dir += Point<ValueType> (del*std::cos (az) *std::cos (el) - daz*std::sin (az), del*std::sin (az) *std::cos (el) + daz*std::cos (az), -del*std::sin (el));
            unit_init_dir.normalise();

            if (dt < ANGLE_TOLERANCE) return amplitude;
          }

          unit_init_dir.invalidate();
          DEBUG ("failed to find SH peak!");
          return NAN;
        }






      //! computes first and second order derivatives of SH series
      /*! This is used primarily in the get_peaks() function. */
      template <typename ValueType>
        inline void derivatives (
            const ValueType* sh, const int lmax, const ValueType elevation, const ValueType azimuth, ValueType& amplitude,
            ValueType& dSH_del, ValueType& dSH_daz, ValueType& d2SH_del2, ValueType& d2SH_deldaz,
            ValueType& d2SH_daz2, PrecomputedAL<ValueType>* precomputer)
        {
          ValueType sel = std::sin (elevation);
          ValueType cel = std::cos (elevation);
          bool atpole = sel < 1e-4;

          dSH_del = dSH_daz = d2SH_del2 = d2SH_deldaz = d2SH_daz2 = 0.0;
          VLA_MAX (AL, ValueType, NforL_mpos (lmax), 64);

          if (precomputer) {
            PrecomputedFraction<ValueType> f;
            precomputer->set (f, elevation);
            precomputer->get (AL, f);
          }
          else {
            VLA_MAX (buf, ValueType, lmax+1, 64);
            for (int m = 0; m <= lmax; m++) {
              Legendre::Plm_sph (buf, lmax, m, cel);
              for (int l = ( (m&1) ?m+1:m); l <= lmax; l+=2)
                AL[index_mpos (l,m)] = buf[l];
            }
          }

          amplitude = sh[0] * AL[0];
          for (int l = 2; l <= (int) lmax; l+=2) {
            const ValueType& v (sh[index (l,0)]);
            amplitude += v * AL[index_mpos (l,0)];
            dSH_del += v * sqrt (ValueType (l* (l+1))) * AL[index_mpos (l,1)];
            d2SH_del2 += v * (sqrt (ValueType (l* (l+1) * (l-1) * (l+2))) * AL[index_mpos (l,2)] - l* (l+1) * AL[index_mpos (l,0)]) /2.0;
          }

          for (int m = 1; m <= lmax; m++) {
#ifndef USE_NON_ORTHONORMAL_SH_BASIS
            ValueType caz = Math::sqrt2 * std::cos (m*azimuth);
            ValueType saz = Math::sqrt2 * std::sin (m*azimuth);
#else
            ValueType caz = std::cos (m*azimuth);
            ValueType saz = std::sin (m*azimuth);
#endif
            for (int l = ( (m&1) ? m+1 : m); l <= lmax; l+=2) {
              const ValueType& vp (sh[index (l,m)]);
              const ValueType& vm (sh[index (l,-m)]);
              amplitude += (vp*caz + vm*saz) * AL[index_mpos (l,m)];

              ValueType tmp = sqrt (ValueType ( (l+m) * (l-m+1))) * AL[index_mpos (l,m-1)];
              if (l > m) tmp -= sqrt (ValueType ( (l-m) * (l+m+1))) * AL[index_mpos (l,m+1)];
              tmp /= -2.0;
              dSH_del += (vp*caz + vm*saz) * tmp;

              ValueType tmp2 = - ( (l+m) * (l-m+1) + (l-m) * (l+m+1)) * AL[index_mpos (l,m)];
              if (m == 1) tmp2 -= sqrt (ValueType ( (l+m) * (l-m+1) * (l+m-1) * (l-m+2))) * AL[index_mpos (l,1)];
              else tmp2 += sqrt (ValueType ( (l+m) * (l-m+1) * (l+m-1) * (l-m+2))) * AL[index_mpos (l,m-2)];
              if (l > m+1) tmp2 += sqrt (ValueType ( (l-m) * (l+m+1) * (l-m-1) * (l+m+2))) * AL[index_mpos (l,m+2)];
              tmp2 /= 4.0;
              d2SH_del2 += (vp*caz + vm*saz) * tmp2;

              if (atpole) dSH_daz += (vm*caz - vp*saz) * tmp;
              else {
                d2SH_deldaz += m * (vm*caz - vp*saz) * tmp;
                dSH_daz += m * (vm*caz - vp*saz) * AL[index_mpos (l,m)];
                d2SH_daz2 -= (vp*caz + vm*saz) * m*m * AL[index_mpos (l,m)];
              }

            }
          }

          if (!atpole) {
            dSH_daz /= sel;
            d2SH_deldaz /= sel;
            d2SH_daz2 /= sel*sel;
          }
        }



      //! a class to hold the coefficients for an apodised point-spread function.
      template <typename ValueType> class aPSF
      {
        public:
          aPSF (const size_t _lmax) :
            lmax (_lmax)
          {
            switch (lmax) {
              case 2:
                RH.allocate (2);
                RH[0] = ValueType(1.00000000);
                RH[1] = ValueType(0.41939279);
                break;
              case 4:
                RH.allocate (3);
                RH[0] = ValueType(1.00000000);
                RH[1] = ValueType(0.63608543);
                RH[2] = ValueType(0.18487087);
                break;
              case 6:
                RH.allocate (4);
                RH[0] = ValueType(1.00000000);
                RH[1] = ValueType(0.75490341);
                RH[2] = ValueType(0.37126442);
                RH[3] = ValueType(0.09614699);
                break;
              case 8:
                RH.allocate (5);
                RH[0] = ValueType(1.00000000);
                RH[1] = ValueType(0.82384816);
                RH[2] = ValueType(0.51261696);
                RH[3] = ValueType(0.22440563);
                RH[4] = ValueType(0.05593079);
                break;
              case 10:
                RH.allocate (6);
                RH[0] = ValueType(1.00000000);
                RH[1] = ValueType(0.86725945);
                RH[2] = ValueType(0.61519436);
                RH[3] = ValueType(0.34570667);
                RH[4] = ValueType(0.14300355);
                RH[5] = ValueType(0.03548062);
                break;
              case 12:
                RH.allocate (7);
                RH[0] = ValueType(1.00000000);
                RH[1] = ValueType(0.89737759);
                RH[2] = ValueType(0.69278503);
                RH[3] = ValueType(0.45249879);
                RH[4] = ValueType(0.24169922);
                RH[5] = ValueType(0.09826171);
                RH[6] = ValueType(0.02502481);
                break;
              case 14:
                RH.allocate (8);
                RH[0] = ValueType(1.00000000);
                RH[1] = ValueType(0.91717853);
                RH[2] = ValueType(0.74685644);
                RH[3] = ValueType(0.53467773);
                RH[4] = ValueType(0.33031863);
                RH[5] = ValueType(0.17013825);
                RH[6] = ValueType(0.06810155);
                RH[7] = ValueType(0.01754930);
                break;
              case 16:
                RH.allocate (9);
                RH[0] = ValueType(1.00000000);
                RH[1] = ValueType(0.93261196);
                RH[2] = ValueType(0.79064858);
                RH[3] = ValueType(0.60562880);
                RH[4] = ValueType(0.41454703);
                RH[5] = ValueType(0.24880754);
                RH[6] = ValueType(0.12661242);
                RH[7] = ValueType(0.05106681);
                RH[8] = ValueType(0.01365433);
                break;
              default:
                throw Exception ("No aPSF RH data for lmax " + str(lmax));
            }
          }


          Vector<ValueType>& operator() (Vector<ValueType>& sh, const Point<ValueType>& dir) const
          {
            sh.allocate(RH.size());
            Vector<ValueType> delta_coefs;
            delta (delta_coefs, dir, lmax);
            sconv (sh, RH, delta_coefs);
            return sh;
          }


        private:
          const size_t lmax;
          Vector<ValueType> RH;

      };


      //! convenience function to check if an input image can contain SH coefficients
      template <class Infotype>
        void check (const Infotype& info) {
          if (info.datatype().is_complex()) 
            throw Exception ("image \"" + info.name() + "\" does not contain SH coefficients - contains complex data");
          if (!info.datatype().is_floating_point()) 
            throw Exception ("image \"" + info.name() + "\" does not contain SH coefficients - data type is not floating-point");
          if (info.ndim() < 4)
            throw Exception ("image \"" + info.name() + "\" does not contain SH coefficients - not 4D");
          size_t l = LforN (info.dim(3));
          if (l%2 || NforL (l) != size_t (info.dim(3)))
            throw Exception ("image \"" + info.name() + "\" does not contain SH coefficients - unexpected number of coefficients");
        }
  /** @} */

    }
  }
}

#endif
