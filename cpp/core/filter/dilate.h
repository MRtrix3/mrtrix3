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
#include "memory.h"

namespace MR::Filter {

/** \addtogroup Filters
  @{ */

//! a filter to dilate a mask
/*!
 * Typical usage:
 * \code
 * auto input = Image<bool>::open (argument[0]);
 *
 * Filter::Dilate dilate (input);
 *
 * Image<bool> output (dilate, argument[1]);
 * dilate (input, output);
 *
 * \endcode
 */
class Dilate : public Base {

public:
  template <class HeaderType> Dilate(const HeaderType &in) : Base(in), npass(1), do_26_connectivity(false) {
    datatype_ = DataType::Bit;
  }

  template <class HeaderType>
  Dilate(const HeaderType &in, std::string_view message) : Base(in, message), npass(1), do_26_connectivity(false) {
    datatype_ = DataType::Bit;
  }

  template <class InputImageType, class OutputImageType>
  void operator()(InputImageType &input, OutputImageType &output) {
    std::shared_ptr<Image<bool>> in(new Image<bool>(Image<bool>::scratch(input)));
    copy(input, *in);
    std::shared_ptr<Image<bool>> out;
    std::shared_ptr<ProgressBar> progress(!message.empty() ? new ProgressBar(message, npass + 1) : nullptr);

    for (unsigned int pass = 0; pass < npass; pass++) {
      out = std::make_shared<Image<bool>>(Image<bool>::scratch(input));
      for (auto l = Loop(*in)(*in, *out); l; ++l)
        out->value() = dilate(*in);
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
  bool dilate(Image<bool> &in) {
    if (in.value())
      return true;
    const ssize_t x = in.index(0);
    const ssize_t y = in.index(1);
    const ssize_t z = in.index(2);
    bool result = false;
    for (ssize_t dz = -1; dz <= 1 && !result; ++dz) {
      for (ssize_t dy = -1; dy <= 1 && !result; ++dy) {
        for (ssize_t dx = -1; dx <= 1 && !result; ++dx) {
          const int nonzero = (dx != 0) + (dy != 0) + (dz != 0);
          if (nonzero == 0)
            continue;
          if (!do_26_connectivity && nonzero > 1)
            continue;
          const ssize_t nx = x + dx;
          const ssize_t ny = y + dy;
          const ssize_t nz = z + dz;
          if (nx < 0 || nx >= in.size(0) || ny < 0 || ny >= in.size(1) || nz < 0 || nz >= in.size(2))
            continue;
          in.index(0) = nx;
          in.index(1) = ny;
          in.index(2) = nz;
          if (in.value())
            result = true;
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
