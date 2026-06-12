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

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "transform.h"
#include "types.h"

#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/formats/write_buffer.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"

namespace MR::File {
class MMap;
}

namespace MR::DWI::Tractography::Formats::TRKUtils {

//! the fixed size in bytes of the TrackVis ".trk" header (spec: 1000 bytes)
constexpr size_t header_bytes = 1000;
//! the value the \c hdr_size field must read as; used as the byte-swap sentinel
constexpr int32_t header_size_sentinel = 1000;
//! the maximum number of scalars (dpv) / properties (dps) the header can name
constexpr size_t max_named_fields = 10;
//! the maximum length (chars) of a scalar / property name in the header
constexpr size_t name_length = 20;

//! \brief Byte-packed view of the fixed-size 1000-byte TrackVis ".trk" header.
/*! The member layout and byte offsets are taken verbatim from the TrackVis
 * format specification. The struct is packed (no padding) and built from
 * fixed-width types (wrapped in std::array, which is layout-compatible with the
 * underlying C array) so that its in-memory image matches the on-disk byte
 * layout exactly (verified by a static_assert on sizeof == 1000). Multi-byte
 * fields are stored little-endian on disk; the reader detects the opposite-endian
 * case via the \c hdr_size sentinel (must read as 1000) and byte-swaps every
 * multi-byte field on ingest. */
#pragma pack(push, 1)
struct Header {
  std::array<char, 6> id_string;                                             //!< first 5 chars are "TRACK"
  std::array<int16_t, 3> dim;                                                //!< image volume dimensions
  std::array<float, 3> voxel_size;                                           //!< voxel spacing (mm)
  std::array<float, 3> origin;                                               //!< origin (unused; always 0)
  int16_t n_scalars;                                                         //!< per-vertex scalars (→ dpv)
  std::array<std::array<char, name_length>, max_named_fields> scalar_name;   //!< scalar field names
  int16_t n_properties;                                                      //!< per-streamline props (→ dps)
  std::array<std::array<char, name_length>, max_named_fields> property_name; //!< property field names
  std::array<std::array<float, 4>, 4> vox_to_ras;                            //!< voxel→RAS affine
  std::array<char, 444> reserved;                                            //!< reserved
  std::array<char, 4> voxel_order;                                           //!< storage order of the image
  std::array<char, 4> pad2;                                                  //!< padding
  std::array<float, 6> image_orientation_patient;                            //!< DICOM image orientation
  std::array<char, 2> pad1;                                                  //!< padding
  uint8_t invert_x;                                                          //!< flags (internal use)
  uint8_t invert_y;
  uint8_t invert_z;
  uint8_t swap_xy;
  uint8_t swap_yz;
  uint8_t swap_zx;
  int32_t n_count;  //!< number of tracks (0 ⇒ unset)
  int32_t version;  //!< format version (current is 2)
  int32_t hdr_size; //!< header size; must be 1000
};
#pragma pack(pop)

static_assert(sizeof(Header) == header_bytes, "TrackVis \".trk\" header must be exactly 1000 bytes");

} // namespace MR::DWI::Tractography::Formats::TRKUtils
// Reader/writer backends and the Formats::TRK handler are added in subsequent
//   Stage 14 steps (write, read, command-string provenance, capability flags).
