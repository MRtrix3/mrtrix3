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
          compute_mask (temp);
        }

        //! Alternative constructor that uses a separate image to construct the mask
        /* If the cost of accessing the underlying image is high (e.g. bootstrapping),
         * this constructor enables use of the original plain data in the sampling of
         * image values to construct the implicit mask */
        template <class ImageType>
        Masked (const typename InterpType::image_type& parent,
                const ImageType& masking_data,
                const value_type value_when_out_of_bounds = Base<typename InterpType::image_type>::default_out_of_bounds_value()) :
            InterpType (parent, value_when_out_of_bounds),
            mask (Image<bool>::scratch (make_mask_header (masking_data), "scratch binary mask for masked interpolator"))
        {
          assert (dimensions_match (parent, masking_data, 0, 3));
          ImageType temp (masking_data);
          if (masking_data.ndim() == 3)
            copy (temp, mask);
          else
            compute_mask (temp);
        }

        //! Set the current position to <b>voxel space</b> position \a pos
        /* Unlike other interpolators, this involves checking the value of
         * an internally stored mask, which tracks those voxels that contain
         * at least one finite & non-zero value.
         *
         * See file interp/base.h for details. */
        template <class VectorType>
        bool voxel (const VectorType& pos) {
          if (InterpType::set_out_of_bounds (pos))
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

        template <class ImageType>
        static Header make_mask_header (const ImageType& image)
        {
          Header H (image);
          H.ndim() = 3;
          H.datatype() = DataType::Bit;
          return H;
        }

        template <class ImageType>
        void compute_mask (ImageType& data)
        {
          // True if at least one finite & non-zero value
          for (auto l_voxel = Loop("pre-computing implicit voxel mask for \"" + data.name() + "\"", mask) (mask); l_voxel; ++l_voxel) {
            assign_pos_of (mask, 0, 3).to (data);
            bool value = false;
            for (auto l_inner = Loop(data, 3) (data); l_inner; ++l_inner) {
              if (data.value()) {
                value = true;
                continue;
              }
            }
            mask.value() = value;
          }
        }

    };



    //! @}

  }
}

#endif


