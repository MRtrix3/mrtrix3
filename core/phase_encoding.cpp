/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#include "phase_encoding.h"

#include "math/math.h"

namespace MR
{
  namespace PhaseEncoding
  {



    using namespace App;
    const OptionGroup ImportOptions = OptionGroup ("Options for importing phase-encode tables")
    + Option ("import_pe_table", "import a phase-encoding table from file")
      + Argument ("file").type_file_in()
    + Option ("import_pe_eddy", "import phase-encoding information from an EDDY-style config / index file pair")
      + Argument ("config").type_file_in()
      + Argument ("indices").type_file_in();

    const OptionGroup SelectOptions = OptionGroup ("Options for selecting volumes based on phase-encoding")
    + Option ("pe", "select volumes with a particular phase encoding; "
                    "this can be three comma-separated values (for i,j,k components of vector direction) or four (direction & total readout time)")
      + Argument ("desc").type_sequence_float();

    const OptionGroup ExportOptions = OptionGroup ("Options for exporting phase-encode tables")
    + Option ("export_pe_table", "export phase-encoding table to file")
      + Argument ("file").type_file_out()
    + Option ("export_pe_eddy", "export phase-encoding information to an EDDY-style config / index file pair")
      + Argument ("config").type_file_out()
      + Argument ("indices").type_file_out();



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



    Eigen::MatrixXd parse_scheme (const Header& header)
    {
      Eigen::MatrixXd PE;
      const auto it = header.keyval().find ("pe_scheme");
      if (it != header.keyval().end()) {
        try {
          PE = parse_matrix (it->second);
        } catch (Exception& e) {
          throw Exception (e, "malformed PE scheme in image \"" + header.name() + "\"");
        }
        if (ssize_t(PE.rows()) != ((header.ndim() > 3) ? header.size(3) : 1))
          throw Exception ("malformed PE scheme in image \"" + header.name() + "\" - number of rows does not equal number of volumes");
      } else {
        // Header entries are cast to lowercase at some point
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



    Eigen::MatrixXd get_scheme (const Header& header)
    {
      DEBUG ("searching for suitable phase encoding data...");
      using namespace App;
      Eigen::MatrixXd result;

      try {
        const auto opt_table = get_options ("import_pe_table");
        if (opt_table.size())
          result = load (opt_table[0][0]);
        const auto opt_eddy = get_options ("import_pe_eddy");
        if (opt_eddy.size()) {
          if (opt_table.size())
            throw Exception ("Please provide phase encoding table using either -import_pe_table or -import_pe_eddy option (not both)");
          result = load_eddy (opt_eddy[0][0], opt_eddy[0][1]);
        }
        if (!opt_table.size() && !opt_eddy.size())
          result = parse_scheme (header);
      }
      catch (Exception& e) {
        throw Exception (e, "error importing phase encoding table for image \"" + header.name() + "\"");
      }

      if (!result.rows())
        return result;

      if (result.cols() < 4)
        throw Exception ("unexpected phase encoding table matrix dimensions");

      INFO ("found " + str (result.rows()) + "x" + str (result.cols()) + " phase encoding table");

      return result;
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



    void export_commandline (const Header& header)
    {
      auto check = [&](const Eigen::MatrixXd& m) -> const Eigen::MatrixXd& {
        if (!m.rows())
          throw Exception ("no phase-encoding information found within image \"" + header.name() + "\"");
        return m;
      };

      auto scheme = parse_scheme (header);

      auto opt = get_options ("export_pe_table");
      if (opt.size())
        save (check (scheme), opt[0][0]);

      opt = get_options ("export_pe_eddy");
      if (opt.size())
        save_eddy (check (scheme), opt[0][0], opt[0][1]);
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

