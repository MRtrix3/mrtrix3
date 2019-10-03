/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#include <fstream>
#include <sstream>

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
        read (json, H, true);
      }



      void save (const Header& H, const std::string& path)
      {
        nlohmann::json json;
        write (H, json, true);
        File::OFStream out (path);
        out << json.dump(4);
      }





      KeyValues read (const nlohmann::json& json)
      {
        KeyValues result;
        for (auto i = json.cbegin(); i != json.cend(); ++i) {

          if (i->is_boolean()) {
            result.insert (std::make_pair (i.key(), i.value() ? "1" : "0"));
          } else if (i->is_number_integer()) {
            result.insert (std::make_pair (i.key(), str<int>(i.value())));
          } else if (i->is_number_float()) {
            result.insert (std::make_pair (i.key(), str<float>(i.value())));
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
            result.insert (std::make_pair (i.key(), join(s, "\n")));
          } else if (i->is_string()) {
            const std::string s = i.value();
            result.insert (std::make_pair (i.key(), s));
          }
        }
        return result;
      }



      void read (const nlohmann::json& json, Header& header, const bool realign)
      {
        header.keyval() = read (json);
        if (realign && !Header::do_not_realign_transform) {

          // The corresponding header may have been rotated on image load prior to the JSON
          //   being loaded. If this is the case, any fields that indicate an image axis
          //   number / direction need to be correspondingly modified.

          size_t perm[3];
          bool flip[3];
          Axes::get_permutation_to_make_axial (header.transform(), perm, flip);
          if (perm[0] != 0 || perm[1] != 1 || perm[2] != 2 || flip[0] || flip[1] || flip[2]) {

            auto pe_scheme = PhaseEncoding::get_scheme (header);
            if (pe_scheme.rows()) {
              for (ssize_t row = 0; row != pe_scheme.rows(); ++row) {
                Eigen::VectorXd new_line = pe_scheme.row (row);
                for (ssize_t axis = 0; axis != 3; ++axis)
                  new_line[perm[axis]] = flip[perm[axis]] ? pe_scheme(row,axis) : -pe_scheme(row,axis);
                pe_scheme.row (row) = new_line;
              }
              PhaseEncoding::set_scheme (header, pe_scheme);
              INFO ("Phase encoding information read from JSON file modified according to expected input image header transform realignment");
            }

            auto slice_encoding_it = header.keyval().find ("SliceEncodingDirection");
            if (slice_encoding_it != header.keyval().end()) {
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



      namespace {
        template <typename T>
        bool attempt_scalar (const std::pair<std::string, std::string>& kv, nlohmann::json& json)
        {
          try {
            std::stringstream stream (kv.second);
            T temp;
            stream >> temp;
            if (stream && stream.eof()) {
              json[kv.first] = temp;
              return true;
            }
          } catch (...) { }
          return false;
        }
        bool attempt_matrix (const std::pair<std::string, std::string>& kv, nlohmann::json& json)
        {
          try {
            auto M_float = parse_matrix<default_type> (kv.second);
            if (M_float.cols() == 1)
              M_float.transposeInPlace();
            nlohmann::json temp;
            bool noninteger = false;
            for (ssize_t row = 0; row != M_float.rows(); ++row) {
              vector<default_type> data (M_float.cols());
              for (ssize_t i = 0; i != M_float.cols(); ++i) {
                data[i] = M_float (row, i);
                if (std::floor (data[i]) != data[i])
                  noninteger = true;
              }
              if (row)
                temp[kv.first].push_back (data);
              else if (M_float.rows() == 1)
                temp[kv.first] = data;
              else
                temp[kv.first] = { data };
            }
            if (noninteger) {
              json[kv.first] = temp[kv.first];
            } else {
              // No non-integer values found;
              // Write the data natively as integers
              auto M_int = parse_matrix<int> (kv.second);
              if (M_int.cols() == 1)
                M_int.transposeInPlace();
              temp[kv.first] = nlohmann::json({});
              for (ssize_t row = 0; row != M_int.rows(); ++row) {
                vector<int> data (M_int.cols());
                for (ssize_t i = 0; i != M_int.cols(); ++i)
                  data[i] = M_int (row, i);
                if (row)
                  temp[kv.first].push_back (data);
                else if (M_int.rows() == 1)
                  temp[kv.first] = data;
                else
                  temp[kv.first] = { data };
              }
              json[kv.first] = temp[kv.first];
            }
            return true;
          } catch (...) { return false; }
        }
      }


      void write (const KeyValues& keyval, nlohmann::json& json)
      {
        auto write_string = [] (const std::pair<std::string, std::string>& kv,
                                nlohmann::json& json)
        {
          const auto lines = split_lines (kv.second);
          if (lines.size() > 1)
            json[kv.first] = lines;
          else
            json[kv.first] = kv.second;
        };

        for (const auto& kv : keyval) {

          if (attempt_scalar<int> (kv, json)) continue;
          if (attempt_scalar<default_type> (kv, json)) continue;
          if (attempt_scalar<bool> (kv, json)) continue;
          if (attempt_matrix (kv, json)) continue;
          if (json.find (kv.first) == json.end()) {
            write_string (kv, json);
          } else {
            nlohmann::json temp;
            write_string (kv, temp);
            if (json[kv.first] != temp[kv.first])
              json[kv.first] = "variable";
          }
        }
      }



      void write (const Header& header, nlohmann::json& json, const bool realign)
      {
        if (realign && !Header::do_not_realign_transform) {
          auto pe_scheme = PhaseEncoding::get_scheme (header);
          vector<size_t> order;
          File::NIfTI::adjust_transform (header, order);
          Header H_adj (header);
          const bool axes_adjusted = (order[0] != 0 || order[1] != 1 || order[2] != 2 || header.stride(0) < 0 || header.stride(1) < 0 || header.stride(2) < 0);
          if (pe_scheme.rows() && axes_adjusted) {
            // Assume that image being written to disk is going to have its transform adjusted,
            //   so modify the phase encoding scheme appropriately before writing to JSON
            for (ssize_t row = 0; row != pe_scheme.rows(); ++row) {
              Eigen::VectorXd new_line = pe_scheme.row (row);
              for (ssize_t axis = 0; axis != 3; ++axis)
                new_line[axis] = header.stride (order[axis]) > 0 ? pe_scheme(row, order[axis]) : -pe_scheme(row, order[axis]);
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
              new_dir[order[axis]] = header.stride (order[axis]) > 0 ? orig_dir[order[axis]] : -orig_dir[order[axis]];
            slice_encoding_it->second = Axes::dir2id (new_dir);
            INFO ("Slice encoding direction written to JSON file modified according to expected output NIfTI header transform realignment");
          }
          write (H_adj.keyval(), json);
        } else {
          write (header.keyval(), json);
        }
      }







    }
  }
}

