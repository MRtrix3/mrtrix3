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

#include <algorithm>
#include <numeric>

#include "command.h"
#include "degibbs/degibbs.h"
#include "degibbs/operators2d.h"
#include "degibbs/tgv2d.h"
#include "degibbs/unring2d.h"
#include "degibbs/unring3d.h"
#include "metadata/bids.h"

using namespace MR;
using namespace App;

using MR::Degibbs::complex_type;
using MR::Degibbs::real_type;

namespace {

enum class Method { Kellner, Bautista, TGV };

const std::vector<std::string> method_choices{"kellner", "bautista", "tgv"};
const std::vector<std::string> window_choices{"rect", "tukey", "hamming"};

} // namespace

// clang-format off
void usage() {

  AUTHOR = "Ben Jeurissen (ben.jeurissen@uantwerpen.be),"
           " J-Donald Tournier (jdtournier@gmail.com)"
           " and Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Remove Gibbs ringing artefacts";

  DESCRIPTION
    + "This command removes Gibbs ringing artefacts from MRI images."
      " Two families of methods are provided, selected via the -method option."

    + "The local subvoxel-shift methods (\"kellner\" and \"bautista\") use the approach proposed by Kellner et al."
      " The default \"kellner\" choice is the original slice-wise 2D formulation;"
      " \"bautista\" selects the 3D volume-wise extension proposed by Bautista et al."
      " (see references below)."

    + "The \"tgv\" method targets each 2D slice independently"
      " by reconstructing it at twice the in-plane spatial resolution"
      " under a regularised inverse problem."
      " The data fidelity term enforces consistency between the input k-space"
      " and the central window of the high-resolution Fourier transform"
      " of the reconstruction;"
      " the regulariser is second-order total generalised variation (TGV2)"
      " on the symmetrised spatial derivative."
      " Unlike the local subvoxel-shift methods,"
      " this approach extrapolates the unsampled high frequencies of the slice,"
      " yielding an image at double the in-plane resolution."
      " The cost is convex; minimisation uses a first-order primal-dual"
      " (Chambolle-Pock) algorithm whose data-term proximal step"
      " is diagonal in high-resolution k-space."
      " By default, each high-resolution reconstruction is downsampled back to the"
      " input resolution by direct 2x2 voxel aggregation,"
      " so that the output image matches the input grid;"
      " the -superresolution option exports the high-resolution reconstruction directly"
      " (in-plane sizes doubled, voxel spacing halved,"
      " affine origin shifted by half a voxel along each in-plane axis"
      " so that the field-of-view is preserved)."

    + "This command is designed to run on data directly after it has been reconstructed by the scanner,"
      " before any interpolation of any kind has taken place."
      " You should not run this command after any form of motion correction"
      " (e.g. not after dwifslpreproc)."
      " If however you intend to run a thermal denoising step (eg. dwidenoise),"
      " you should do so before this command to not alter the noise structure,"
      " which would impact on denoising performance."

    + "For best results, any form of filtering performed by the scanner should be disabled,"
      " whether performed in the image domain or k-space."
      " This includes elliptic filtering and other filters "
      " that are often applied to reduce Gibbs ringing artifacts."
      " While this method can still safely be applied to such data,"
      " some residual ringing artefacts may still be present in the output."

    + "Note that these methods are designed to work on images acquired with full k-space coverage."
      " If executed on data acquired with partial Fourier (eg. \"half-scan\") acceleration,"
      " they may not fully remove all ringing artifacts,"
      " and you may observe residuals of the original artifact in the partial Fourier direction."
      " Nonetheless, application of the method is still considered safe and worthwhile."
      " Users are however encouraged to acquired full-Fourier data where possible."

    + "As these methods are based on utilisation of the Fourier shift theorem,"
      " they operate best if provided with complex-valued image data;"
      " in this use case the output image will also be complex-valued."
      " The \"tgv\" method requires complex-valued input,"
      " and always produces complex-valued output.";


  ARGUMENTS
  + Argument ("in", "the input image.").type_image_in()
  + Argument ("out", "the output image"
                     " (at the input image's resolution by default;"
                     " at twice the in-plane resolution if -method tgv -superresolution is given).").type_image_out();


  OPTIONS
  + OptionGroup ("Method selection")

  + Option ("method",
            "select the algorithm used to remove Gibbs ringing"
            " (one of: kellner, bautista, tgv; default: kellner)."
            " The \"kellner\" choice selects the original 2D subvoxel-shift method by Kellner et al.;"
            " \"bautista\" selects its 3D volume-wise extension by Bautista et al.;"
            " \"tgv\" selects 2D TGV-regularised super-resolution k-space extrapolation.")
    + Argument ("choice").type_choice(method_choices)

  + Option ("axes",
            "select the in-plane (slice) axes for the 2D methods (kellner, tgv)"
            " (default: 0,1 - i.e. x-y);"
            " ignored for the bautista method.")
    + Argument ("list").type_sequence_int()


  + OptionGroup ("Options for the local subvoxel-shift methods (kellner, bautista)")

  + Option ("nshifts", "discretization of subpixel spacing"
                       " (default: 20).")
    + Argument ("value").type_integer(8, 128)

  + Option ("minW", "left border of window used for TV computation"
                    " (default: 1).")
    + Argument ("value").type_integer(0, 10)

  + Option ("maxW", "right border of window used for TV computation"
                    " (default: 3).")
    + Argument ("value").type_integer(0, 128)


  + OptionGroup ("Options for the TGV super-resolution method (tgv)")

  + Option ("superresolution",
            "export the super-resolution reconstruction (in-plane sizes doubled,"
            " voxel sizes halved, affine origin shifted by half a voxel along each"
            " in-plane axis to preserve the field of view)"
            " rather than the default LR result obtained by 2x2 voxel aggregation.")

  + Option ("mu", "data-fidelity / regularisation balance coefficient"
                  " (default: 5e-3).")
    + Argument ("value").type_float(0.0)

  + Option ("alpha", "comma-separated TGV2 weights (alpha1, alpha0)"
                     " on the gradient and symmetric-derivative terms respectively"
                     " (default: 1.0,2.0).")
    + Argument ("list").type_sequence_float()

  + Option ("window", "k-space apodisation window applied during cropping"
                      " (default: rect).")
    + Argument ("choice").type_choice(window_choices)

  + Option ("niter", "maximum primal-dual iterations per slice"
                     " (default: 500).")
    + Argument ("value").type_integer(1)

  + Option ("tol", "relative tolerance on primal change for early termination"
                   " (default: 1e-4).")
    + Argument ("value").type_float(0.0)


  + DataType::options();


  REFERENCES
    + "Kellner, E; Dhital, B; Kiselev, V.G & Reisert, M. "
    "Gibbs-ringing artifact removal based on local subvoxel-shifts. "
    "Magnetic Resonance in Medicine, 2016, 76, 1574-1581."

    + "Bautista, T; O'Muircheartaigh, J; Hajnal, JV; & Tournier, J-D. "
    "Removal of Gibbs ringing artefacts for 3D acquisitions using subvoxel shifts. "
    "Proc. ISMRM, 2021, 29, 3535."

    + "Knoll, F.; Bredies, K.; Pock, T.; & Stollberger, R. "
      "Second order total generalized variation (TGV) for MRI. "
      "Magnetic Resonance in Medicine, 2011, 65, 480-491."

    + "Bredies, K.; Kunisch, K.; & Pock, T. "
      "Total generalized variation. "
      "SIAM Journal on Imaging Sciences, 2010, 3, 492-526."

    + "Chambolle, A. & Pock, T. "
      "A first-order primal-dual algorithm for convex problems "
      "with applications to imaging. "
      "Journal of Mathematical Imaging and Vision, 2011, 40, 120-145.";

}
// clang-format on

namespace {

Method get_method() {
  auto opt = get_options("method");
  if (opt.empty())
    return Method::Kellner;
  switch (static_cast<int>(opt[0][0])) {
  case 0:
    return Method::Kellner;
  case 1:
    return Method::Bautista;
  case 2:
    return Method::TGV;
  default:
    throw Exception("invalid value for -method option");
  }
}

/*! Resolve the two in-plane (slice) axes for the 2D methods.
 *  Consults -axes if given, otherwise defaults to {0, 1};
 *  reconciles against any "SliceEncodingDirection" header field with the same
 *  precedence rules used by the original mrdegibbs / mrdegibbs2 commands. */
std::vector<size_t> resolve_2d_slice_axes(const Header &header) {
  std::vector<size_t> slice_axes = {0, 1};
  auto opt = get_options("axes");
  const bool axes_set_manually = !opt.empty();
  if (axes_set_manually) {
    const std::vector<uint32_t> axes = parse_ints<uint32_t>(opt[0][0]);
    if (axes.size() != 2)
      throw Exception("for 2D methods, -axes must specify two comma-separated values");
    if (axes[0] == axes[1])
      throw Exception("two distinct slice axes must be specified");
    slice_axes = {static_cast<size_t>(axes[0]), static_cast<size_t>(axes[1])};
    if (std::max(slice_axes[0], slice_axes[1]) >= static_cast<size_t>(header.ndim()))
      throw Exception("slice axes must be within the dimensionality of the image");
  }

  auto slice_encoding_it = header.keyval().find("SliceEncodingDirection");
  if (slice_encoding_it == header.keyval().end())
    return slice_axes;

  try {
    const Metadata::BIDS::axis_vector_type slice_encoding_axis_onehot =
        Metadata::BIDS::axisid2vector(slice_encoding_it->second);
    std::vector<size_t> auto_slice_axes;
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
        INFO("User's manual selection of within-slice axes consistent with \"SliceEncodingDirection\" field in image "
             "header");
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
  return slice_axes;
}

std::vector<size_t> outer_axes_excluding(const Header &header, const std::vector<size_t> &slice_axes) {
  std::vector<size_t> outer_axes(header.ndim());
  std::iota(outer_axes.begin(), outer_axes.end(), 0);
  outer_axes.erase(std::remove_if(outer_axes.begin(),
                                  outer_axes.end(),
                                  [&](size_t a) {
                                    return std::find(slice_axes.begin(), slice_axes.end(), a) != //
                                           slice_axes.end();
                                  }), //
                   outer_axes.end());
  return outer_axes;
}

void run_subvoxel_shift_2d(const Header &header_in, Image<complex_type> &in) {
  const int nshifts = get_option_value("nshifts", 20);
  const int minW = get_option_value("minW", 1);
  const int maxW = get_option_value("maxW", 3);
  if (minW >= maxW)
    throw Exception("minW must be smaller than maxW");

  const std::vector<size_t> slice_axes = resolve_2d_slice_axes(header_in);
  const std::vector<size_t> outer_axes = outer_axes_excluding(header_in, slice_axes);

  Header header_out(header_in);
  header_out.datatype() =
      DataType::from_command_line(header_in.datatype().is_complex() ? DataType::CFloat32 : DataType::Float32);
  auto out = Image<complex_type>::create(argument[1], header_out);

  ThreadedLoop("performing 2D Gibbs ringing removal (Kellner subvoxel-shift)", in, outer_axes, slice_axes)
      .run_outer(Degibbs::Unring2DFunctor(outer_axes, slice_axes, nshifts, minW, maxW, in, out));
}

void run_subvoxel_shift_3d(const Header &header_in, Image<complex_type> &in) {
  const int nshifts = get_option_value("nshifts", 20);
  const int minW = get_option_value("minW", 1);
  const int maxW = get_option_value("maxW", 3);
  if (minW >= maxW)
    throw Exception("minW must be smaller than maxW");

  if (!get_options("axes").empty())
    WARN("-axes option is ignored when -method bautista is selected");

  if (header_in.keyval().find("SliceEncodingDirection") != header_in.keyval().end())
    WARN("running 3D volume-wise unringing,"                            //
         " but image header contains \"SliceEncodingDirection\" field;" //
         " if data were acquired using multi-slice encoding,"           //
         " run with -method kellner.");                                 //

  Header header_out(header_in);
  header_out.datatype() =
      DataType::from_command_line(header_in.datatype().is_complex() ? DataType::CFloat32 : DataType::Float32);
  auto out = Image<complex_type>::create(argument[1], header_out);

  Degibbs::unring3D(in, out, minW, maxW, nshifts);
}

void run_tgv_super_resolution(const Header &header_in, Image<complex_type> &in) {
  if (!header_in.datatype().is_complex())
    throw Exception("the tgv method requires complex-valued input image data");

  const real_type mu = get_option_value("mu", real_type(5e-3));
  const int max_iter = get_option_value("niter", 500);
  const real_type tol = get_option_value("tol", real_type(1e-4));

  real_type alpha1 = real_type(1.0);
  real_type alpha0 = real_type(2.0);
  auto opt_alpha = get_options("alpha");
  if (!opt_alpha.empty()) {
    auto values = parse_floats(opt_alpha[0][0]);
    if (values.size() != 2)
      throw Exception("-alpha requires exactly two comma-separated values (alpha1,alpha0)");
    alpha1 = static_cast<real_type>(values[0]);
    alpha0 = static_cast<real_type>(values[1]);
  }

  Mrdegibbs2::WindowKind window_kind = Mrdegibbs2::WindowKind::Rect;
  auto opt_window = get_options("window");
  if (!opt_window.empty()) {
    switch (static_cast<int>(opt_window[0][0])) {
    case 0:
      window_kind = Mrdegibbs2::WindowKind::Rect;
      break;
    case 1:
      window_kind = Mrdegibbs2::WindowKind::Tukey;
      break;
    case 2:
      window_kind = Mrdegibbs2::WindowKind::Hamming;
      break;
    default:
      throw Exception("invalid value for -window option");
    }
  }

  const std::vector<size_t> slice_axes = resolve_2d_slice_axes(header_in);
  const std::vector<size_t> outer_axes = outer_axes_excluding(header_in, slice_axes);

  const bool superresolution = !get_options("superresolution").empty();
  for (auto a : slice_axes)
    if (a >= 3 && superresolution)
      throw Exception("with -superresolution, slice axes must be spatial (0, 1, or 2)");

  Header header_out(header_in);
  if (superresolution) {
    header_out.size(slice_axes[0]) = 2 * header_in.size(slice_axes[0]);
    header_out.size(slice_axes[1]) = 2 * header_in.size(slice_axes[1]);
    header_out.spacing(slice_axes[0]) = header_in.spacing(slice_axes[0]) * 0.5;
    header_out.spacing(slice_axes[1]) = header_in.spacing(slice_axes[1]) * 0.5;
    // Half-voxel affine origin shift (in new-spacing units) so the FOV centre is preserved
    // and HR voxel positions match the half-shifted sampling grid produced by the solver.
    header_out.transform().translation() -=
        0.5 * header_out.spacing(slice_axes[0]) * header_out.transform().linear().col(slice_axes[0]) +
        0.5 * header_out.spacing(slice_axes[1]) * header_out.transform().linear().col(slice_axes[1]);
  }
  header_out.datatype() = DataType::from_command_line(DataType::CFloat32);
  if (!header_out.datatype().is_complex())
    throw Exception("output datatype must be complex-valued for the tgv method");
  auto out = Image<complex_type>::create(argument[1], header_out);

  Mrdegibbs2::run_tgv2d(
      in, out, outer_axes, slice_axes, mu, alpha1, alpha0, window_kind, max_iter, tol, !superresolution);
}

} // namespace

void run() {
  const Method method = get_method();

  auto header_in = Header::open(argument[0]);
  auto in = header_in.get_image<complex_type>();

  switch (method) {
  case Method::Kellner:
    run_subvoxel_shift_2d(header_in, in);
    break;
  case Method::Bautista:
    run_subvoxel_shift_3d(header_in, in);
    break;
  case Method::TGV:
    run_tgv_super_resolution(header_in, in);
    break;
  }
}
