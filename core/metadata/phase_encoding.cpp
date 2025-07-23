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

#include "metadata/phase_encoding.h"

#include "exception.h"

namespace MR {
  namespace Metadata {
    namespace PhaseEncoding {



      using namespace App;
      const OptionGroup ImportOptions =
        OptionGroup("Options for importing phase-encode tables")
        + Option("import_pe_topup", "import a phase-encoding table intended for FSL TOPUP from file")
          + Argument("file").type_file_in()
        + Option("import_pe_eddy", "import phase-encoding information from an EDDY-style config / index file pair")
          + Argument("config").type_file_in()
          + Argument("indices").type_file_in();

      const OptionGroup SelectOptions =
        OptionGroup("Options for selecting volumes based on phase-encoding")
        + Option("pe",
                 "select volumes with a particular phase encoding;"
                 " this can be three comma-separated values"
                 " (for i,j,k components of vector direction)"
                 " or four (direction & total readout time)")
          + Argument("desc").type_sequence_float();

      const OptionGroup ExportOptions =
        OptionGroup("Options for exporting phase-encode tables")
        + Option("export_pe_topup", "export phase-encoding table to a file intended for FSL topup")
          + Argument("file").type_file_out()
        + Option("export_pe_eddy", "export phase-encoding information to an EDDY-style config / index file pair")
          + Argument("config").type_file_out()
          + Argument("indices").type_file_out();



      void check(const scheme_type& PE) {
        if (PE.rows() == 0)
          throw Exception("No valid phase encoding table found");
        if (PE.cols() < 3)
          throw Exception("Phase-encoding matrix must have at least 3 columns");
        for (ssize_t row = 0; row != PE.rows(); ++row) {
          for (ssize_t axis = 0; axis != 3; ++axis) {
            if (std::round(PE(row, axis)) != PE(row, axis))
              throw Exception("Phase-encoding matrix contains non-integral axis designation");
          }
        }
      }



      void check(const scheme_type& PE, const Header& header) {
        check(PE);
        const ssize_t num_volumes = (header.ndim() > 3) ? header.size(3) : 1;
        if (num_volumes != PE.rows())
          throw Exception("Number of volumes in image \"" + header.name() + "\" (" + str(num_volumes) + ")"
                          + " does not match that in phase encoding table (" + str(PE.rows()) + ")");
      }



      namespace {
        void erase(KeyValues& keyval, const std::string& s) {
          auto it = keyval.find(s);
          if (it != keyval.end())
            keyval.erase(it);
        };
      }



      void set_scheme(KeyValues& keyval, const scheme_type& PE) {
        if (PE.rows() == 0) {
          erase(keyval, "pe_scheme");
          erase(keyval, "PhaseEncodingDirection");
          erase(keyval, "TotalReadoutTime");
          return;
        }
        std::string pe_scheme;
        std::string first_line;
        bool variation = false;
        for (ssize_t row = 0; row < PE.rows(); ++row) {
          std::string line = str(PE(row, 0));
          for (ssize_t col = 1; col < PE.cols(); ++col)
            line += "," + str(PE(row, col), 3);
          add_line(pe_scheme, line);
          if (first_line.empty())
            first_line = line;
          else if (line != first_line)
            variation = true;
        }
        if (variation) {
          keyval["pe_scheme"] = pe_scheme;
          erase(keyval, "PhaseEncodingDirection");
          erase(keyval, "TotalReadoutTime");
        } else {
          erase(keyval, "pe_scheme");
          const Metadata::BIDS::axis_vector_type dir{int(PE(0, 0)), int(PE(0, 1)), int(PE(0, 2))};
          keyval["PhaseEncodingDirection"] = Metadata::BIDS::vector2axisid(dir);
          if (PE.cols() >= 4)
            keyval["TotalReadoutTime"] = str(PE(0, 3), 3);
          else
            erase(keyval, "TotalReadoutTime");
        }
      }



      void clear_scheme(KeyValues& keyval) {
        auto erase = [&](const std::string& s) {
          auto it = keyval.find(s);
          if (it != keyval.end())
          keyval.erase(it);
        };
        erase("pe_scheme");
        erase("PhaseEncodingDirection");
        erase("TotalReadoutTime");
      }



      scheme_type parse_scheme(const KeyValues& keyval, const Header& header) {
        scheme_type PE;
        const auto it = keyval.find("pe_scheme");
        if (it != keyval.end()) {
          try {
            PE = MR::parse_matrix(it->second);
          } catch (Exception& e) {
            throw Exception(e, "malformed PE scheme associated with image \"" + header.name() + "\"");
          }
          if (ssize_t(PE.rows()) != ((header.ndim() > 3) ? header.size(3) : 1))
            throw Exception("malformed PE scheme associated with image \"" + header.name() + "\": "
                            + "number of rows does not equal number of volumes");
        } else {
          const auto it_dir = keyval.find("PhaseEncodingDirection");
          if (it_dir != keyval.end()) {
            const auto it_time = keyval.find("TotalReadoutTime");
            const size_t cols = it_time == keyval.end() ? 3 : 4;
            Eigen::Matrix<default_type, Eigen::Dynamic, 1> row(cols);
            try {
              row.head(3) = BIDS::axisid2vector(it_dir->second).cast<default_type>();
            } catch (Exception& e) {
              throw Exception(e, "malformed phase encoding direction associated with image \"" + header.name() + "\"");
            }
            if (it_time != keyval.end()) {
              try {
                row[3] = to<default_type>(it_time->second);
              } catch (Exception& e) {
                throw Exception(e, "Error adding readout time to phase encoding table");
              }
            }
            PE.resize((header.ndim() > 3) ? header.size(3) : 1, cols);
            PE.rowwise() = row.transpose();
          }
        }
        return PE;
      }



      scheme_type get_scheme(const Header& header) {
        DEBUG("searching for suitable phase encoding data...");
        using namespace App;

        const auto opt_topup = get_options("import_pe_topup");
        const auto opt_eddy = get_options("import_pe_eddy");
        if (opt_topup.size() + opt_eddy.size() > 1)
          throw Exception("Cannot specify more than one command-line option"
                          " for importing phase encoding information from external file(s)");

        scheme_type result;
        try {
          if (!opt_topup.empty())
            result = load_topup(opt_topup[0][0], header);
          else if (!opt_eddy.empty())
            result = load_eddy(opt_eddy[0][0], opt_eddy[0][1], header);
          else
            result = parse_scheme(header.keyval(), header);
        } catch (Exception &e) {
          throw Exception(e, "error importing phase encoding table for image \"" + header.name() + "\"");
        }

        if (result.rows() == 0)
          return result;

        if (result.cols() < 3)
          throw Exception("unexpected phase encoding table matrix dimensions");

        INFO("found " + str(result.rows()) + "x" + str(result.cols()) + " phase encoding table");

        return result;
      }







      void transform_for_image_load(KeyValues& keyval, const Header& H) {
        scheme_type pe_scheme;
        try {
          pe_scheme = parse_scheme(keyval, H);
        } catch (Exception& e) {
          WARN("Unable to conform phase encoding information to image realignment "
               " for image \"" + H.name() + "\"; erasing");
          clear_scheme(keyval);
          return;
        }
        if (pe_scheme.rows() == 0) {
          DEBUG("No phase encoding information found for transformation with load of image \"" + H.name() + "\"");
          return;
        }
        if (H.realignment().is_identity()) {
          INFO("No transformation of phase encoding data for load of image \"" + H.name() + "\" required");
          return;
        }
        set_scheme(keyval, transform_for_image_load(pe_scheme, H));
        INFO("Phase encoding data transformed to match RAS realignment of image \"" + H.name() + "\"");
      }



      scheme_type transform_for_image_load(const scheme_type& pe_scheme, const Header& H) {
        if (H.realignment().is_identity())
          return pe_scheme;
        scheme_type result(pe_scheme.rows(), pe_scheme.cols());
        for (ssize_t row = 0; row != pe_scheme.rows(); ++row) {
          Eigen::VectorXd new_line = pe_scheme.row(row);
          new_line.head<3>() = (H.realignment().applied_transform() * new_line.head<3>().cast<int>()).cast<default_type>();
          result.row(row) = new_line;
        }
        return result;
      }



      void transform_for_nifti_write(KeyValues& keyval, const Header& H) {
        const scheme_type pe_scheme = parse_scheme(keyval, H);
        if (pe_scheme.rows() == 0) {
          DEBUG("No phase encoding information found for transformation with save of NIfTI image \"" + H.name() + "\"");
          return;
        }
        set_scheme(keyval, transform_for_nifti_write(pe_scheme, H));
      }



      scheme_type transform_for_nifti_write(const scheme_type& pe_scheme, const Header& H) {
        if (pe_scheme.rows() == 0)
          return pe_scheme;
        Axes::Shuffle shuffle = File::NIfTI::axes_on_write(H);
        if (shuffle.is_identity()) {
          INFO("No transformation of phase encoding data required for export to file:"
               " output image will be RAS");
          return pe_scheme;
        }
        scheme_type result(pe_scheme.rows(), pe_scheme.cols());
        for (ssize_t row = 0; row != pe_scheme.rows(); ++row) {
          Eigen::VectorXd new_line = pe_scheme.row(row);
          for (ssize_t axis = 0; axis != 3; ++axis)
            new_line[axis] =
              pe_scheme(row, shuffle.permutations[axis]) != 0.0 && shuffle.flips[axis] ?
              -pe_scheme(row, shuffle.permutations[axis]) :
              pe_scheme(row, shuffle.permutations[axis]);
          result.row(row) = new_line;
        }
        INFO("Phase encoding data transformed to match NIfTI / MGH image export prior to writing to file");
        return result;
      }



      void topup2eddy(const scheme_type& PE, Eigen::MatrixXd& config, Eigen::Array<int, Eigen::Dynamic, 1>& indices) {
        try {
          check(PE);
        } catch (Exception& e) {
          throw Exception(e, "Cannot convert phase-encoding scheme to eddy format");
        }
        if (PE.cols() != 4)
          throw Exception("Phase-encoding matrix requires 4 columns to convert to eddy format");
        config.resize(0, 0);
        indices = Eigen::Array<int, Eigen::Dynamic, 1>::Constant (PE.rows(), PE.rows());
        for (ssize_t PE_row = 0; PE_row != PE.rows(); ++PE_row) {
          for (ssize_t config_row = 0; config_row != config.rows(); ++config_row) {
            const bool dir_match = PE.template block<1, 3>(PE_row, 0).isApprox(config.block<1, 3>(config_row, 0));
            const bool time_match = abs(PE(PE_row, 3) - config(config_row, 3)) < 1e-3;
            if (dir_match && time_match) {
              // FSL-style index file indexes from 1
              indices[PE_row] = config_row + 1;
              break;
            }
          }
          if (indices[PE_row] == PE.rows()) {
            // No corresponding match found in config matrix; create a new entry
            config.conservativeResize(config.rows() + 1, 4);
            config.row(config.rows() - 1) = PE.row(PE_row);
            indices[PE_row] = config.rows();
          }
        }
      }



      scheme_type eddy2topup(const Eigen::MatrixXd& config, const Eigen::Array<int, Eigen::Dynamic, 1>& indices) {
        if (config.cols() != 4)
          throw Exception("Expected 4 columns in EDDY-format phase-encoding config file");
        scheme_type result(indices.size(), 4);
        for (ssize_t row = 0; row != indices.size(); ++row) {
          if (indices[row] > config.rows())
            throw Exception("Malformed EDDY-style phase-encoding information:"
                            " index exceeds number of config entries");
          result.row(row) = config.row(indices[row] - 1);
        }
        return result;
      }



      void export_commandline(const Header& header) {
        auto check = [&](const scheme_type& m) -> const scheme_type & {
          if (m.rows() == 0)
            throw Exception("no phase-encoding information found within image \"" + header.name() + "\"");
          return m;
        };

        auto scheme = parse_scheme(header.keyval(), header);

        auto opt = get_options("export_pe_topup");
        if (!opt.empty())
          save_topup(check(scheme), header, opt[0][0]);

        opt = get_options("export_pe_eddy");
        if (!opt.empty())
          save_eddy(check(scheme), header, opt[0][0], opt[0][1]);
      }



      scheme_type load_topup(const std::string& path, const Header& header) {
        scheme_type PE = load_matrix(path);
        check(PE, header);
        // Flip of first image axis based on determinant of image transform
        //   applies to however the image was stored on disk,
        //   before any interpretation by MRtrix3
        if (header.realignment().orig_transform().linear().determinant() > 0.0)
          PE.col(0) *= -1;
        return transform_for_image_load(PE, header);
      }



      scheme_type load_eddy(const std::string& config_path, const std::string& index_path, const Header& header) {
        const Eigen::MatrixXd config = load_matrix(config_path);
        const Eigen::Array<int, Eigen::Dynamic, 1> indices = load_vector<int>(index_path);
        scheme_type PE = eddy2topup(config, indices);
        check(PE, header);
        if (header.realignment().orig_transform().linear().determinant() > 0.0)
          PE.col(0) *= -1;
        return transform_for_image_load(PE, header);
      }



      void save_table(const scheme_type& PE, const std::string& path, const bool write_command_history) {
        File::OFStream out(path);
        if (write_command_history)
          out << "# " << App::command_history_string << "\n";
        for (ssize_t row = 0; row != PE.rows(); ++row) {
          // Write phase-encode direction as integers; other information as floating-point
          out << PE.template block<1, 3>(row, 0).template cast<int>();
          if (PE.cols() > 3)
            out << " " << PE.block(row, 3, 1, PE.cols() - 3);
          out << "\n";
        }
      }



    }
  }
}
