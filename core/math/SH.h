/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#ifndef __math_SH_h__
#define __math_SH_h__

#include "math/legendre.h"
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
        return l * (l+1) /2 + m;
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
      template <class MatrixType>
        Eigen::Matrix<typename MatrixType::Scalar,Eigen::Dynamic, Eigen::Dynamic> init_transform (const MatrixType& dirs, const int lmax)
        {
          using namespace Eigen;
          using value_type = typename MatrixType::Scalar;
          if (dirs.cols() != 2)
            throw Exception ("direction matrix should have 2 columns: [ azimuth elevation ]");
          Matrix<value_type,Dynamic,Dynamic> SHT (dirs.rows(), NforL (lmax));
          Matrix<value_type,Dynamic,1,0,64> AL (lmax+1);
          for (ssize_t i = 0; i < dirs.rows(); i++) {
            const value_type z = std::cos (dirs (i,1));
            Legendre::Plm_sph (AL, lmax, 0, z);
            for (int l = 0; l <= lmax; l+=2)
              SHT (i,index (l,0)) = AL[l];
            for (int m = 1; m <= lmax; m++) {
              Legendre::Plm_sph (AL, lmax, m, z);
              for (int l = ( (m&1) ? m+1 : m); l <= lmax; l+=2) {
                SHT(i, index(l, m)) = Math::sqrt2 * AL[l]*std::cos (m*dirs (i,0));
                SHT(i, index(l,-m)) = Math::sqrt2 * AL[l]*std::sin (m*dirs (i,0));
              }
            }
          }
          return SHT;
        }

      //! form the SH->amplitudes matrix
      /*! This computes the matrix \a SHT mapping spherical harmonic
       * coefficients up to maximum harmonic degree \a lmax onto directions \a
       * dirs (in cartesian coordinates, with columns [ x y z ] ans normalised). */
      template <class MatrixType>
        Eigen::Matrix<typename MatrixType::Scalar,Eigen::Dynamic, Eigen::Dynamic> init_transform_cart (const MatrixType& dirs, const int lmax)
        {
          using namespace Eigen;
          using value_type = typename MatrixType::Scalar;
          if (dirs.cols() != 3)
            throw Exception ("direction matrix should have 3 columns: [ x y z ]");
          Matrix<value_type,Dynamic,Dynamic> SHT (dirs.rows(), NforL (lmax));
          Matrix<value_type,Dynamic,1,0,64> AL (lmax+1);
          for (ssize_t i = 0; i < dirs.rows(); i++) {
            value_type z = dirs (i,2);
            value_type rxy = std::hypot(dirs(i,0), dirs(i,1));
            value_type cp = (rxy) ? dirs(i,0)/rxy : 1.0;
            value_type sp = (rxy) ? dirs(i,1)/rxy : 0.0;
            Legendre::Plm_sph (AL, lmax, 0, z);
            for (int l = 0; l <= lmax; l+=2)
              SHT (i,index (l,0)) = AL[l];
            value_type c0 (1.0), s0 (0.0);
            for (int m = 1; m <= lmax; m++) {
              Legendre::Plm_sph (AL, lmax, m, z);
              value_type c = c0 * cp - s0 * sp;
              value_type s = s0 * cp + c0 * sp;
              for (int l = ( (m&1) ? m+1 : m); l <= lmax; l+=2) {
                SHT(i, index(l, m)) = Math::sqrt2 * AL[l] * c;
                SHT(i, index(l,-m)) = Math::sqrt2 * AL[l] * s;
              }
              c0 = c;
              s0 = s;
            }
          }
          return SHT;
        }



      //! scale the coefficients of each SH degree by the corresponding value in \a coefs
      template <class MatrixType, class VectorType>
        inline void scale_degrees_forward (MatrixType& SH2amp_mapping, const VectorType& coefs)
        {
          ssize_t l = 0, nl = 1;
          for (ssize_t col = 0; col < SH2amp_mapping.cols(); ++col) {
            if (col >= nl) {
              l++;
              nl = NforL (2*l);
            }
            SH2amp_mapping.col(col) *= coefs[l];
          }
        }



      //! scale the coefficients of each SH degree by the corresponding value in \a coefs
      template <typename MatrixType, class VectorType>
        inline void scale_degrees_inverse (MatrixType& amp2SH_mapping, const VectorType& coefs)
        {
          ssize_t l = 0, nl = 1;
          for (ssize_t row = 0; row < amp2SH_mapping.rows(); ++row) {
            if (row >= nl) {
              l++;
              nl = NforL (2*l);
            }
            amp2SH_mapping.row(row) *= coefs[l];
          }
        }




      //! invert any non-zero coefficients in \a coefs
      template <typename VectorType>
        inline Eigen::Matrix<typename VectorType::Scalar,Eigen::Dynamic,1> invert (const VectorType& coefs)
        {
          Eigen::Matrix<typename VectorType::Scalar,Eigen::Dynamic,1> ret (coefs.size());
          for (size_t n = 0; n < coefs.size(); ++n)
            ret[n] = ( coefs[n] ? 1.0 / coefs[n] : 0.0 );
          return ret;
        }

      template <typename ValueType>
      class Transform {
        public:
          using matrix_type = Eigen::Matrix<ValueType,Eigen::Dynamic,Eigen::Dynamic>;

          template <class MatrixType>
            Transform (const MatrixType& dirs, int lmax) :
              SHT (init_transform (dirs, lmax)),
              iSHT (pinv (SHT)) { }

          template <class VectorType>
            void set_filter (const VectorType& filter) {
              scale_degrees_forward (SHT, invert (filter));
              scale_degrees_inverse (iSHT, filter);
            }
          template <class VectorType1, class VectorType2>
            void A2SH (VectorType1& sh, const VectorType2& amplitudes) const {
              sh.noalias() = iSHT * amplitudes;
            }
          template <class VectorType1, class VectorType2>
            void SH2A (VectorType1& amplitudes, const VectorType2& sh) const {
              amplitudes.noalias() = SHT * sh;
            }

          size_t n_SH () const {
            return SHT.cols();
          }
          size_t n_amp () const  {
            return SHT.rows();
          }

          const matrix_type& mat_A2SH () const {
            return iSHT;
          }
          const matrix_type& mat_SH2A () const {
            return SHT;
          }

        protected:
          matrix_type SHT, iSHT;
      };



      template <class VectorType>
        inline typename VectorType::Scalar value (const VectorType& coefs,
            typename VectorType::Scalar cos_elevation,
            typename VectorType::Scalar cos_azimuth,
            typename VectorType::Scalar sin_azimuth,
            int lmax)
        {
          using value_type = typename VectorType::Scalar;
          value_type amplitude = 0.0;
          Eigen::Matrix<value_type,Eigen::Dynamic,1,0,64> AL (lmax+1);
          Legendre::Plm_sph (AL, lmax, 0, cos_elevation);
          for (int l = 0; l <= lmax; l+=2)
            amplitude += AL[l] * coefs[index (l,0)];
          value_type c0 (1.0), s0 (0.0);
          for (int m = 1; m <= lmax; m++) {
            Legendre::Plm_sph (AL, lmax, m, cos_elevation);
            value_type c = c0 * cos_azimuth - s0 * sin_azimuth;  // std::cos(m*azimuth)
            value_type s = s0 * cos_azimuth + c0 * sin_azimuth;  // std::sin(m*azimuth)
            for (int l = ( (m&1) ? m+1 : m); l <= lmax; l+=2)
              amplitude += AL[l] * Math::sqrt2 * (c * coefs[index (l,m)] + s * coefs[index (l,-m)]);
            c0 = c;
            s0 = s;
          }
          return amplitude;
        }

      template <class VectorType>
        inline typename VectorType::Scalar value (const VectorType& coefs,
            typename VectorType::Scalar cos_elevation,
            typename VectorType::Scalar azimuth,
            int lmax)
        {
          return value (coefs, cos_elevation, std::cos(azimuth), std::sin(azimuth), lmax);
        }

      template <class VectorType1, class VectorType2>
        inline typename VectorType1::Scalar value (const VectorType1& coefs, const VectorType2& unit_dir, int lmax)
        {
          using value_type = typename VectorType1::Scalar;
          value_type rxy = std::sqrt ( pow2(unit_dir[1]) + pow2(unit_dir[0]) );
          value_type cp = (rxy) ? unit_dir[0]/rxy : 1.0;
          value_type sp = (rxy) ? unit_dir[1]/rxy : 0.0;
          return value (coefs, unit_dir[2], cp, sp, lmax);
        }


      template <class VectorType1, class VectorType2>
        inline VectorType1& delta (VectorType1& delta_vec, const VectorType2& unit_dir, int lmax)
        {
          using value_type = typename VectorType1::Scalar;
          delta_vec.resize (NforL (lmax));
          value_type rxy = std::sqrt ( pow2(unit_dir[1]) + pow2(unit_dir[0]) );
          value_type cp = (rxy) ? unit_dir[0]/rxy : 1.0;
          value_type sp = (rxy) ? unit_dir[1]/rxy : 0.0;
          Eigen::Matrix<value_type,Eigen::Dynamic,1,0,64> AL (lmax+1);
          Legendre::Plm_sph (AL, lmax, 0, unit_dir[2]);
          for (int l = 0; l <= lmax; l+=2)
            delta_vec[index (l,0)] = AL[l];
          value_type c0 (1.0), s0 (0.0);
          for (int m = 1; m <= lmax; m++) {
            Legendre::Plm_sph (AL, lmax, m, unit_dir[2]);
            value_type c = c0 * cp - s0 * sp;
            value_type s = s0 * cp + c0 * sp;
            for (int l = ( (m&1) ? m+1 : m); l <= lmax; l+=2) {
              delta_vec[index (l,m)]  = AL[l] * Math::sqrt2 * c;
              delta_vec[index (l,-m)] = AL[l] * Math::sqrt2 * s;
            }
            c0 = c;
            s0 = s;
          }
          return delta_vec;
        }



      template <class VectorType1, class VectorType2>
        inline VectorType1& SH2RH (VectorType1& RH, const VectorType2& sh)
        {
          using value_type = typename VectorType2::Scalar;
          RH.resize (sh.size());
          int lmax = 2*sh.size() +1;
          Eigen::Matrix<value_type,Eigen::Dynamic,1,0,64> AL (lmax+1);
          Legendre::Plm_sph (AL, lmax, 0, 1.0);
          for (ssize_t l = 0; l < sh.size(); l++)
            RH[l] = sh[l]/ AL[2*l];
          return RH;
        }

      template <class VectorType>
        inline Eigen::Matrix<typename VectorType::Scalar,Eigen::Dynamic,1> SH2RH (const VectorType& sh)
        {
          Eigen::Matrix<typename VectorType::Scalar,Eigen::Dynamic,1> RH (sh.size());
          SH2RH (RH, sh);
          return RH;
        }


      //! perform spherical convolution, in place
      /*! perform spherical convolution of SH coefficients \a sh with response
       * function \a RH, storing the results in place in vector \a sh. */
      template <class VectorType1, class VectorType2>
        inline VectorType1& sconv (VectorType1& sh, const VectorType2& RH)
        {
          assert (sh.size() >= ssize_t (NforL (2* (RH.size()-1))));
          for (ssize_t i = 0; i < RH.size(); ++i) {
            int l = 2*i;
            for (int m = -l; m <= l; ++m)
              sh[index (l,m)] *= RH[i];
          }
          return sh;
        }


      //! perform spherical convolution
      /*! perform spherical convolution of SH coefficients \a sh with response
       * function \a RH, storing the results in vector \a C. */
      template <class VectorType1, class VectorType2, class VectorType3>
        inline VectorType1& sconv (VectorType1& C, const VectorType2& RH, const VectorType3& sh)
        {
          assert (sh.size() >= ssize_t (NforL (2* (RH.size()-1))));
          C.resize (NforL (2* (RH.size()-1)));
          for (ssize_t i = 0; i < RH.size(); ++i) {
            int l = 2*i;
            for (int m = -l; m <= l; ++m)
              C[index (l,m)] = RH[i] * sh[index (l,m)];
          }
          return C;
        }


      //! perform spherical convolution, in place
      /*! perform spherical convolution of SH coefficients, stored in rows
       * in matrix \a sh with response function \a RH, storing the results
       * in place in matrix \a sh. */
      template <class MatrixType1, class VectorType2>
        inline MatrixType1& sconv_mat (MatrixType1& sh, const VectorType2& RH)
        {
          assert (sh.cols() >= ssize_t (NforL (2* (RH.size()-1))));
          for (ssize_t i = 0; i < RH.size(); ++i) {
            int l = 2*i;
            for (int m = -l; m <= l; ++m)
              sh.col(index (l,m)) *= RH[i];
          }
          return sh;
        }


      namespace {
        template <typename> struct __dummy {  using type = int; };
      }






      //! used to speed up SH calculation
      template <typename ValueType> class PrecomputedFraction
      {
        public:
          PrecomputedFraction () : f1 (0.0), f2 (0.0) { }
          ValueType f1, f2;
          typename vector<ValueType>::const_iterator p1, p2;
      };


      //! Precomputed Associated Legrendre Polynomials - used to speed up SH calculation
      template <typename ValueType> class PrecomputedAL
      {
        public:
          using value_type = ValueType;

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
            Eigen::Matrix<value_type,Eigen::Dynamic,1,0,64> buf (lmax+1);

            for (int n = 0; n < ndir; n++) {
              typename vector<value_type>::iterator p = AL.begin() + n*nAL;
              value_type cos_el = std::cos (n*inc);
              for (int m = 0; m <= lmax; m++) {
                Legendre::Plm_sph (buf, lmax, m, cos_el);
                for (int l = ( (m&1) ?m+1:m); l <= lmax; l+=2)
                  p[index_mpos (l,m)] = buf[l];
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

          template <class VectorType, class UnitVectorType>
            ValueType value (const VectorType& val, const UnitVectorType& unit_dir) const {
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
                  v += get (f,l,m) * Math::sqrt2 * (c * val[index (l,m)] + s * val[index (l,-m)]);
                c0 = c;
                s0 = s;
              }
              return v;
            }

        protected:
          int lmax, ndir, nAL;
          ValueType inc;
          vector<ValueType> AL;
      };






      //! estimate direction & amplitude of SH peak
      /*! find a peak of an SH series using Gauss-Newton optimisation, modified
       * to operate directly in spherical coordinates. The initial search
       * direction is \a unit_init_dir. If \a precomputer is not nullptr, it
       * will be used to speed up the calculations, at the cost of a minor
       * reduction in accuracy. */
      template <class VectorType, class UnitVectorType, class ValueType = float>
        inline typename VectorType::Scalar get_peak (
            const VectorType& sh,
            int lmax,
            UnitVectorType& unit_init_dir,
            PrecomputedAL<typename VectorType::Scalar>* precomputer = nullptr)
        {
          using value_type = typename VectorType::Scalar;
          assert (std::isfinite (unit_init_dir[0]));
          for (int i = 0; i < 50; i++) {
            value_type az = std::atan2 (unit_init_dir[1], unit_init_dir[0]);
            value_type el = std::acos (unit_init_dir[2]);
            value_type amplitude, dSH_del, dSH_daz, d2SH_del2, d2SH_deldaz, d2SH_daz2;
            derivatives (sh, lmax, el, az, amplitude, dSH_del, dSH_daz, d2SH_del2, d2SH_deldaz, d2SH_daz2, precomputer);

            value_type del = sqrt (dSH_del*dSH_del + dSH_daz*dSH_daz);
            value_type daz = 0.0;
            if (del != 0.0) {
              daz = dSH_daz/del;
              del = dSH_del/del;
            }


            value_type dSH_dt = daz*dSH_daz + del*dSH_del;
            value_type d2SH_dt2 = daz*daz*d2SH_daz2 + 2.0*daz*del*d2SH_deldaz + del*del*d2SH_del2;
            value_type dt = d2SH_dt2 ? (-dSH_dt / d2SH_dt2) : 0.0;

            if (dt < 0.0) dt = -dt;
            if (dt > MAX_DIR_CHANGE) dt = MAX_DIR_CHANGE;

            del *= dt;
            daz *= dt;

            unit_init_dir[0] += del*std::cos (az) *std::cos (el) - daz*std::sin (az);
            unit_init_dir[1] += del*std::sin (az) *std::cos (el) + daz*std::cos (az);
            unit_init_dir[2] -= del*std::sin (el);
            unit_init_dir.normalize();

            if (dt < ANGLE_TOLERANCE)
              return amplitude;
          }

          unit_init_dir = { NaN, NaN, NaN };
          DEBUG ("failed to find SH peak!");
          return NaN;
        }






      //! computes first and second order derivatives of SH series
      /*! This is used primarily in the get_peak() function. */
      template <class VectorType>
        inline void derivatives (
            const VectorType& sh,
            const int lmax,
            const typename VectorType::Scalar elevation,
            const typename VectorType::Scalar azimuth,
            typename VectorType::Scalar& amplitude,
            typename VectorType::Scalar& dSH_del,
            typename VectorType::Scalar& dSH_daz,
            typename VectorType::Scalar& d2SH_del2,
            typename VectorType::Scalar& d2SH_deldaz,
            typename VectorType::Scalar& d2SH_daz2,
            PrecomputedAL<typename VectorType::Scalar>* precomputer)
        {
          using value_type = typename VectorType::Scalar;
          value_type sel = std::sin (elevation);
          value_type cel = std::cos (elevation);
          bool atpole = sel < 1e-4;

          dSH_del = dSH_daz = d2SH_del2 = d2SH_deldaz = d2SH_daz2 = 0.0;
          VLA_MAX (AL, value_type, NforL_mpos (lmax), 64);

          if (precomputer) {
            PrecomputedFraction<value_type> f;
            precomputer->set (f, elevation);
            precomputer->get (AL, f);
          }
          else {
            Eigen::Matrix<value_type,Eigen::Dynamic,1,0,64> buf (lmax+1);
            for (int m = 0; m <= lmax; m++) {
              Legendre::Plm_sph (buf, lmax, m, cel);
              for (int l = ( (m&1) ?m+1:m); l <= lmax; l+=2)
                AL[index_mpos (l,m)] = buf[l];
            }
          }

          amplitude = sh[index (0, 0)] * AL[index_mpos (0, 0)];
          for (int l = 2; l <= (int) lmax; l+=2) {
            const value_type& v (sh[index (l,0)]);
            amplitude += v * AL[index_mpos (l,0)];
            dSH_del += v * sqrt (value_type (l* (l+1))) * AL[index_mpos (l,1)];
            d2SH_del2 += v * (sqrt (value_type (l* (l+1) * (l-1) * (l+2))) * AL[index_mpos (l,2)] - l* (l+1) * AL[index_mpos (l,0)]) /2.0;
          }

          for (int m = 1; m <= lmax; m++) {
            value_type caz = Math::sqrt2 * std::cos (m*azimuth);
            value_type saz = Math::sqrt2 * std::sin (m*azimuth);
            for (int l = ( (m&1) ? m+1 : m); l <= lmax; l+=2) {
              const value_type& vp (sh[index (l,m)]);
              const value_type& vm (sh[index (l,-m)]);
              amplitude += (vp*caz + vm*saz) * AL[index_mpos (l,m)];

              value_type tmp = sqrt (value_type ( (l+m) * (l-m+1))) * AL[index_mpos (l,m-1)];
              if (l > m) tmp -= sqrt (value_type ( (l-m) * (l+m+1))) * AL[index_mpos (l,m+1)];
              tmp /= -2.0;
              dSH_del += (vp*caz + vm*saz) * tmp;

              value_type tmp2 = - ( (l+m) * (l-m+1) + (l-m) * (l+m+1)) * AL[index_mpos (l,m)];
              if (m == 1) tmp2 -= sqrt (value_type ( (l+m) * (l-m+1) * (l+m-1) * (l-m+2))) * AL[index_mpos (l,1)];
              else tmp2 += sqrt (value_type ( (l+m) * (l-m+1) * (l+m-1) * (l-m+2))) * AL[index_mpos (l,m-2)];
              if (l > m+1) tmp2 += sqrt (value_type ( (l-m) * (l+m+1) * (l-m-1) * (l+m+2))) * AL[index_mpos (l,m+2)];
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
          aPSF (const size_t lmax) :
            lmax (lmax),
            RH (lmax/2 + 1)
        {
          switch (lmax) {
            case 2:
              RH[0] = ValueType(1.00000000);
              RH[1] = ValueType(0.41939279);
              break;
            case 4:
              RH[0] = ValueType(1.00000000);
              RH[1] = ValueType(0.63608543);
              RH[2] = ValueType(0.18487087);
              break;
            case 6:
              RH[0] = ValueType(1.00000000);
              RH[1] = ValueType(0.75490341);
              RH[2] = ValueType(0.37126442);
              RH[3] = ValueType(0.09614699);
              break;
            case 8:
              RH[0] = ValueType(1.00000000);
              RH[1] = ValueType(0.82384816);
              RH[2] = ValueType(0.51261696);
              RH[3] = ValueType(0.22440563);
              RH[4] = ValueType(0.05593079);
              break;
            case 10:
              RH[0] = ValueType(1.00000000);
              RH[1] = ValueType(0.86725945);
              RH[2] = ValueType(0.61519436);
              RH[3] = ValueType(0.34570667);
              RH[4] = ValueType(0.14300355);
              RH[5] = ValueType(0.03548062);
              break;
            case 12:
              RH[0] = ValueType(1.00000000);
              RH[1] = ValueType(0.89737759);
              RH[2] = ValueType(0.69278503);
              RH[3] = ValueType(0.45249879);
              RH[4] = ValueType(0.24169922);
              RH[5] = ValueType(0.09826171);
              RH[6] = ValueType(0.02502481);
              break;
            case 14:
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


          template <class VectorType, class UnitVectorType>
          VectorType& operator() (VectorType& sh, const UnitVectorType& dir) const
          {
            sh.resize (RH.size());
            delta (sh, dir, lmax);
            return sconv (sh, RH);
          }

          inline const Eigen::Matrix<ValueType,Eigen::Dynamic,1>& RH_coefs() const { return RH; }

        private:
          const size_t lmax;
          Eigen::Matrix<ValueType,Eigen::Dynamic,1> RH;

      };


      //! convenience function to check if an input image can contain SH coefficients
      template <class ImageType>
        void check (const ImageType& H) {
          if (H.ndim() < 4)
            throw Exception ("image \"" + H.name() + "\" does not contain SH coefficients - not 4D");
          size_t l = LforN (H.size(3));
          if (l%2 || NforL (l) != size_t (H.size(3)))
            throw Exception ("image \"" + H.name() + "\" does not contain SH coefficients - unexpected number of coefficients");
        }
      /** @} */

    }
  }
}

#endif
