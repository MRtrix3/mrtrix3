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
#include "math/quaternion.h"
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
      
      extern const char* encoding_description;

      inline size_t NforL (int lmax)
      {
        return (lmax+1) * (lmax+2) /2;
      }
      inline size_t index (int l, int m)
      {
        return l* (l+1) /2 + m;
      }

      inline size_t NforL_mpos (int lmax)
      {
        return (lmax/2+1) * (lmax/2+1);
      }
      inline size_t index_mpos (int l, int m)
      {
        return l*l/4 + m;
      }

      inline size_t LforN (int N)
      {
        return N ? 2 * floor<size_t> ( (sqrt (float (1+8*N))-3.0) /4.0) : 0;
      }

      template <typename ValueType> 
        Math::Matrix<ValueType>& init_transform (Math::Matrix<ValueType>& SHT, const Math::Matrix<ValueType>& dirs, int lmax)
        {
          if (dirs.columns() != 2) throw Exception ("direction matrix should have 2 columns: [ azimuth elevation ]");
          SHT.allocate (dirs.rows(), NforL (lmax));
          ValueType AL [lmax+1];
          for (size_t i = 0; i < dirs.rows(); i++) {
            ValueType x = cos (dirs (i,1));
            Legendre::Plm_sph (AL, lmax, 0, x);
            for (int l = 0; l <= lmax; l+=2) SHT (i,index (l,0)) = AL[l];
            for (int m = 1; m <= lmax; m++) {
              Legendre::Plm_sph (AL, lmax, m, x);
              for (int l = ( (m&1) ? m+1 : m); l <= lmax; l+=2) {
                SHT (i,index (l, m)) = AL[l]*cos (m*dirs (i,0));
                SHT (i,index (l,-m)) = AL[l]*sin (m*dirs (i,0));
              }
            }
          }
          return SHT;
        }


      template <typename ValueType>
        class Transform {
          public:
            Transform (const Math::Matrix<ValueType>& dirs, int lmax) {
              init_transform (SHT, dirs, lmax);
              iSHT.allocate (SHT.columns(), SHT.rows());
              Math::pinv (iSHT, SHT);
            }

            void set_filter (const Math::Vector<ValueType>& filter) {
              int l = 0;
              size_t nl = 1;
              for (size_t n = 0; n < iSHT.rows(); n++) {
                if (n >= nl) {
                  l++;
                  nl = NforL (2*l);
                }
                for (size_t i = 0; i < iSHT.columns(); i++) {
                  iSHT (n,i) *= filter[l];
                  SHT (i,n) = filter[l] == 0.0 ? 0.0 : SHT (i,n) /filter[l];
                }
              }
            }
            void A2SH (Math::Vector<ValueType>& SH, const Math::Vector<ValueType>& amplitudes)  {
              Math::mult (SH, iSHT, amplitudes);
            }
            void SH2A (Math::Vector<ValueType>& amplitudes, const Math::Vector<ValueType>& SH)  {
              Math::mult (amplitudes, SHT, SH);
            }

            size_t n_SH () const {
              return SHT.columns();
            }
            size_t n_amp () const  {
              return SHT.rows();
            }

            const Math::Matrix<ValueType>& mat_A2SH () const {
              return iSHT;
            }
            const Math::Matrix<ValueType>& mat_SH2A () const {
              return SHT;
            }

          protected:
            Math::Matrix<ValueType> SHT, iSHT;
        };



      template <typename ValueType, typename CoefType> 
        inline ValueType value (const CoefType& coefs, ValueType cos_elevation, ValueType azimuth, int lmax) 
        {
          ValueType amplitude = 0.0;
          ValueType AL [lmax+1];
          Legendre::Plm_sph (AL, lmax, 0, ValueType (cos_elevation));
          for (int l = 0; l <= lmax; l+=2) amplitude += AL[l] * coefs[index (l,0)];
          for (int m = 1; m <= lmax; m++) {
            Legendre::Plm_sph (AL, lmax, m, ValueType (cos_elevation));
            ValueType c = Math::cos (m*azimuth);
            ValueType s = Math::sin (m*azimuth);
            for (int l = ( (m&1) ? m+1 : m); l <= lmax; l+=2)
              amplitude += AL[l] * (c * coefs[index (l,m)] + s * coefs[index (l,-m)]);
          }
          return amplitude;
        }


      template <typename ValueType, typename CoefType> 
        inline ValueType value (const CoefType& coefs, const Point<ValueType>& unit_dir, int lmax)
        {
          return value (coefs, unit_dir[2], atan2 (unit_dir[1], unit_dir[0]), lmax);
        }

      template <typename ValueType, typename CoefType> 
        inline ValueType value (const CoefType* coefs, const Point<ValueType>& unit_dir, int lmax)
        {
          return value (coefs, unit_dir[2], atan2 (unit_dir[1], unit_dir[0]), lmax);
        }


      template <typename ValueType> 
        inline Math::Vector<ValueType>& delta (Math::Vector<ValueType>& delta_vec, const Point<ValueType>& unit_dir, int lmax)
        {
          delta_vec.allocate (NforL (lmax));
          ValueType az = Math::atan2 (unit_dir[1], unit_dir[0]);
          ValueType AL [lmax+1];
          Legendre::Plm_sph (AL, lmax, 0, ValueType (unit_dir[2]));
          for (int l = 0; l <= lmax; l+=2) 
            delta_vec[index (l,0)] = AL[l];
          for (int m = 1; m <= lmax; m++) {
            Legendre::Plm_sph (AL, lmax, m, ValueType (unit_dir[2]));
            ValueType c = Math::cos (m*az);
            ValueType s = Math::sin (m*az);
            for (int l = ( (m&1) ? m+1 : m); l <= lmax; l+=2) {
              delta_vec[index (l,m)]  = 2.0 * AL[l] * c;
              delta_vec[index (l,-m)] = 2.0 * AL[l] * s;
            }
          }
          return delta_vec;
        }



      template <typename ValueType>
        inline Math::Vector<ValueType>& SH2RH (Math::Vector<ValueType>& RH, const Math::Vector<ValueType>& SH)
        {
          RH.allocate (SH.size());
          int lmax = 2*SH.size() +1;
          ValueType AL [lmax+1];
          Legendre::Plm_sph (AL, lmax, 0, ValueType (1.0));
          for (size_t l = 0; l < SH.size(); l++) 
            RH[l] = SH[l]/ AL[2*l];
          return RH;
        }



      template <typename ValueType> 
        inline Math::Vector<ValueType>& sconv (Math::Vector<ValueType>& C, const Math::Vector<ValueType>& RH, const Math::Vector<ValueType>& SH)
        {
          assert (SH.size() >= NforL (2* (RH.size()-1)));
          C.allocate (NforL (2* (RH.size()-1)));
          for (int i = 0; i < int (RH.size()); ++i) {
            int l = 2*i;
            for (int m = -l; m <= l; ++m)
              C[index (l,m)] = RH[i] * SH[index (l,m)];
          }
          return C;
        }


      template <typename ValueType>
        Point<ValueType> S2C (ValueType az, ValueType el)
        {
          return Point<ValueType> (
              sin (el) * cos (az),
              sin (el) * sin (az),
              cos (el));
        }


      template <typename ValueType>
        class Rotate
        {
          public:
            Rotate (Point<ValueType>& axis, ValueType angle, int l_max, const Math::Matrix<ValueType>& directions) :
              lmax (l_max) {
                Quaternion<ValueType> Q (angle, axis);
                ValueType rotation_data [9];
                Q.to_matrix (rotation_data);
                const Matrix<ValueType> R (rotation_data, 3, 3);
                const size_t nSH = NforL (lmax);

                Matrix<ValueType> D (nSH, directions.rows());
                Matrix<ValueType> D_rot (nSH, directions.rows());
                for (size_t i = 0; i < directions.rows(); ++i) {
                  Vector<ValueType> V (D.column (i));
                  Point<ValueType> dir = S2C (directions (i,0), directions (i,1));
                  delta (V, dir, lmax);

                  Point<ValueType> dir_rot;
                  Vector<ValueType> V_dir (dir, 3);
                  Vector<ValueType> V_dir_rot (dir_rot, 3);
                  mult (V_dir_rot, R, V_dir);
                  Vector<ValueType> V_rot (D_rot.column (i));
                  delta (V_rot, dir_rot, lmax);
                }

                size_t n = 1;
                for (int l = 2; l <= lmax; l += 2) {
                  const size_t nSH_l = 2*l+1;
                  M.push_back (new Matrix<ValueType> (nSH_l, nSH_l));
                  Matrix<ValueType>& RH (*M.back());

                  const Matrix<ValueType> d = D.sub (n, n+nSH_l, 0, D.columns());
                  const Matrix<ValueType> d_rot = D_rot.sub (n, n+nSH_l, 0, D.columns());

                  Matrix<ValueType> d_rot_x_d_T;
                  mult (d_rot_x_d_T, ValueType (1.0), CblasNoTrans, d_rot, CblasTrans, d);

                  Matrix<ValueType> d_x_d_T;
                  mult (d_x_d_T, ValueType (1.0), CblasNoTrans, d, CblasTrans, d);

                  Matrix<ValueType> d_x_d_T_inv;
                  LU::inv (d_x_d_T_inv, d_x_d_T);
                  mult (RH, d_rot_x_d_T, d_x_d_T_inv);

                  n += nSH_l;
                }
              }

            Math::Vector<ValueType>& operator() (Math::Vector<ValueType>& SH_rot, const Math::Vector<ValueType>& SH) const {
              SH_rot.allocate (SH);
              SH_rot[0] = SH[0];
              size_t n = 1;
              for (size_t l = 0; l < M.size(); ++l) {
                const size_t nSH_l = M[l]->rows();
                Vector<ValueType> R = SH_rot.sub (n, n+nSH_l);
                const Vector<ValueType> S = SH.sub (n, n+nSH_l);
                mult (R, *M[l], S);
                n += nSH_l;
              }

              return SH_rot;
            }

          protected:
            VecPtr<Matrix<ValueType> > M;
            int lmax;
        };




      template <typename ValueType> 
        inline Math::Vector<ValueType>& FA2SH (
            Math::Vector<ValueType>& SH, ValueType FA, ValueType ADC, ValueType bvalue, int lmax, int precision = 100)
      {
        ValueType a = FA/sqrt (3.0 - 2.0*FA*FA);
        ValueType ev1 = ADC* (1.0+2.0*a), ev2 = ADC* (1.0-a);

        Math::Vector<ValueType> sigs (precision);
        Math::Matrix<ValueType> SHT (precision, lmax/2+1);
        ValueType AL [lmax+1];

        for (int i = 0; i < precision; i++) {
          ValueType el = i*M_PI/ (2.0* (precision-1));
          sigs[i] = exp (-bvalue* (ev1*cos (el) *cos (el) + ev2*sin (el) *sin (el)));
          Legendre::Plm_sph (AL, lmax, 0, cos (el));
          for (int l = 0; l < lmax/2+1; l++) SHT (i,l) = AL[2*l];
        }

        Math::Matrix<ValueType> SHinv (SHT.columns(), SHT.rows());
        return Math::mult (SH, pinv (SHinv, SHT), sigs);
      }




      template <typename ValueType > class PrecomputedFraction
      {
        public:
          PrecomputedFraction () : f1 (0.0), f2 (0.0) { }
          ValueType f1, f2;
          typename std::vector<ValueType>::const_iterator p1, p2;
      };


      template <typename ValueType> class PrecomputedAL
      {
        public:
          typedef ValueType value_type;

          PrecomputedAL () : lmax (0), ndir (0), nAL (0), inc (0.0), AL (NULL) { }
          PrecomputedAL (int up_to_lmax, int num_dir = 512) : AL (NULL) {
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
            inc = M_PI/ (ndir-1);
            AL.resize (ndir*nAL);
            value_type buf [lmax+1];

            for (int n = 0; n < ndir; n++) {
              typename std::vector<value_type>::iterator p = AL.begin() + n*nAL;
              value_type cos_el = Math::cos (n*inc);
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

          template <class ValueContainer>
          ValueType value (const ValueContainer& val, const Point<ValueType>& unit_dir) const {
            PrecomputedFraction<ValueType> f;
            set (f, Math::acos (unit_dir[2]));
            ValueType az = Math::atan2 (unit_dir[1], unit_dir[0]);
            ValueType v = 0.0;
            for (int l = 0; l <= lmax; l+=2) 
              v += get (f,l,0) * val[index (l,0)];
            for (int m = 1; m <= lmax; m++) {
              ValueType c = Math::cos (m*az);
              ValueType s = Math::sin (m*az);
              for (int l = ( (m&1) ? m+1 : m); l <= lmax; l+=2)
                v += get (f,l,m) * (c * val[index (l,m)] + s * val[index (l,-m)]);
            }
            return v;
          }

        protected:
          int lmax, ndir, nAL;
          ValueType inc;
          std::vector<ValueType> AL;
      };






      template <typename ValueType> 
        inline ValueType get_peak (const ValueType* SH, int lmax, Point<ValueType>& unit_init_dir, PrecomputedAL<ValueType>* precomputer = NULL)
        {
          assert (unit_init_dir.valid());
          for (int i = 0; i < 50; i++) {
            ValueType az = atan2 (unit_init_dir[1], unit_init_dir[0]);
            ValueType el = acos (unit_init_dir[2]);
            ValueType amplitude, dSH_del, dSH_daz, d2SH_del2, d2SH_deldaz, d2SH_daz2;
            derivatives (SH, lmax, el, az, amplitude, dSH_del, dSH_daz, d2SH_del2, d2SH_deldaz, d2SH_daz2, precomputer);

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

            unit_init_dir += Point<ValueType> (del*cos (az) *cos (el) - daz*sin (az), del*sin (az) *cos (el) + daz*cos (az), -del*sin (el));
            unit_init_dir.normalise();

            if (dt < ANGLE_TOLERANCE) return amplitude;
          }

          unit_init_dir.invalidate();
          DEBUG ("failed to find SH peak!");
          return NAN;
        }







      template <typename ValueType> 
        inline void derivatives (
            const ValueType* SH, const int lmax, const ValueType elevation, const ValueType azimuth, ValueType& amplitude,
            ValueType& dSH_del, ValueType& dSH_daz, ValueType& d2SH_del2, ValueType& d2SH_deldaz, 
            ValueType& d2SH_daz2, PrecomputedAL<ValueType>* precomputer)
        {
          ValueType sel = sin (elevation);
          ValueType cel = cos (elevation);
          bool atpole = sel < 1e-4;

          dSH_del = dSH_daz = d2SH_del2 = d2SH_deldaz = d2SH_daz2 = 0.0;
          ValueType AL [NforL_mpos (lmax)];

          if (precomputer) {
            PrecomputedFraction<ValueType> f;
            precomputer->set (f, elevation);
            precomputer->get (AL, f);
          }
          else {
            ValueType buf [lmax+1];
            for (int m = 0; m <= lmax; m++) {
              Legendre::Plm_sph (buf, lmax, m, cel);
              for (int l = ( (m&1) ?m+1:m); l <= lmax; l+=2)
                AL[index_mpos (l,m)] = buf[l];
            }
          }

          amplitude = SH[0] * AL[0];
          for (int l = 2; l <= (int) lmax; l+=2) {
            const ValueType& v (SH[index (l,0)]);
            amplitude += v * AL[index_mpos (l,0)];
            dSH_del += v * sqrt (ValueType (l* (l+1))) * AL[index_mpos (l,1)];
            d2SH_del2 += v * (sqrt (ValueType (l* (l+1) * (l-1) * (l+2))) * AL[index_mpos (l,2)] - l* (l+1) * AL[index_mpos (l,0)]) /2.0;
          }

          for (int m = 1; m <= lmax; m++) {
            ValueType caz = cos (m*azimuth);
            ValueType saz = sin (m*azimuth);
            for (int l = ( (m&1) ? m+1 : m); l <= lmax; l+=2) {
              const ValueType& vp (SH[index (l,m)]);
              const ValueType& vm (SH[index (l,-m)]);
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


    }
  }
}

#endif
