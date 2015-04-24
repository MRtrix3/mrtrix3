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

#ifndef __adapter_reslice_h__
#define __adapter_reslice_h__

#include "image.h"
#include "transform.h"

namespace MR
{
  namespace Adapter
  {

    extern const Math::Matrix<default_type> NoTransform;
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
     * // reference header:
     * auto reference = Header::open (argument[0]);
     * // input data to be resliced:
     * auto data = Header::open (argument[1]).get_image<float>(); 
     *
     * auto regridder = Adapter::make_reslice<Interp::Cubic> (data, reference);
     * auto out = Header::create (argument[2], regridder).get_image<float>();
     *
     * // copy data from regridder to output
     * copy (regridder, out);
     * \endcode
     *
     * It is also possible to supply an additional transform to be applied to
     * the data, using the \a transform parameter. The transform will be
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
    template <template <class ImageType> class Interpolator, class ImageType>
      class Reslice 
    {
      public:
        typedef typename ImageType::value_type value_type;

        template <class HeaderType>
          Reslice (const ImageType& original,
              const HeaderType& reference,
              const Math::Matrix<default_type>& transform = NoTransform,
              const std::vector<int>& oversample = AutoOverSample,
              const value_type value_when_out_of_bounds = Transform::default_out_of_bounds_value<value_type>()) :
            interp (original, value_when_out_of_bounds),
            x ({ 0, 0, 0 }),
            dim ({ reference.size(0), reference.size(1), reference.size(2) }),
            vox ({ reference.voxsize(0), reference.voxsize(1), reference.voxsize(2) }),
            transform_ (reference.transform()),
            direct_transform (get_direct_transform (Transform(reference), Transform(original), transform)) {
              assert (ndim() >= 3);

              if (oversample.size()) {
                assert (oversample.size() == 3);
                if (oversample[0] < 1 || oversample[1] < 1 || oversample[2] < 1)
                  throw Exception ("oversample factors must be greater than zero");
                OS[0] = oversample[0];
                OS[1] = oversample[1];
                OS[2] = oversample[2];
              }
              else {
                Point<default_type> x1 (0.0,0.0,0.0), y, x0;
                Transform::transform_position (y, direct_transform, x1);
                x1[0] = 1.0;
                Transform::transform_position (x0, direct_transform, x1);
                OS[0] = std::ceil (0.999 * (y-x0).norm());
                x1[0] = 0.0;
                x1[1] = 1.0;
                Transform::transform_position (x0, direct_transform, x1);
                OS[1] = std::ceil (0.999 * (y-x0).norm());
                x1[1] = 0.0;
                x1[2] = 1.0;
                Transform::transform_position (x0, direct_transform, x1);
                OS[2] = std::ceil (0.999 * (y-x0).norm());
              }

              if (OS[0] * OS[1] * OS[2] > 1) {
                INFO ("using oversampling factors [ " + str (OS[0]) + " " + str (OS[1]) + " " + str (OS[2]) + " ]");
                oversampling = true;
                norm = 1.0;
                for (size_t i = 0; i < 3; ++i) {
                  inc[i] = 1.0/default_type (OS[i]);
                  from[i] = 0.5* (inc[i]-1.0);
                  norm *= OS[i];
                }
                norm = 1.0 / norm;
              }
              else oversampling = false;
            }


        size_t ndim () const { return interp.ndim(); }
        int size (size_t axis) const { return axis < 3 ? dim[axis]: interp.size (axis); }
        default_type voxsize (size_t axis) const { return axis < 3 ? vox[axis] : interp.voxsize (axis); }
        const Math::Matrix<default_type>& transform () const { return transform_; }
        const std::string& name () const { return interp.name(); }

        void reset () {
          x[0] = x[1] = x[2] = 0;
          for (size_t n = 3; n < interp.ndim(); ++n)
            interp.index(n) = 0;
        }

        value_type& value () {
          if (oversampling) {
            Point<default_type> d (x[0]+from[0], x[1]+from[1], x[2]+from[2]);
            value_type result = 0.0;
            Point<default_type> s;
            for (int z = 0; z < OS[2]; ++z) {
              s[2] = d[2] + z*inc[2];
              for (int y = 0; y < OS[1]; ++y) {
                s[1] = d[1] + y*inc[1];
                for (int x = 0; x < OS[0]; ++x) {
                  s[0] = d[0] + x*inc[0];
                  Point<default_type> pos;
                  Transform::transform_position (pos, direct_transform, s);
                  interp.voxel (pos);
                  if (!interp) continue;
                  else result += interp.value();
                }
              }
            }
            result *= norm;
            return result;
          }

          Point<default_type> pos;
          Transform::transform_position (pos, direct_transform, x);
          interp.voxel (pos);
          return interp.value();
        }

        auto index (size_t axis) -> decltype(Helper::voxel_index(*this, axis)) { return { *this, axis }; }

      private:
        Interpolator<ImageType> interp;
        ssize_t x[3];
        const ssize_t dim[3];
        const default_type vox[3];
        bool oversampling;
        int OS[3];
        default_type from[3], inc[3];
        default_type norm;
        const Math::Matrix<default_type> transform_, direct_transform;

        ssize_t get_voxel_position (size_t axis) const {
          return axis < 3 ? x[axis] : interp.index(axis);
        }
        void set_voxel_position (size_t axis, ssize_t position) {
          if (axis < 3) x[axis] = position;
          else interp.index(axis) = position;
        }
        void move_voxel_position (size_t axis, ssize_t increment) {
          if (axis < 3) x[axis] += increment;
          else interp.index(axis) += increment;
        }

        static inline Math::Matrix<default_type> get_direct_transform (const Transform& reference, 
            const Transform& original, const Math::Matrix<default_type>& transform) {
          Math::Matrix<default_type> Mr, Mo, direct_transform;
          reference.voxel2scanner_matrix (Mr);
          original.scanner2voxel_matrix (Mo);

          if (transform.is_set()) {
            Math::Matrix<default_type> Mt;
            Math::mult (Mt, Mo, transform);
            Mo.swap (Mt);
          }

          Math::mult (direct_transform, Mo, Mr);
          return direct_transform;
        }

        friend class Helper::VoxelIndex<Reslice<Interpolator,ImageType> >;
    };

    //! @}

  }
}

#endif




