/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include "metadata/slice_encoding.h"

#include "header.h"
#include "metadata/bids.h"

namespace MR::Metadata::SliceEncoding {

void transform_for_image_load(Header& header) {
  // If there's any slice encoding direction information present in the
  //   header, that's also necessary to update here
  auto slice_encoding_it = header.keyval().find("SliceEncodingDirection");
  auto slice_timing_it = header.keyval().find("SliceTiming");
  if (!(slice_encoding_it == header.keyval().end() && slice_timing_it == header.keyval().end())) {
    const Metadata::BIDS::axis_vector_type
        orig_dir(slice_encoding_it == header.keyval().end()                   //
                 ? Metadata::BIDS::axis_vector_type({0, 0, 1})                //
                 : Metadata::BIDS::axisid2vector(slice_encoding_it->second)); //
    Metadata::BIDS::axis_vector_type new_dir;
    for (size_t axis = 0; axis != 3; ++axis)
      new_dir[axis] = orig_dir[header.realignment().permutation(axis)]                                  //
                    * (header.realignment().flip(header.realignment().permutation(axis)) ? -1.0 : 1.0); //
    if (slice_encoding_it != header.keyval().end()) {
      slice_encoding_it->second = Metadata::BIDS::vector2axisid(new_dir);
      INFO("Slice encoding direction has been modified"
           " to conform to MRtrix3 internal header transform realignment"
           " of image \"" + header.name() + "\"");
    } else if ((new_dir * -1).dot(orig_dir) == 1) {
      auto slice_timing = parse_floats(slice_timing_it->second);
      std::reverse(slice_timing.begin(), slice_timing.end());
      slice_timing_it->second = join(slice_timing, ",");
      INFO("Slice timing vector reversed to conform to MRtrix3 internal transform realignment"
           " of image \"" + header.name() + "\"");
    } else {
      header.keyval()["SliceEncodingDirection"] = Metadata::BIDS::vector2axisid(new_dir);
      WARN("Slice encoding direction of image \"" + header.name() + "\""
           " inferred to be \"k\" in order to preserve interpretation of existing \"SliceTiming\" field"
           " during MRtrix3 internal transform realignment");
    }
  }
}

} // namespace MR::Metadata::SliceEncoding
