/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#ifndef __adapter_reslice_h__
#define __adapter_reslice_h__

#include <type_traits>

#include "image.h"
#include "transform.h"
#include "types.h"
#include "interp/base.h"

namespace MR
{
  namespace Adapter
  {



    namespace
    {
      // Partial specialisation for boolean value_type in order to avoid compiler
      //   warning regarding use of multiplication when assigning to a boolean
      template <typename value_type>
      typename std::enable_if<std::is_same<value_type, bool>::value, value_type>::type
      inline normalise (const default_type sum, const default_type norm)
      {
        return ((sum*norm) >= 0.5) ? true : false;
      }

      // Partial specialisation to invoke round-to-nearest when taking an average of integers
      template <typename value_type>
      typename std::enable_if<!std::is_same<value_type, bool>::value && std::is_integral<value_type>::value, value_type>::type
      inline normalise (const default_type sum, const default_type norm)
      {
        return value_type(std::round (sum*norm));
      }

      template <typename value_type>
      typename std::enable_if<std::is_floating_point<value_type>::value, value_type>::type
      inline normalise (const default_type sum, const default_type norm)
      {
        return (sum * norm);
      }
    }



    extern const transform_type NoTransform;
    extern const vector<int> AutoOverSample;

    //! \addtogroup interp
    // @{

    //! an Image providing interpolated values from another Image
    /*! the Reslice class provides an Image interface to data
     * interpolated using the specified Interpolator class from the
     * Image \a original. The Reslice object will have the same
     * dimensions, voxel sizes and transform as the \a reference HeaderType.
     * Any of the interpolator classes (currently Interp::Nearest,
     * Interp::Linear, Interp::Cubic and Interp::Sinc) can be used.
     *
     * For example:
     * \code
     * // reference header:
     * auto reference = Header::open (argument[0]);
     * // input data to be resliced:
     * auto input = Image<float>::open (argument[1]);
     *
     * Adapter::Reslice<Interp::Cubic, Image<float> > reslicer (input, reference);
     * auto output = Image::create<float> (argument[2], reslicer);
     *
     * // copy data from regridder to output
     * copy (reslicer, output);
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
     * \sa Interp::reslice()
     */
    template <template <class ImageType> class Interpolator, class ImageType>
      class Reslice :
        public ImageBase<Reslice<Interpolator,ImageType>,typename ImageType::value_type>
    { MEMALIGN (Reslice<Interpolator, ImageType>)
      public:

        using value_type = typename ImageType::value_type;


        template <class HeaderType>
          Reslice (const ImageType& original,
                   const HeaderType& reference,
                   const transform_type& transform = NoTransform,
                   const vector<int>& oversample = AutoOverSample,
                   const value_type value_when_out_of_bounds = Interp::Base<ImageType>::default_out_of_bounds_value()) :
            interp (original, value_when_out_of_bounds),
            x { 0, 0, 0 },
            dim { reference.size(0), reference.size(1), reference.size(2) },
            vox { reference.spacing(0), reference.spacing(1), reference.spacing(2) },
            transform_ (reference.transform()),
            direct_transform (Transform(original).scanner2voxel * transform * Transform(reference).voxel2scanner) {
              using namespace Eigen;
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
                Vector3 y = direct_transform * Vector3 (0.0, 0.0, 0.0);
                Vector3 x0 = direct_transform * Vector3 (1.0, 0.0, 0.0);
                OS[0] = std::ceil ((1.0-std::numeric_limits<default_type>::epsilon()) * (y-x0).norm());
                x0 = direct_transform * Vector3 (0.0, 1.0, 0.0);
                OS[1] = std::ceil ((1.0-std::numeric_limits<default_type>::epsilon()) * (y-x0).norm());
                x0 = direct_transform * Vector3 (0.0, 0.0, 1.0);
                OS[2] = std::ceil ((1.0-std::numeric_limits<default_type>::epsilon()) * (y-x0).norm());
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
        bool valid () const { return interp.valid(); }
        int size (size_t axis) const { return axis < 3 ? dim[axis]: interp.size (axis); }
        default_type spacing (size_t axis) const { return axis < 3 ? vox[axis] : interp.spacing (axis); }
        const transform_type& transform () const { return transform_; }
        const std::string& name () const { return interp.name(); }

        ssize_t stride (size_t axis) const {
          return interp.stride (axis);
        }

        void reset () {
          x[0] = x[1] = x[2] = 0;
          for (size_t n = 3; n < interp.ndim(); ++n)
            interp.index(n) = 0;
        }

        value_type value () {
          using namespace Eigen;
          if (oversampling) {
            Vector3 d (x[0]+from[0], x[1]+from[1], x[2]+from[2]);
            default_type sum (0.0);
            Vector3 s;
            for (int z = 0; z < OS[2]; ++z) {
              s[2] = d[2] + z*inc[2];
              for (int y = 0; y < OS[1]; ++y) {
                s[1] = d[1] + y*inc[1];
                for (int x = 0; x < OS[0]; ++x) {
                  s[0] = d[0] + x*inc[0];
                  if (interp.voxel (direct_transform * s))
                    sum += interp.value();
                }
              }
            }
            return normalise<value_type> (sum, norm);
          }
          interp.voxel (direct_transform * Vector3 (x[0], x[1], x[2]));
          return interp.value();
        }

        ssize_t get_index (size_t axis) const { return axis < 3 ? x[axis] : interp.index(axis); }
        void move_index (size_t axis, ssize_t increment) {
          if (axis < 3) x[axis] += increment;
          else interp.index(axis) += increment;
        }

      private:
        Interpolator<ImageType> interp;
        ssize_t x[3];
        const ssize_t dim[3];
        const default_type vox[3];
        bool oversampling;
        int OS[3];
        default_type from[3], inc[3];
        default_type norm;
        const transform_type transform_, direct_transform;

    };

    //! @}

  }
}

#endif




