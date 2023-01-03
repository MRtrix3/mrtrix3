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

#ifndef __interp_masked_h__
#define __interp_masked_h__

#include "datatype.h"
#include "algo/loop.h"
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
     * one non-NaN & non-zero value in the voxel)
     */
    template <class InterpType>
    class Masked : public InterpType { MEMALIGN(Masked<InterpType>)
      public:
        using typename InterpType::value_type;

        Masked (const typename InterpType::image_type& parent,
                const value_type value_when_out_of_bounds = Base<typename InterpType::image_type>::default_out_of_bounds_value()) :
            InterpType (parent, value_when_out_of_bounds) { }

        //! Set the current position to <b>voxel space</b> position \a pos
        /* Unlike other interpolators, this sets the .index() location of
         * the parent image, and checks to see whether or not there is
         * any finite & non-zero data present; if there is not, then the
         * function returns false, as though the location is outside of the
         * image FoV.
         *
         * See file interp/base.h for details. */
        template <class VectorType>
        bool voxel (const VectorType& pos)
        {
          if (InterpType::set_out_of_bounds (pos))
            return false;
          InterpType::image_type::index(0) = std::round (pos[0]);
          InterpType::image_type::index(1) = std::round (pos[1]);
          InterpType::image_type::index(2) = std::round (pos[2]);
          for (auto l_inner = Loop(*this, 3) (*this); l_inner; ++l_inner) {
            if (InterpType::image_type::value())
              return InterpType::voxel (pos);
          }
          InterpType::set_out_of_bounds (true);
          return true;
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

    };



    //! @}

  }
}

#endif


