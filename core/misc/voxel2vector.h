/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __misc_voxel2vector_h__
#define __misc_voxel2vector_h__

#include <limits>

#include "exception.h"
#include "header.h"
#include "image.h"
#include "algo/loop.h"
#include "types.h"

#include "adapter/replicate.h"


namespace MR
{



  // A class to achieve a mapping from a voxel position in an image
  //   with any number of axes, to an index within a 1D vector of data.
  class Voxel2Vector
  { MEMALIGN(Voxel2Vector)
    public:

      typedef uint32_t index_t;

      static const index_t invalid = std::numeric_limits<index_t>::max();

      template <class MaskType>
      Voxel2Vector (MaskType& mask, const Header& data);

      template <class MaskType>
      Voxel2Vector (MaskType& mask) :
          Voxel2Vector (mask, Header(mask)) { }

      size_t size() const { return reverse.size(); }

      const vector<index_t>& operator[] (const size_t index) const {
        assert (index < reverse.size());
        return reverse[index];
      }

      template <class PosType>
      index_t operator() (const PosType& pos) const {
        Image<index_t> temp (forward); // For thread-safety
        assign_pos_of (pos).to (temp);
        if (is_out_of_bounds (temp))
          return invalid;
        return temp.value();
      }

    private:
      Image<index_t> forward;
      vector< vector<index_t> > reverse;
  };



  template <class MaskType>
  Voxel2Vector::Voxel2Vector (MaskType& mask, const Header& data) :
      forward (Image<index_t>::scratch (data, "Voxel to vector index conversion scratch image"))
  {
    if (!dimensions_match (mask, data, 0, std::min (mask.ndim(), data.ndim())))
      throw Exception ("Dimension mismatch between image data and processing mask");
    // E.g. Mask may be 3D but data are 4D; for any voxel where the mask is
    //   true, want to include data from all volumes
    Adapter::Replicate<Image<bool>> r_mask (mask, data);
    // Loop in axis order so that those voxels contiguous in memory are still
    //   contiguous in the vectorised data
    index_t counter = 0;
    for (auto l = Loop(data) (r_mask, forward); l; ++l) {
      if (r_mask.value()) {
        forward.value() = counter++;
        vector<index_t> pos;
        for (size_t index = 0; index != data.ndim(); ++index)
          pos.push_back (forward.index(index));
        reverse.push_back (pos);
      } else {
        forward.value() = invalid;
      }
    }
    DEBUG ("Voxel2Vector class has " + str(reverse.size()) + " non-zero entries");
  }



}


#endif
