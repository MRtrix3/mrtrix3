/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#pragma once

#include "algo/copy.h"
#include "algo/loop.h"
#include "filter/base.h"
#include "image.h"
#include "image_helpers.h"
#include "memory.h"
#include "progressbar.h"

namespace MR::Filter {

/** \addtogroup Filters
  @{ */

//! a filter to erode a mask
/*!
 * Typical usage:
 * \code
 * auto input = Image<bool>::open (argument[0]);
 *
 * Filter::Erode erode (input);
 *
 * Image<bool> output (erode, argument[1]);
 * erode (input, output);
 *
 * \endcode
 */
class Erode : public Base {

public:
  template <class HeaderType> Erode(const HeaderType &in) : Base(in), npass(1), do_26_connectivity(false) {
    check_3D_nonunity(in);
    datatype_ = DataType::Bit;
  }

  template <class HeaderType>
  Erode(const HeaderType &in, std::string_view message) : Base(in, message), npass(1), do_26_connectivity(false) {
    check_3D_nonunity(in);
    datatype_ = DataType::Bit;
  }

  template <class InputImageType, class OutputImageType>
  void operator()(InputImageType &input, OutputImageType &output) {
    std::shared_ptr<Image<bool>> in = std::make_shared<Image<bool>>(Image<bool>::scratch(input));
    copy(input, *in);
    std::shared_ptr<Image<bool>> out;
    std::shared_ptr<ProgressBar> progress(!message.empty() ? new ProgressBar(message, npass + 1) : nullptr);

    for (unsigned int pass = 0; pass < npass; pass++) {
      out = std::make_shared<Image<bool>>(Image<bool>::scratch(input));
      for (auto l = Loop(*in)(*in, *out); l; ++l)
        out->value() = erode(*in);

      if (pass < npass - 1)
        in = out;
      if (progress)
        ++(*progress);
    }
    copy(*out, output);
  }

  void set_npass(unsigned int npasses) { npass = npasses; }

  //! select the voxel neighbourhood tested for adjacency
  /*! \param value if \c false (the default) only the 6 voxels sharing a face
   *    with the central voxel are tested; if \c true the full 26-voxel
   *    neighbourhood (face-, edge- and corner-adjacent) is tested. */
  void set_26_connectivity(bool value) { do_26_connectivity = value; }

protected:
  bool erode(Image<bool> &in) {
    if (!in.value())
      return false;
    // A voxel on the field-of-view boundary has at least one neighbour outside
    //   the image (for both connectivities); such a neighbour counts as
    //   background and the voxel is therefore eroded.
    if ((in.index(0) == 0) || (in.index(0) == in.size(0) - 1) || (in.index(1) == 0) ||
        (in.index(1) == in.size(1) - 1) || (in.index(2) == 0) || (in.index(2) == in.size(2) - 1))
      return false;
    const ssize_t x = in.index(0);
    const ssize_t y = in.index(1);
    const ssize_t z = in.index(2);
    bool result = true;
    // All neighbours are guaranteed in-bounds by the boundary test above.
    for (ssize_t dz = -1; dz <= 1 && result; ++dz) {
      for (ssize_t dy = -1; dy <= 1 && result; ++dy) {
        for (ssize_t dx = -1; dx <= 1 && result; ++dx) {
          const int nonzero = (dx != 0) + (dy != 0) + (dz != 0);
          if (nonzero == 0)
            continue;
          if (!do_26_connectivity && nonzero > 1)
            continue;
          in.index(0) = x + dx;
          in.index(1) = y + dy;
          in.index(2) = z + dz;
          if (!in.value())
            result = false;
        }
      }
    }
    in.index(0) = x;
    in.index(1) = y;
    in.index(2) = z;
    return result;
  }

  unsigned int npass;
  bool do_26_connectivity;
};
//! @}
} // namespace MR::Filter
