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

#ifndef __image_interp_reslice_h__
#define __image_interp_reslice_h__

#include "image/transform.h"
#include "image/value.h"
#include "image/copy.h"
#include "image/position.h"
#include "image/interp/base.h"

namespace MR
{
  namespace Image
  {
    namespace Interp
    {

      extern const Math::Matrix<float> NoOp;
      extern const std::vector<int> AutoOverSample;

      //! \addtogroup interp
      // @{

      //! a DataSet providing interpolated values from another DataSet
      /*! the Reslice class provides a DataSet interface to data interpolated
       * using the specified Interpolator class from the DataSet \a original. The
       * Reslice object will have the same dimensions, voxel sizes and
       * transform as the \a reference DataSet. Any of the interpolator classes
       * (currently Interp::Nearest, Interp::Linear, and Interp::Cubic) can be
       * used.
       *
       * For example:
       * \code
       * Image::Header reference = ...;     // the reference header
       * Image::Header header = ...;        // the actual header of the data
       * Image::Voxel<float> data (header); // to access the corresponding data
       *
       * // create a Reslice object to regrid 'data' according to the
       * // dimensions, etc. of 'reference', using cubic interpolation:
       * DataSet::Interp::Reslice<
       *       DataSet::Interp::Cubic,
       *       Image::Voxel<float>,
       *       Image::Header>   regridder (data, reference);
       *
       * // this class can be used like any other DataSet class, e.g.:
       * Image::Voxel<float> output (...);
       * DataSet::copy (output, regridder);
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
       * \sa DataSet::Interp::reslice()
       */
      template <template <class Set, typename T> class Interpolator, class Set, class RefSet, typename T>
      class Reslice
      {
        public:
          typedef typename Set::value_type value_type;
          typedef T pos_type;

          Reslice (Set& original, RefSet& reference,
                   const Math::Matrix<pos_type>& operation = NoOp,
                   const std::vector<int>& oversample = AutoOverSample,
                   const std::string& description = "") :
            D (original),
            interp (D),
            transform_matrix (reference.transform()),
            descriptor (description.empty() ? D.name() + " [resliced]" : description) {
            assert (reference.ndim() >= 3);
            x[0] = x[1] = x[2] = 0;
            N[0] = reference.dim (0);
            N[1] = reference.dim (1);
            N[2] = reference.dim (2);
            V[0] = reference.vox (0);
            V[1] = reference.vox (1);
            V[2] = reference.vox (2);

            Math::Matrix<pos_type> Mr, Mo;
            Image::Transform::voxel2scanner (Mr, reference);
            Image::Transform::voxel2scanner (Mo, original);

            if (operation.is_set()) {
              Math::Matrix<T> Mt;
              Math::mult (Mt, operation, Mo);
              Mo.swap (Mt);
            }

            Math::Matrix<pos_type> iMo;
            Math::LU::inv (iMo, Mo);
            Math::mult (M, iMo, Mr);

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
              Image::Transform::apply (x0, M, x1);
              x1[0] = 1.0;
              Image::Transform::apply (y, M, x1);
              OS[0] = Math::ceil (0.999 * (y-x0).norm());
              x1[0] = 0.0;
              x1[1] = 1.0;
              Image::Transform::apply (y, M, x1);
              OS[1] = Math::ceil (0.999 * (y-x0).norm());
              x1[1] = 0.0;
              x1[2] = 1.0;
              Image::Transform::apply (y, M, x1);
              OS[2] = Math::ceil (0.999 * (y-x0).norm());
            }

            if (OS[0] * OS[1] * OS[2] > 1) {
              info ("using oversampling factors [ " + str (OS[0]) + " " + str (OS[1]) + " " + str (OS[2]) + " ]");
              oversampling = true;
              norm = 1.0;
              for (size_t i = 0; i < 3; ++i) {
                inc[i] = 1.0/pos_type (OS[i]);
                from[i] = 0.5* (inc[i]-1.0);
                norm *= OS[i];
              }
              norm = 1.0 / norm;
            }
            else oversampling = false;
          }


          const std::string& name () const {
            return (descriptor);
          }
          size_t  ndim () const {
            return (D.ndim());
          }
          int     dim (size_t axis) const {
            return (axis < 3 ? N[axis] : D.dim (axis));
          }
          ssize_t stride (size_t axis) const {
            return (D.stride (axis));
          }
          float   vox (size_t axis) const {
            return (axis < 3 ? V[axis] : D.vox (axis));
          }

          const Math::Matrix<pos_type>& transform () const {
            return (transform_matrix);
          }

          void    reset () {
            x[0] = x[1] = x[2] = 0;
            for (size_t n = 3; n < D.ndim(); ++n) D[n] = 0;
          }

          value_type value () {
            if (oversampling) {
              Point<pos_type> d (x[0]+from[0], x[1]+from[1], x[2]+from[2]);
              value_type ret = 0.0;
              Point<pos_type> s;
              for (int z = 0; z < OS[2]; ++z) {
                s[2] = d[2] + z*inc[2];
                for (int y = 0; y < OS[1]; ++y) {
                  s[1] = d[1] + y*inc[1];
                  for (int x = 0; x < OS[0]; ++x) {
                    s[0] = d[0] + x*inc[0];
                    Point<pos_type> pos;
                    Image::Transform::apply (pos, M, s);
                    interp.voxel (pos);
                    if (!interp) continue;
                    else ret += interp.value();
                  }
                }
              }
              return (ret * norm);
            }
            else {
              Point<pos_type> pos;
              Transform::apply (pos, M, x);
              interp.voxel (pos);
              return (!interp ? 0.0 : interp.value());
            }
          }

          Position<Reslice<Interpolator,Set,RefSet,pos_type> > operator[] (size_t axis) {
            return (Position<Reslice<Interpolator,Set,RefSet,pos_type> > (*this, axis));
          }

        private:
          Set& D;
          Interpolator<Set,pos_type> interp;
          size_t N[3];
          ssize_t x[3];
          bool oversampling;
          int OS[3];
          pos_type from[3], inc[3];
          pos_type norm;

          pos_type V[3];
          Math::Matrix<pos_type> M, transform_matrix;
          std::string descriptor;

          ssize_t get_pos (size_t axis) const {
            return (axis < 3 ? x[axis] : D[axis]);
          }
          void set_pos (size_t axis, ssize_t position) {
            if (axis < 3) x[axis] = position;
            else D[axis] = position;
          }
          void move_pos (size_t axis, ssize_t increment) {
            if (axis < 3) x[axis] += increment;
            else D[axis] += increment;
          }

          friend class Position<Reslice<Interpolator,Set,RefSet,pos_type> >;
      };


      //! convenience function to regrid one DataSet onto another.
      /*! This function resamples (regrids) the DataSet \a source onto the
       * DataSet& \a destination, using the Interp::Reslice class.
       *
       * For example:
       * \code
       * // source and destination data:
       * Image::Header source_header (...);
       * Image::Voxel<float> source (source_header);
       *
       * Image::Header destination_header (...);
       * Image::Voxel<float> destination (destination_header);
       *
       * // regrid source onto destination using linear interpolation:
       * DataSet::Interp::reslice<DataSet::Interp::Linear> (destination, source);
       * \endcode
       */
      template <template <class Set, typename T> class Interpolator, class Set1, class Set2, typename T>
      void reslice (
        Set1& destination,
        Set2& source,
        const Math::Matrix<T>& operation = NoOp,
        const std::vector<int>& oversampling = AutoOverSample)
      {
        Reslice<Interpolator,Set2,Set1,T> interp (source, destination, operation, oversampling);
        Image::copy_with_progress (destination, interp);
      }


      //! @}
    }
  }
}

#endif



