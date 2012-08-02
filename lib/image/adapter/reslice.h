/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 22/10/09.

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

#ifndef __image_adapter_reslice_h__
#define __image_adapter_reslice_h__

#include "image/info.h"
#include "image/transform.h"
#include "image/value.h"
#include "image/position.h"
#include "image/interp/base.h"

namespace MR
{
  namespace Image
  {
    namespace Adapter
    {

      extern const Math::Matrix<float> NoOp;
      extern const std::vector<int> AutoOverSample;

      //! \addtogroup interp
      // @{

      //! a Image::Voxel providing interpolated values from another Image::Voxel
      /*! the Reslice class provides a Image::Voxel interface to data
       * interpolated using the specified Interpolator class from the
       * Image::Voxel \a original. The Reslice object will have the same
       * dimensions, voxel sizes and transform as the \a reference Image::Info.
       * Any of the interpolator classes (currently Interp::Nearest,
       * Interp::Linear, and Interp::Cubic) can be used.
       *
       * For example:
       * \code
       * Image::Header reference = ...;     // the reference header
       * Image::Header header = ...;        // the actual header of the data
       * Image::Voxel<float> data (header); // to access the corresponding data
       *
       * // create a Reslice object to regrid 'data' according to the
       * // dimensions, etc. of 'reference', using cubic interpolation:
       * Image::Adapter::Reslice<
       *       Image::Interp::Cubic,
       *       Image::Voxel<float> >   regridder (data, reference);
       *
       * // this class can be used like any other Image::Voxel class, e.g.:
       * Image::Voxel<float> output (...);
       * Image::copy (output, regridder);
       * \endcode
       *
       * It is also possible to supply an additional transform to be applied to
       * the data, using the \a operation parameter. The transform will be
       * applied in the scanner coordinate system, and should map scanner-space
       * coordinates in the original image to scanner-space coordinates in the
       * reference image.
       *
       * To deal with possible aliasing due to sparse sampling of a
       * high-resolution image, the Reslice object may perform over-sampling,
       * whereby multiple samples are taken at regular sub-voxel intervals and
       * averaged. By default, oversampling will be performed along those axes
       * where it is deemed necessary. This can be over-ridden using the \a
       * oversampling parameter, which should contain one (integer)
       * over-sampling factor for each of the 3 imaging axes. Specifying the
       * vector [ 1 1 1 ] will therefore disable over-sampling.
       *
       * \sa Image::Interp::reslice()
       */
      template <template <class VoxelType> class Interpolator, class VoxelType>
        class Reslice : public ConstInfo
      {
        public:
          typedef typename VoxelType::value_type value_type;

          using typename ConstInfo::name;

          template <class InfoType>
            Reslice (const VoxelType& original, 
                const InfoType& reference,
                const Math::Matrix<float>& operation = NoOp,
                const std::vector<int>& oversample = AutoOverSample) :
              ConstInfo (reference),
              interp (original) {
                assert (ndim() >= 3);
                x[0] = x[1] = x[2] = 0;

                Math::Matrix<float> Mr, Mo;
                Image::Transform::voxel2scanner (Mr, reference);
                Image::Transform::voxel2scanner (Mo, original);

                if (operation.is_set()) {
                  Math::Matrix<float> Mt;
                  Math::mult (Mt, operation, Mo);
                  Mo.swap (Mt);
                }

                Math::Matrix<float> iMo;
                Math::LU::inv (iMo, Mo);
                Math::mult (direct_transform, iMo, Mr);

                if (oversample.size()) {
                  assert (oversample.size() == 3);
                  if (oversample[0] < 1 || oversample[1] < 1 || oversample[2] < 1)
                    throw Exception ("oversample factors must be greater than zero");
                  OS[0] = oversample[0];
                  OS[1] = oversample[1];
                  OS[2] = oversample[2];
                }
                else {
                  Point<value_type> y, x0, x1 (0.0,0.0,0.0);
                  Image::Transform::apply (x0, direct_transform, x1);
                  x1[0] = 1.0;
                  Image::Transform::apply (y, direct_transform, x1);
                  OS[0] = Math::ceil (0.999 * (y-x0).norm());
                  x1[0] = 0.0;
                  x1[1] = 1.0;
                  Image::Transform::apply (y, direct_transform, x1);
                  OS[1] = Math::ceil (0.999 * (y-x0).norm());
                  x1[1] = 0.0;
                  x1[2] = 1.0;
                  Image::Transform::apply (y, direct_transform, x1);
              OS[2] = Math::ceil (0.999 * (y-x0).norm());
            }

            if (OS[0] * OS[1] * OS[2] > 1) {
              INFO ("using oversampling factors [ " + str (OS[0]) + " " + str (OS[1]) + " " + str (OS[2]) + " ]");
              oversampling = true;
              norm = 1.0;
              for (size_t i = 0; i < 3; ++i) {
                inc[i] = 1.0/float (OS[i]);
                from[i] = 0.5* (inc[i]-1.0);
                norm *= OS[i];
              }
              norm = 1.0 / norm;
            }
            else oversampling = false;
          }


          size_t ndim () const {
            return interp.ndim();
          }
          int dim (size_t axis) const {
            return axis < 3 ? ConstInfo::dim (axis): interp.dim (axis);
          }
          float vox (size_t axis) const {
            return axis < 3 ? ConstInfo::vox (axis) : interp.vox (axis);
          }

          void reset () {
            x[0] = x[1] = x[2] = 0;
            for (size_t n = 3; n < interp.ndim(); ++n) 
              interp[n] = 0;
          }

          value_type value () {
            if (oversampling) {
              Point<float> d (x[0]+from[0], x[1]+from[1], x[2]+from[2]);
              value_type ret = 0.0;
              Point<float> s;
              for (int z = 0; z < OS[2]; ++z) {
                s[2] = d[2] + z*inc[2];
                for (int y = 0; y < OS[1]; ++y) {
                  s[1] = d[1] + y*inc[1];
                  for (int x = 0; x < OS[0]; ++x) {
                    s[0] = d[0] + x*inc[0];
                    Point<float> pos;
                    Image::Transform::apply (pos, direct_transform, s);
                    interp.voxel (pos);
                    if (!interp) continue;
                    else ret += interp.value();
                  }
                }
              }
              return ret * norm;
            }
            else {
              Point<float> pos;
              Transform::apply (pos, direct_transform, x);
              interp.voxel (pos);
              return interp.value();
            }
          }

          Position<Reslice<Interpolator,VoxelType> > operator[] (size_t axis) {
            return Position<Reslice<Interpolator,VoxelType> > (*this, axis);
          }

        private:
          Interpolator<VoxelType> interp;
          ssize_t x[3];
          bool oversampling;
          int OS[3];
          float from[3], inc[3];
          float norm;
          Math::Matrix<float> direct_transform;

          ssize_t get_pos (size_t axis) const {
            return axis < 3 ? x[axis] : interp[axis];
          }
          void set_pos (size_t axis, ssize_t position) {
            if (axis < 3) x[axis] = position;
            else interp[axis] = position;
          }
          void move_pos (size_t axis, ssize_t increment) {
            if (axis < 3) x[axis] += increment;
            else interp[axis] += increment;
          }

          friend class Position<Reslice<Interpolator,VoxelType> >;
      };

      //! @}
    }
  }
}

#endif




