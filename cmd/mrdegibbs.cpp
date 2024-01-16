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

#include <numeric>

#include "command.h"
#include "degibbs/unring2d.h"
#include "degibbs/unring3d.h"

using namespace MR;
using namespace App;

const char *modes[] = {"2d", "3d", nullptr};

void usage() {
  AUTHOR = "Ben Jeurissen (ben.jeurissen@uantwerpen.be) & J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Remove Gibbs Ringing Artifacts";

  DESCRIPTION
  +"This application attempts to remove Gibbs ringing artefacts from MRI images using the method "
   "of local subvoxel-shifts proposed by Kellner et al. (see reference below for details). By default, "
   "the original 2D slice-wise version is used. If the -mode 3d option is provided, the program will run "
   "the 3D version as proposed by Bautista et al. (also in the reference list below)."

      + "This command is designed to run on data directly after it has been reconstructed by the scanner, "
        "before any interpolation of any kind has taken place. You should not run this command after any "
        "form of motion correction (e.g. not after dwifslpreproc). Similarly, if you intend running dwidenoise, "
        "you should run denoising before this command to not alter the noise structure, "
        "which would impact on dwidenoise's performance."

      + "Note that this method is designed to work on images acquired with full k-space coverage. "
        "Running this method on partial Fourier ('half-scan') or filtered data may not remove all ringing "
        "artefacts. Users are encouraged to acquired full-Fourier data where possible, and disable any "
        "form of filtering on the scanner.";

  ARGUMENTS
  +Argument("in", "the input image.").type_image_in() + Argument("out", "the output image.").type_image_out();

  OPTIONS
  +Option("mode",
          "specify the mode of operation. Valid choices are: 2d, 3d (default: "
          "2d). The 2d mode corresponds to the original slice-wise approach as "
          "propoosed by Kellner et al., appropriate for images acquired using "
          "2D muli-slice approaches. The 3d mode corresponds to the 3D "
          "volume-wise extension proposed by Bautista et al., which is "
          "appropriate for images acquired using 3D Fourier encoding.") +
      Argument("type").type_choice(modes)

      + Option("axes",
               "select the slice axes (default: 0,1 - i.e. x-y). Select all 3 spatial axes for 3D operation, "
               "i.e. 0:2 or 0,1,2 (this is equivalent to '-mode 3d').") +
      Argument("list").type_sequence_int()

      + Option("nshifts", "discretization of subpixel spacing (default: 20).") + Argument("value").type_integer(8, 128)

      + Option("minW", "left border of window used for TV computation (default: 1).") +
      Argument("value").type_integer(0, 10)

      + Option("maxW", "right border of window used for TV computation (default: 3).") +
      Argument("value").type_integer(0, 128)

      + DataType::options();

  REFERENCES
  +"Kellner, E; Dhital, B; Kiselev, V.G & Reisert, M. "
   "Gibbs-ringing artifact removal based on local subvoxel-shifts. "
   "Magnetic Resonance in Medicine, 2016, 76, 1574–1581."

      + "Bautista, T; O’Muircheartaigh, J; Hajnal, JV; & Tournier, J-D. "
        "Removal of Gibbs ringing artefacts for 3D acquisitions using subvoxel shifts. "
        "Proc. ISMRM, 2021, 29, 3535.";
}

void run() {
  const int nshifts = App::get_option_value("nshifts", 20);
  const int minW = App::get_option_value("minW", 1);
  const int maxW = App::get_option_value("maxW", 3);

  if (minW >= maxW)
    throw Exception("minW must be smaller than maxW");

  auto header = Header::open(argument[0]);
  auto in = header.get_image<Degibbs::value_type>();

  header.datatype() =
      DataType::from_command_line(header.datatype().is_complex() ? DataType::CFloat32 : DataType::Float32);
  auto out = Image<Degibbs::value_type>::create(argument[1], header);

  int mode = get_option_value("mode", 0);

  std::vector<size_t> slice_axes = {0, 1};
  auto opt = get_options("axes");
  const bool axes_set_manually = opt.size();
  if (opt.size()) {
    std::vector<uint32_t> axes = parse_ints<uint32_t>(opt[0][0]);
    if (axes == std::vector<uint32_t>({0, 1, 2})) {
      mode = 1;
    } else {
      if (axes.size() != 2)
        throw Exception("slice axes must be specified as a comma-separated 2-vector");
      if (size_t(std::max(axes[0], axes[1])) >= header.ndim())
        throw Exception("slice axes must be within the dimensionality of the image");
      if (axes[0] == axes[1])
        throw Exception("two independent slice axes must be specified");
      slice_axes = {size_t(axes[0]), size_t(axes[1])};
    }
  }

  auto slice_encoding_it = header.keyval().find("SliceEncodingDirection");
  if (slice_encoding_it != header.keyval().end()) {
    if (mode == 1) {
      WARN("running 3D volume-wise unringing, but image header contains \"SliceEncodingDirection\" field");
      WARN("If data were acquired using multi-slice encoding, run in default 2D mode.");
    } else {
      try {
        const Eigen::Vector3d slice_encoding_axis_onehot = Axes::id2dir(slice_encoding_it->second);
        std::vector<size_t> auto_slice_axes = {0, 0};
        if (slice_encoding_axis_onehot[0])
          auto_slice_axes = {1, 2};
        else if (slice_encoding_axis_onehot[1])
          auto_slice_axes = {0, 2};
        else if (slice_encoding_axis_onehot[2])
          auto_slice_axes = {0, 1};
        else
          throw Exception("Fatal error: Invalid slice axis one-hot encoding [ " +
                          str(slice_encoding_axis_onehot.transpose()) + " ]");
        if (axes_set_manually) {
          if (slice_axes == auto_slice_axes) {
            INFO("User's manual selection of within-slice axes consistent with \"SliceEncodingDirection\" field in "
                 "image header");
          } else {
            WARN("Within-slice axes set using -axes option will be used, but is inconsistent with "
                 "SliceEncodingDirection field present in image header (" +
                 slice_encoding_it->second + ")");
          }
        } else {
          if (slice_axes == auto_slice_axes) {
            INFO("\"SliceEncodingDirection\" field in image header is consistent with default selection of first two "
                 "axes as being within-slice");
          } else {
            slice_axes = auto_slice_axes;
            CONSOLE("Using axes { " + str(slice_axes[0]) + ", " + str(slice_axes[1]) +
                    " } for Gibbs ringing removal based on \"SliceEncodingDirection\" field in image header");
          }
        }
      } catch (...) {
        WARN("Invalid value for field \"SliceEncodingDirection\" in image header (" + slice_encoding_it->second +
             "); ignoring");
      }
    }
  }

  if (mode == 1) {
    Degibbs::unring3D(in, out, minW, maxW, nshifts);
    return;
  }

  // build vector of outer axes:
  std::vector<size_t> outer_axes(header.ndim());
  std::iota(outer_axes.begin(), outer_axes.end(), 0);
  for (const auto axis : slice_axes) {
    auto it = std::find(outer_axes.begin(), outer_axes.end(), axis);
    if (it == outer_axes.end())
      throw Exception("slice axis out of range!");
    outer_axes.erase(it);
  }

  ThreadedLoop("performing 2D Gibbs ringing removal", in, outer_axes, slice_axes)
      .run_outer(Degibbs::Unring2DFunctor(outer_axes, slice_axes, nshifts, minW, maxW, in, out));
}
