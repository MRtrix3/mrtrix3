/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include "command.h"
#include "image.h"
#include "phase_encoding.h"
#include "progressbar.h"
#include "dwi/gradient.h"
#include "algo/loop.h"
#include "adapter/extract.h"


using namespace MR;
using namespace App;

using value_type = float;

void usage ()
{

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au) and Thijs Dhollander (thijs.dhollander@gmail.com) and Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Extract diffusion-weighted volumes, b=0 volumes, or certain shells from a DWI dataset";

  EXAMPLES
    + Example ("Calculate the mean b=0 image from a 4D DWI series",
               "dwiextract dwi.mif - -bzero | mrmath - mean mean_bzero.mif -axis 3",
               "The dwiextract command extracts all volumes for which the b-value is "
               "(approximately) zero; the resulting 4D image can then be provided to "
               "the mrmath command to calculate the mean intensity across volumes "
               "for each voxel.");

  ARGUMENTS
    + Argument ("input", "the input DW image.").type_image_in ()
    + Argument ("output", "the output image (diffusion-weighted volumes by default).").type_image_out ();

  OPTIONS
    + Option ("bzero", "Output b=0 volumes (instead of the diffusion weighted volumes, if -singleshell is not specified).")
    + Option ("no_bzero", "Output only non b=0 volumes (default, if -singleshell is not specified).")
    + Option ("singleshell", "Force a single-shell (single non b=0 shell) output. This will include b=0 volumes, if present. Use with -bzero to enforce presence of b=0 volumes (error if not present) or with -no_bzero to exclude them.")
    + DWI::GradImportOptions()
    + DWI::ShellsOption
    + DWI::GradExportOptions()
    + PhaseEncoding::ImportOptions
    + PhaseEncoding::SelectOptions
    + Stride::Options;
}






void run()
{
  auto input_image = Image<float>::open (argument[0]);
  if (input_image.ndim() < 4)
    throw Exception ("Epected input image to contain more than three dimensions");
  auto grad = DWI::get_DW_scheme (input_image);

  // Want to support non-shell-like data if it's just a straight extraction
  //   of all dwis or all bzeros i.e. don't initialise the Shells class
  vector<uint32_t> volumes;
  bool bzero = get_options ("bzero").size();
  if (get_options ("shells").size() || get_options ("singleshell").size()) {
    DWI::Shells shells (grad);
    shells.select_shells (get_options ("singleshell").size(),get_options ("bzero").size(),get_options ("no_bzero").size());
    for (size_t s = 0; s != shells.count(); ++s) {
      DEBUG ("Including data from shell b=" + str(shells[s].get_mean()) + " +- " + str(shells[s].get_stdev()));
      for (const auto v : shells[s].get_volumes())
        volumes.push_back (v);
    }
    bzero = (shells.count() == 1 && shells.has_bzero());
  // If no command-line options specified, then just grab all non-b=0 volumes
  // If however we are selecting volumes according to phase-encoding, and
  //   shells have not been explicitly selected, do NOT filter by b-value here
  } else if (!get_options ("pe").size()) {
    const float bzero_threshold = File::Config::get_float ("BZeroThreshold", 10.0);
    for (ssize_t row = 0; row != grad.rows(); ++row) {
      if ((bzero && (grad (row, 3) < bzero_threshold)) || (!bzero && (grad (row, 3) > bzero_threshold)))
        volumes.push_back (row);
    }
  } else {
    // "pe" option has been provided - need to initialise list of volumes
    //   to include all voxels, as the PE selection filters from this
    for (uint32_t i = 0; i != grad.rows(); ++i)
      volumes.push_back (i);
  }

  auto opt = get_options ("pe");
  const auto pe_scheme = PhaseEncoding::get_scheme (input_image);
  if (opt.size()) {
    if (!pe_scheme.rows())
      throw Exception ("Cannot filter volumes by phase-encoding: No such information present");
    const auto filter = parse_floats (opt[0][0]);
    if (!(filter.size() == 3 || filter.size() == 4))
      throw Exception ("Phase encoding filter must be a comma-separated list of either 3 or 4 numbers");
    vector<uint32_t> new_volumes;
    for (const auto i : volumes) {
      bool keep = true;
      for (size_t axis = 0; axis != 3; ++axis) {
        if (pe_scheme(i, axis) != filter[axis]) {
          keep = false;
          break;
        }
      }
      if (filter.size() == 4) {
        if (abs (pe_scheme(i, 3) - filter[3]) > 5e-3)
          keep = false;
      }
      if (keep)
        new_volumes.push_back (i);
    }
    std::swap (volumes, new_volumes);
  }

  if (volumes.empty()) {
    auto type = (bzero) ? "b=0" : "dwi";
    throw Exception ("No " + str(type) + " volumes present");
  }

  std::sort (volumes.begin(), volumes.end());

  Header header (input_image);
  Stride::set_from_command_line (header);
  header.size (3) = volumes.size();

  Eigen::MatrixXd new_grad (volumes.size(), grad.cols());
  for (size_t i = 0; i < volumes.size(); i++)
    new_grad.row (i) = grad.row (volumes[i]);
  DWI::set_DW_scheme (header, new_grad);

  if (pe_scheme.rows()) {
    Eigen::MatrixXd new_scheme (volumes.size(), pe_scheme.cols());
    for (size_t i = 0; i != volumes.size(); ++i)
      new_scheme.row(i) = pe_scheme.row (volumes[i]);
    PhaseEncoding::set_scheme (header, new_scheme);
  }

  auto output_image = Image<float>::create (argument[1], header);
  DWI::export_grad_commandline (header);

  auto input_volumes = Adapter::make<Adapter::Extract1D> (input_image, 3, volumes);
  threaded_copy_with_progress_message ("extracting volumes", input_volumes, output_image);
}
