/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include "phase_encoding.h"

namespace MR
{
  namespace PhaseEncoding
  {



    std::string dir2id (const Eigen::Vector3& axis)
    {
      if (axis[0] == -1) {
        assert (!axis[1]); assert (!axis[2]); return "i-";
      } else if (axis[0] == 1) {
        assert (!axis[1]); assert (!axis[2]); return "i";
      } else if (axis[1] == -1) {
        assert (!axis[0]); assert (!axis[2]); return "j-";
      } else if (axis[1] == 1) {
        assert (!axis[0]); assert (!axis[2]); return "j";
      } else if (axis[2] == -1) {
        assert (!axis[0]); assert (!axis[1]); return "k-";
      } else if (axis[2] == 1) {
        assert (!axis[0]); assert (!axis[1]); return "k";
      } else {
        throw Exception ("Malformed phase-encode direction: \"" + str(axis.transpose()) + "\"");
      }
    }
    Eigen::Vector3 id2dir (const std::string& id)
    {
      if (id == "i-")
        return { -1,  0,  0 };
      else if (id == "i")
        return {  1,  0,  0 };
      else if (id == "j-")
        return {  0, -1,  0 };
      else if (id == "j")
        return {  0,  1,  0 };
      else if (id == "k-")
        return {  0,  0, -1 };
      else if (id == "k")
        return {  0,  0,  1 };
      else
        throw Exception ("Malformed phase-encode identifier: \"" + id + "\"");
    }



    Eigen::MatrixXd get_scheme (const Header& header)
    {
      Eigen::MatrixXd PE;
      const auto it = header.keyval().find ("pe_scheme");
      if (it != header.keyval().end()) {
        const auto lines = split_lines (it->second);
        for (size_t row = 0; row < lines.size(); ++row) {
          const auto values = parse_floats (lines[row]);
          if (PE.cols() == 0)
            PE.resize (lines.size(), values.size());
          else if (PE.cols() != ssize_t (values.size()))
            throw Exception ("malformed PE scheme in image \"" + header.name() + "\" - uneven number of entries per row");
          for (size_t col = 0; col < values.size(); ++col)
            PE(row, col) = values[col];
        }
      } else {
        const auto it_dir  = header.keyval().find ("PhaseEncodingDirection");
        const auto it_time = header.keyval().find ("TotalReadoutTime");
        if (it_dir != header.keyval().end() && it_time != header.keyval().end()) {
          Eigen::Matrix<default_type, 4, 1> row;
          row.head<3>() = id2dir (it_dir->second);
          row[3] = to<default_type>(it_time->second);
          PE.resize ((header.ndim() > 3) ? header.size(3) : 1, 4);
          PE.rowwise() = row.transpose();
        }
      }
      return PE;
    }



    Eigen::MatrixXd eddy2scheme (const Eigen::MatrixXd& config, const Eigen::Array<int, Eigen::Dynamic, 1>& indices)
    {
      if (config.cols() != 4)
        throw Exception ("Expected 4 columns in EDDY-format phase-encoding config file");
      Eigen::MatrixXd result (indices.size(), 4);
      for (ssize_t row = 0; row != indices.size(); ++row) {
        if (indices[row] > config.rows())
          throw Exception ("Malformed EDDY-style phase-encoding information: Index exceeds number of config entries");
        result.row(row) = config.row(indices[row]-1);
      }
      return result;
    }



    Eigen::MatrixXd load (const std::string& path)
    {
      const Eigen::MatrixXd result = load_matrix (path);
      check (result);
      return result;
    }

    Eigen::MatrixXd load_eddy (const std::string& config_path, const std::string& index_path)
    {
      const Eigen::MatrixXd config = load_matrix (config_path);
      const Eigen::Array<int, Eigen::Dynamic, 1> indices = load_vector<int> (index_path);
      return eddy2scheme (config, indices);
    }



  }
}

