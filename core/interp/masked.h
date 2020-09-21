/* Copyright (c) 2008-2020 the MRtrix3 contributors.
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

#ifndef __interp_masked_h__
#define __interp_masked_h__

#include "datatype.h"
#include "interp/base.h"

namespace MR
{
  namespace Interp
  {

    //! \addtogroup interp
    // @{

    //! Implicit masking for interpolator class
    /*! Wrap a image interpolator in a way that returns false not only if
     * the position is outside of the field of view of the image, but also
     * if the image is zero-filled or contains non-finite values at that
     * location.
     *
     * (NaN values are permitted in order to be compatible with 3-vectors
     * images, i.e. sets of XYZ triplets; but there needs to be at least
     * one non-NaN value in the voxel)
     */

    template <class InterpType>
    class Masked : public InterpType { MEMALIGN(Masked<InterpType>)
      public:
        using typename InterpType::value_type;

        Masked (const typename InterpType::image_type& parent,
                const value_type value_when_out_of_bounds = Base<typename InterpType::image_type>::default_out_of_bounds_value()) :
            InterpType (parent, value_when_out_of_bounds),
            mask (Image<bool>::scratch (make_mask_header (parent), "scratch binary mask for masked interpolator"))
        {
          typename InterpType::image_type temp (parent);
          // True if at least one finite & non-zero value
          for (auto l_voxel = Loop(mask) (temp, mask); l_voxel; ++l_voxel) {
            bool value = false;
            for (auto l_inner = Loop(temp, 3) (temp); l_inner; ++l_inner) {
              if (temp.value()) {
                value = true;
                continue;
              }
            }
            mask.value() = value;
          }
        }

        //! Set the current position to <b>voxel space</b> position \a pos
        /* Unlike other interpolators, this involves checking the value of
         * an internally stored mask, which tracks those voxels that contain
         * at least one finite & non-zero value.
         *
         * See file interp/base.h for details. */
        template <class VectorType>
        bool voxel (const VectorType& pos) {
          InterpType::intravoxel_offset (pos);
          if (InterpType::out_of_bounds)
            return false;
          mask.index(0) = std::round (pos[0]);
          mask.index(1) = std::round (pos[1]);
          mask.index(2) = std::round (pos[2]);
          if (!mask.value())
            return false;
          return InterpType::voxel (pos);
        }

        //! Set the current position to <b>image space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
        FORCE_INLINE bool image (const VectorType& pos) {
          return voxel (Transform::voxelsize.inverse() * pos.template cast<default_type>());
        }

        //! Set the current position to the <b>scanner space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
        FORCE_INLINE bool scanner (const VectorType& pos) {
          return voxel (Transform::scanner2voxel * pos.template cast<default_type>());
        }

      protected:
        Image<bool> mask;

        static Header make_mask_header (const typename InterpType::image_type& image)
        {
          Header H (image);
          H.ndim() = 3;
          H.datatype() = DataType::Bit;
          return H;
        }

    };



    //! @}

  }
}

#endif


