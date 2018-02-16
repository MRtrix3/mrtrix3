/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include <fstream>

#include "file/json_utils.h"
#include "file/nifti_utils.h"

#include "axes.h"
#include "exception.h"
#include "header.h"
#include "mrtrix.h"
#include "phase_encoding.h"
#include "types.h"
#include "file/ofstream.h"

namespace MR
{
  namespace File
  {
    namespace JSON
    {



      void load (Header& H, const std::string& path)
      {
        std::ifstream in (path);
        if (!in)
          throw Exception ("Error opening JSON file \"" + path + "\"");
        nlohmann::json json;
        try {
          in >> json;
        } catch (std::logic_error& e) {
          throw Exception ("Error parsing JSON file \"" + path + "\": " + e.what());
        }
        for (auto i = json.cbegin(); i != json.cend(); ++i) {

          if (i->is_boolean()) {
            H.keyval().insert (std::make_pair (i.key(), i.value() ? "1" : "0"));
          } else if (i->is_number_integer()) {
            H.keyval().insert (std::make_pair (i.key(), str<int>(i.value())));
          } else if (i->is_number_float()) {
            H.keyval().insert (std::make_pair (i.key(), str<float>(i.value())));
          } else if (i->is_array()) {
            vector<std::string> s;
            for (auto j = i->cbegin(); j != i->cend(); ++j) {
              if (j->is_array()) {
                vector<std::string> line;
                for (auto k : *j)
                  line.push_back (str(k));
                s.push_back (join(line, ","));
              } else {
                s.push_back (str(*j));
              }
            }
            H.keyval().insert (std::make_pair (i.key(), join(s, "\n")));
          } else if (i->is_string()) {
            const std::string s = i.value();
            H.keyval().insert (std::make_pair (i.key(), s));
          }

        }

        if (!Header::do_not_realign_transform) {

          // The corresponding header may have been rotated on image load prior to the JSON
          //   being loaded. If this is the case, any fields that indicate an image axis
          //   number / direction need to be correspondingly modified.

          size_t perm[3];
          bool flip[3];
          Axes::get_permutation_to_make_axial (H.transform(), perm, flip);
          if (perm[0] != 0 || perm[1] != 1 || perm[2] != 2 || flip[0] || flip[1] || flip[2]) {

            auto pe_scheme = PhaseEncoding::get_scheme (H);
            if (pe_scheme.rows()) {
              for (ssize_t row = 0; row != pe_scheme.rows(); ++row) {
                auto new_line = pe_scheme.row (row);
                for (ssize_t axis = 0; axis != 3; ++axis)
                  new_line[perm[axis]] = flip[perm[axis]] ? pe_scheme(row,axis) : -pe_scheme(row,axis);
                pe_scheme.row (row) = new_line;
              }
              PhaseEncoding::set_scheme (H, pe_scheme);
              INFO ("Phase encoding information read from JSON file modified according to expected input image header transform realignment");
            }

            auto slice_encoding_it = H.keyval().find ("SliceEncodingDirection");
            if (slice_encoding_it != H.keyval().end()) {
              const Eigen::Vector3 orig_dir (Axes::id2dir (slice_encoding_it->second));
              Eigen::Vector3 new_dir;
              for (size_t axis = 0; axis != 3; ++axis)
                new_dir[perm[axis]] = flip[perm[axis]] > 0 ? orig_dir[axis] : -orig_dir[axis];
              slice_encoding_it->second = Axes::dir2id (new_dir);
              INFO ("Slice encoding direction read from JSON file modified according to expected input image header transform realignment");
            }

          }
        }
      }



      void save (const Header& H, const std::string& path)
      {
        nlohmann::json json;
        auto pe_scheme = PhaseEncoding::get_scheme (H);
        vector<size_t> order;
        File::NIfTI::adjust_transform (H, order);
        Header H_adj (H);
        const bool axes_adjusted = (order[0] != 0 || order[1] != 1 || order[2] != 2 || H.stride(0) < 0 || H.stride(1) < 0 || H.stride(2) < 0);
        if (pe_scheme.rows() && axes_adjusted) {
          // Assume that image being written to disk is going to have its transform adjusted,
          //   so modify the phase encoding scheme appropriately before writing to JSON
          for (ssize_t row = 0; row != pe_scheme.rows(); ++row) {
            auto new_line = pe_scheme.row (row);
            for (ssize_t axis = 0; axis != 3; ++axis)
              new_line[axis] = H.stride (order[axis]) > 0 ? pe_scheme(row, order[axis]) : -pe_scheme(row, order[axis]);
            pe_scheme.row (row) = new_line;
          }
          PhaseEncoding::set_scheme (H_adj, pe_scheme);
          INFO ("Phase encoding information written to JSON file modified according to expected output NIfTI header transform realignment");
        }
        auto slice_encoding_it = H_adj.keyval().find ("SliceEncodingDirection");
        if (slice_encoding_it != H_adj.keyval().end() && axes_adjusted) {
          const Eigen::Vector3 orig_dir (Axes::id2dir (slice_encoding_it->second));
          Eigen::Vector3 new_dir;
          for (size_t axis = 0; axis != 3; ++axis)
            new_dir[order[axis]] = H.stride (order[axis]) > 0 ? orig_dir[order[axis]] : -orig_dir[order[axis]];
          slice_encoding_it->second = Axes::dir2id (new_dir);
          INFO ("Slice encoding direction written to JSON file modified according to expected output NIfTI header transform realignment");
        }
        for (const auto& kv : H_adj.keyval())
          json[kv.first] = kv.second;
        File::OFStream out (path);
        out << json.dump(4);
      }



    }
  }
}

