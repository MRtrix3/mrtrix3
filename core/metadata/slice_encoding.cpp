/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "axes.h"
#include "file/nifti_utils.h"
#include "header.h"
#include "metadata/bids.h"

namespace MR {
  namespace Metadata {
    namespace SliceEncoding {



      void transform_for_image_load(KeyValues &keyval, const Header& header) {
        // If there's any slice encoding direction information present in the
        //   header, that's also necessary to update here
        auto slice_encoding_it = keyval.find("SliceEncodingDirection");
        auto slice_timing_it = keyval.find("SliceTiming");
        if (!(slice_encoding_it == keyval.end() && slice_timing_it == keyval.end())) {
          if (header.realignment().is_identity()) {
            INFO("No transformation of slice encoding direction for load of image \"" + header.name() + "\" required");
            return;
          }
          Metadata::BIDS::axis_vector_type orig_dir = Metadata::BIDS::axis_vector_type({0, 0, 1});
          if (slice_encoding_it != keyval.end()) {
            try {
              orig_dir = Metadata::BIDS::axisid2vector(slice_encoding_it->second);
            } catch (Exception& e) {
              WARN("Unable to conform slice encoding direction to image realignment "
                   "for image \"" + header.name() + "\"; erasing");
              clear(keyval);
              return;
            }
          }
          const Metadata::BIDS::axis_vector_type new_dir = header.realignment().applied_transform() * orig_dir;
          if (slice_encoding_it != keyval.end()) {
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
            keyval["SliceEncodingDirection"] = Metadata::BIDS::vector2axisid(new_dir);
            WARN("Slice encoding direction of image \"" + header.name() + "\""
                 " inferred to be \"k\" in order to preserve interpretation of existing \"SliceTiming\" field"
                 " after MRtrix3 internal transform realignment");
          }
        }
      }



      void transform_for_nifti_write(KeyValues& keyval, const Header &H) {
        auto slice_encoding_it = keyval.find("SliceEncodingDirection");
        auto slice_timing_it = keyval.find("SliceTiming");
        if (slice_encoding_it == keyval.end() && slice_timing_it == keyval.end())
          return;

        const Axes::Shuffle shuffle = File::NIfTI::axes_on_write(H);
        if (shuffle.is_identity()) {
          INFO("No need to transform slice encoding information for NIfTI image write:"
               " image is already RAS");
          return;
        }

        const Metadata::BIDS::axis_vector_type
            orig_dir(slice_encoding_it == keyval.end() ?
                     Metadata::BIDS::axis_vector_type({0, 0, 1}) :
                     Metadata::BIDS::axisid2vector(slice_encoding_it->second));
        Metadata::BIDS::axis_vector_type new_dir;
        for (size_t axis = 0; axis != 3; ++axis)
          new_dir[axis] = shuffle.flips[axis] ? -orig_dir[shuffle.permutations[axis]] : orig_dir[shuffle.permutations[axis]];

        if (slice_encoding_it != keyval.end()) {
          slice_encoding_it->second = Metadata::BIDS::vector2axisid(new_dir);
          INFO("Slice encoding direction modified according to output NIfTI strides");
        } else if ((new_dir * -1).dot(orig_dir) == 1) {
          auto slice_timing = parse_floats(slice_timing_it->second);
          std::reverse (slice_timing.begin(), slice_timing.end());
          slice_timing_it->second = join(slice_timing, ",");
          INFO("Slice timing vector reversed to conform to output NIfTI strides");
        } else {
          keyval["SliceEncodingDirection"] = Metadata::BIDS::vector2axisid(new_dir);
          WARN("Slice encoding direction added to metadata"
               " in order to preserve interpretation of existing \"SliceTiming\" field"
               " following output NIfTI strides");
        }
      }



      std::string resolve_slice_timing (const std::string &one, const std::string &two) {
        if (one == "variable" || two == "variable")
          return "variable";
        const vector<std::string> one_split = split(one, ",");
        const vector<std::string> two_split = split(two, ",");
        if (one_split.size() != two_split.size()) {
          DEBUG("Slice timing vectors of inequal length");
          return "invalid";
        }
        // Siemens CSA reports with 2.5ms precision = 0.0025s
        // Allow slice times to vary by 1.5x this amount, but no more
        for (size_t i = 0; i != one_split.size(); ++i) {
          default_type f_one, f_two;
          try {
            f_one = to<default_type>(one_split[i]);
            f_two = to<default_type>(two_split[i]);
          } catch (Exception&) {
            DEBUG("Error converting slice timing vector to floating-point");
            return "invalid";
          }
          const default_type diff = abs(f_two - f_one);
          if (diff > 0.00375) {
            DEBUG("Supra-threshold difference of " + str(diff) + "s in slice times");
            return "variable";
          }
        }
        return one;
      }



      void clear(KeyValues &keyval) {
        auto erase = [](KeyValues& keyval, const std::string& s) {
          auto it = keyval.find(s);
          if (it != keyval.end())
            keyval.erase(it);
        };
        erase(keyval, "SliceEncodingDirection");
        erase(keyval, "SliceTiming");
      }



    }
  }
}
