/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by JRobert E. Smith, 26/10/2015.

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

#ifndef __math_ZSH_h__
#define __math_ZSH_h__

#include <Eigen/Dense>

#include "math/legendre.h"
#include "math/least_squares.h"
#include "math/SH.h"

namespace MR
{
  namespace Math
  {
    namespace ZSH
    {

      /** \defgroup zonal_spherical_harmonics Zonal Spherical Harmonics
       * \brief Classes & functions to manage zonal spherical harmonics
       * (spherical harmonic functions containing only m=0 terms). */

      /** \addtogroup zonal_spherical_harmonics
       * @{ */


      //! the number of (even-degree) coefficients for the given value of \a lmax
      inline size_t NforL (int lmax)
      {
        return (1 + lmax/2);
      }

      //! compute the index for coefficient l
      inline size_t index (int l)
      {
        return (l/2);
      }

      //! returns the largest \e lmax given \a N parameters 
      inline size_t LforN (int N)
      {
        return (2 * (N-1));
      }



      //! form the ZSH->amplitudes matrix for a set of elevation angles
      /*! This computes the matrix \a ZSHT mapping zonal spherical harmonic
       * coefficients up to maximum harmonic degree \a lmax onto amplitudes on
       * a set of elevations stored in a vector */
      template <typename value_type, class VectorType>
      Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic> init_amp_transform (const VectorType& els, const size_t lmax)
      {
        Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic> ZSHT;
        ZSHT.resize (els.size(), ZSH::NforL (lmax));
        Eigen::Matrix<value_type, Eigen::Dynamic, 1, 0, 64> AL (lmax+1);
        for (size_t i = 0; i != size_t(els.size()); i++) {
          Legendre::Plm_sph (AL, lmax, 0, std::cos (els[i]));
          for (size_t l = 0; l <= lmax; l += 2)
            ZSHT (i,index(l)) = AL[l];
        }
        return ZSHT;
      }



      //! form the ZSH->derivatives matrix for a set of elevation angles
      /*! This computes the matrix \a ZSHT mapping zonal spherical harmonic
       * coefficients up to maximum harmonic degree \a lmax onto derivatives
       * with respect to elevation angle, for a set of elevations stored in
       * a vector */
      template <typename value_type, class VectorType>
      Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic> init_deriv_transform (const VectorType& els, const size_t lmax)
      {
        Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic> dZSHdelT;
        dZSHdelT.resize (els.size(), ZSH::NforL (lmax));
        Eigen::Matrix<value_type, Eigen::Dynamic, 1, 0, 64> AL (lmax+1);
        for (size_t i = 0; i != size_t(els.size()); i++) {
          Legendre::Plm_sph (AL, lmax, 1, std::cos (els[i]));
          dZSHdelT (i,index(0)) = 0.0;
          for (size_t l = 2; l <= lmax; l += 2)
            dZSHdelT (i,index(l)) = AL[l] * sqrt (value_type (l*(l+1)));
        }
        return dZSHdelT;
      }




      template <typename ValueType>
      class Transform { MEMALIGN (Transform<ValueType>)
        public:
          typedef Eigen::Matrix<ValueType,Eigen::Dynamic,Eigen::Dynamic> matrix_type;

          template <class MatrixType>
          Transform (const MatrixType& dirs, const size_t lmax) :
              ZSHT (init_amp_transform (dirs.col(1), lmax)), // Elevation angles are second column of aximuth/elevation matrix
              iZSHT (pinv (ZSHT)) { }

          template <class VectorType1, class VectorType2>
          void A2ZSH (VectorType1& zsh, const VectorType2& amplitudes) const {
            zsh.noalias() = iZSHT * amplitudes;
          }
          template <class VectorType1, class VectorType2>
          void ZSH2A (VectorType1& amplitudes, const VectorType2& zsh) const {
            amplitudes.noalias() = ZSHT * zsh;
          }

          size_t n_ZSH () const {
            return ZSHT.cols();
          }
          size_t n_amp () const  {
            return ZSHT.rows();
          }

          const matrix_type& mat_A2ZSH () const {
            return iZSHT;
          }
          const matrix_type& mat_ZSH2A () const {
            return ZSHT;
          }

        protected:
          matrix_type ZSHT, iZSHT;
      };



      template <class VectorType>
      inline typename VectorType::Scalar value (
          const VectorType& coefs,
          typename VectorType::Scalar elevation,
          const size_t lmax)
      {
        typedef typename VectorType::Scalar value_type;
        Eigen::Matrix<value_type, Eigen::Dynamic, 1, 0, 64> AL (lmax+1);
        Legendre::Plm_sph (AL, lmax, 0, std::cos(elevation));
        value_type amplitude = 0.0;
        for (size_t l = 0; l <= lmax; l += 2)
          amplitude += AL[l] * coefs[index(l)];
        return amplitude;
      }



      template <class VectorType>
      inline typename VectorType::Scalar derivative (
          const VectorType& coefs,
          const typename VectorType::Scalar elevation,
          const size_t lmax)
      {
        typedef typename VectorType::Scalar value_type;
        Eigen::Matrix<value_type, Eigen::Dynamic, 1, 0, 64> AL (lmax+1);
        Legendre::Plm_sph (AL, lmax, 1, std::cos (elevation));
        value_type dZSH_del = 0.0;
        for (size_t l = 2; l <= lmax; l += 2)
          dZSH_del += AL[l] * coefs[index(l)] * sqrt (value_type (l*(l+1)));
        return dZSH_del;
      }



      template <class VectorType1, class VectorType2>
      inline VectorType1& ZSH2SH (VectorType1& sh, const VectorType2& zsh)
      {
        const size_t lmax = LforN (zsh.size());
        sh.resize (Math::SH::NforL (lmax));
        for (size_t i = 0; i != size_t(sh.size()); ++i)
          sh[i] = 0.0;
        for (size_t l = 0; l <= lmax; l+=2)
          sh[Math::SH::index(l,0)] = zsh[index(l)];
        return sh;
      }

      template <class VectorType>
      inline Eigen::Matrix<typename VectorType::Scalar, Eigen::Dynamic, 1> ZSH2SH (const VectorType& zsh)
      {
        Eigen::Matrix<typename VectorType::Scalar, Eigen::Dynamic, 1> sh;
        ZSH2SH (sh, zsh);
        return sh;
      }




      template <class VectorType1, class VectorType2>
      inline VectorType1& SH2ZSH (VectorType1& zsh, const VectorType2& sh)
      {
        const size_t lmax = Math::SH::LforN (sh.size());
        zsh.resize (NforL (lmax));
        for (size_t l = 0; l <= lmax; l+=2)
          zsh[index(l)] = sh[Math::SH::index(l,0)];
        return zsh;
      }

      template <class VectorType>
      inline Eigen::Matrix<typename VectorType::Scalar, Eigen::Dynamic, 1> SH2ZSH (const VectorType& sh)
      {
        Eigen::Matrix<typename VectorType::Scalar, Eigen::Dynamic, 1> zsh;
        SH2ZSH (zsh, sh);
        return zsh;
      }



      template <class VectorType1, class VectorType2>
      inline VectorType1& ZSH2RH (VectorType1& rh, const VectorType2& zsh)
      {
        typedef typename VectorType2::Scalar value_type;
        rh.resize (zsh.size());
        const size_t lmax = LforN (zsh.size());
        Eigen::Matrix<value_type,Eigen::Dynamic,1,0,64> AL (lmax+1);
        Legendre::Plm_sph (AL, lmax, 0, 1.0);
        for (size_t l = 0; l <= lmax; l += 2)
          rh[index(l)] = zsh[index(l)] / AL[l];
        return rh;
      }

      template <class VectorType>
      inline Eigen::Matrix<typename VectorType::Scalar,Eigen::Dynamic,1> ZSH2RH (const VectorType& zsh)
      {
        Eigen::Matrix<typename VectorType::Scalar,Eigen::Dynamic,1> rh (zsh.size());
        ZSH2RH (rh, zsh);
        return rh;
      }



      //! perform zonal spherical convolution, in place
      /*! perform zonal spherical convolution of ZSH coefficients \a zsh with response
       * function \a RH, storing the results in place in vector \a zsh. */
      template <class VectorType1, class VectorType2>
      inline VectorType1& zsconv (VectorType1& zsh, const VectorType2& RH)
      {
        assert (zsh.size() >= RH.size());
        for (size_t i = 0; i != size_t(RH.size()); ++i)
          zsh[i] *= RH[i];
        return zsh;
      }


      //! perform zonal spherical convolution
      /*! perform zonal spherical convolution of SH coefficients \a sh with response
       * function \a RH, storing the results in vector \a C. */
      template <class VectorType1, class VectorType2, class VectorType3>
      inline VectorType1& zsconv (VectorType1& C, const VectorType2& RH, const VectorType3& zsh)
      {
        assert (zsh.size() >= RH.size());
        C.resize (RH.size());
        for (size_t i = 0; i != size_t(RH.size()); ++i)
          C[i] = zsh[i] * RH[i];
        return C;
      }




      //! compute ZSH coefficients corresponding to specified tensor
      template <class VectorType>
      inline VectorType& FA2ZSH (VectorType& zsh, default_type FA, default_type ADC, default_type bvalue, const size_t lmax, const size_t precision = 100)
      {
        default_type a = FA/sqrt (3.0 - 2.0*FA*FA);
        default_type ev1 = ADC* (1.0+2.0*a), ev2 = ADC* (1.0-a);

        Eigen::VectorXd sigs (precision);
        Eigen::MatrixXd ZSHT (precision, lmax/2+1);
        Eigen::Matrix<default_type,Eigen::Dynamic,1,0,64> AL;

        for (size_t i = 0; i < precision; i++) {
          default_type el = i*Math::pi / (2.0*(precision-1));
          sigs[i] = exp (-bvalue * (ev1*std::cos (el)*std::cos (el) + ev2*std::sin (el)*std::sin (el)));
          Legendre::Plm_sph (AL, lmax, 0, std::cos (el));
          for (size_t l = 0; l <= lmax; l+=2)
            ZSHT (i,index(l)) = AL[l];
        }

        return (zsh = pinv(ZSHT) * sigs);
      }



    }
  }
}

#endif
