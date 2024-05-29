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

#include "metadata/phase_encoding.h"

namespace MR::Metadata::PhaseEncoding {

// clang-format off
using namespace App;
const OptionGroup ImportOptions =
    OptionGroup("Options for importing phase-encode tables")
    + Option("import_pe_table", "import a phase-encoding table from file")
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
    + Option("export_pe_table", "export phase-encoding table to file")
      + Argument("file").type_file_out()
    + Option("export_pe_eddy", "export phase-encoding information to an EDDY-style config / index file pair")
      + Argument("config").type_file_out()
      + Argument("indices").type_file_out();
// clang-format on

void clear_scheme(KeyValues &keyval) {
  auto erase = [&](const std::string &s) {
    auto it = keyval.find(s);
    if (it != keyval.end())
      keyval.erase(it);
  };
  erase("pe_scheme");
  erase("PhaseEncodingDirection");
  erase("TotalReadoutTime");
}

scheme_type parse_scheme(const KeyValues &keyval, const Header &header) {
  scheme_type PE;
  const auto it = keyval.find("pe_scheme");
  if (it != keyval.end()) {
    try {
      PE = MR::parse_matrix(it->second);
    } catch (Exception &e) {
      throw Exception(e, "malformed PE scheme associated with image \"" + header.name() + "\"");
    }
    if (ssize_t(PE.rows()) != ((header.ndim() > 3) ? header.size(3) : 1))
      throw Exception("malformed PE scheme associated with image \"" + header.name() + "\":" + //
                      " number of rows does not equal number of volumes");
  } else {
    const auto it_dir = keyval.find("PhaseEncodingDirection");
    if (it_dir != keyval.end()) {
      const auto it_time = keyval.find("TotalReadoutTime");
      const size_t cols = it_time == keyval.end() ? 3 : 4;
      Eigen::Matrix<default_type, Eigen::Dynamic, 1> row(cols);
      row.head(3) = BIDS::axisid2vector(it_dir->second).cast<default_type>();
      if (it_time != keyval.end())
        row[3] = to<default_type>(it_time->second);
      PE.resize((header.ndim() > 3) ? header.size(3) : 1, cols);
      PE.rowwise() = row.transpose();
    }
  }
  return PE;
}

scheme_type get_scheme(const Header &header) {
  DEBUG("searching for suitable phase encoding data...");
  using namespace App;
  scheme_type result;

  try {
    const auto opt_table = get_options("import_pe_table");
    if (!opt_table.empty())
      result = load(opt_table[0][0], header);
    const auto opt_eddy = get_options("import_pe_eddy");
    if (!opt_eddy.empty()) {
      if (!opt_table.empty())
        throw Exception("Phase encoding table can be provided"
                        " using either -import_pe_table or -import_pe_eddy option,"
                        " but NOT both");
      result = load_eddy(opt_eddy[0][0], opt_eddy[0][1], header);
    }
    if (opt_table.empty() && opt_eddy.empty())
      result = parse_scheme(header.keyval(), header);
  } catch (Exception &e) {
    throw Exception(e, "error importing phase encoding table for image \"" + header.name() + "\"");
  }

  if (!result.rows())
    return result;

  if (result.cols() < 3)
    throw Exception("unexpected phase encoding table matrix dimensions");

  INFO("found " + str(result.rows()) + "x" + str(result.cols()) + " phase encoding table");

  return result;
}

void transform_for_image_load(KeyValues &keyval, const Header &H) {
  scheme_type pe_scheme = parse_scheme(keyval, H);
  if (!pe_scheme.rows()) {
    DEBUG("No phase encoding information found for transformation with load of image \"" + H.name() + "\"");
    return;
  }
  if (!H.realignment()) {
    INFO("No transformation of phase encoding data for load of image \"" + H.name() + "\" required");
    return;
  }
  set_scheme(keyval, transform_for_image_load(pe_scheme, H));
  INFO("Phase encoding data transformed to match RAS realignment of image \"" + H.name() + "\"");
}

scheme_type transform_for_image_load(const scheme_type &pe_scheme, const Header &H) {
  if (!H.realignment())
    return pe_scheme;
  scheme_type result(pe_scheme.rows(), pe_scheme.cols());
  for (ssize_t row = 0; row != pe_scheme.rows(); ++row) {
    Eigen::VectorXd new_line = pe_scheme.row(row);
    for (ssize_t axis = 0; axis != 3; ++axis) {
      new_line[axis] = pe_scheme(row, H.realignment().permutation(axis));
      if (new_line[axis] && H.realignment().flip(H.realignment().permutation(axis)))
        new_line[axis] = -new_line[axis];
    }
    result.row(row) = new_line;
  }
  return result;
}

void transform_for_nifti_write(KeyValues &keyval, const Header &H) {
  scheme_type pe_scheme = parse_scheme(keyval, H);
  if (!pe_scheme.rows()) {
    DEBUG("No phase encoding information found for transformation with save of NIfTI image \"" + H.name() + "\"");
    return;
  }
  set_scheme(keyval, transform_for_nifti_write(pe_scheme, H));
}

scheme_type transform_for_nifti_write(const scheme_type &pe_scheme, const Header &H) {
  if (!pe_scheme.rows())
    return pe_scheme;
  Axes::Shuffle shuffle = File::NIfTI::axes_on_write(H);
  if (!shuffle) {
    INFO("No transformation of phase encoding data required for export to file:"
         " output image will be RAS");
    return pe_scheme;
  }
  scheme_type result(pe_scheme.rows(), pe_scheme.cols());
  for (ssize_t row = 0; row != pe_scheme.rows(); ++row) {
    Eigen::VectorXd new_line = pe_scheme.row(row);
    for (ssize_t axis = 0; axis != 3; ++axis)
      new_line[axis] =                                                      //
          pe_scheme(row, shuffle.permutations[axis]) && shuffle.flips[axis] //
          ? -pe_scheme(row, shuffle.permutations[axis])                     //
          : pe_scheme(row, shuffle.permutations[axis]);                     //
    result.row(row) = new_line;
  }
  INFO("Phase encoding data transformed to match NIfTI / MGH image export prior to writing to file");
  return result;
}

void scheme2eddy(const scheme_type &PE, Eigen::MatrixXd &config, Eigen::Array<int, Eigen::Dynamic, 1> &indices) {
  try {
    check(PE);
  } catch (Exception &e) {
    throw Exception(e, "Cannot convert phase-encoding scheme to eddy format");
  }
  if (PE.cols() != 4)
    throw Exception("Phase-encoding matrix requires 4 columns to convert to eddy format");
  config.resize(0, 0);
  indices = Eigen::Array<int, Eigen::Dynamic, 1>::Constant(PE.rows(), PE.rows());
  for (ssize_t PE_row = 0; PE_row != PE.rows(); ++PE_row) {
    for (ssize_t config_row = 0; config_row != config.rows(); ++config_row) {
      bool dir_match = PE.template block<1, 3>(PE_row, 0).isApprox(config.block<1, 3>(config_row, 0));
      bool time_match = abs(PE(PE_row, 3) - config(config_row, 3)) < 1e-3;
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

scheme_type eddy2scheme(const Eigen::MatrixXd &config, const Eigen::Array<int, Eigen::Dynamic, 1> &indices) {
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

void export_commandline(const Header &header) {
  auto check = [&](const scheme_type &m) -> const scheme_type & {
    if (!m.rows())
      throw Exception("no phase-encoding information found within image \"" + header.name() + "\"");
    return m;
  };

  auto scheme = parse_scheme(header.keyval(), header);

  auto opt = get_options("export_pe_table");
  if (!opt.empty())
    save(check(scheme), header, opt[0][0]);

  opt = get_options("export_pe_eddy");
  if (!opt.empty())
    save_eddy(check(scheme), header, opt[0][0], opt[0][1]);
}

scheme_type load(const std::string &path, const Header &header) {
  const scheme_type PE = File::Matrix::load_matrix(path);
  check(PE, header);
  // As with JSON import, need to query the header to discover if the
  //   strides / transform were modified on image load to make the image
  //   data appear approximately axial, in which case we need to apply the
  //   same transforms to the phase encoding data on load
  return transform_for_image_load(PE, header);
}

} // namespace MR::Metadata::PhaseEncoding
