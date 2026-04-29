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

#include "command.h"
#include "header.h"
#include "image.h"
#include "algo/threaded_loop.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "math/SH.h"
#include "metadata/phase_encoding.h"

#include "dwi/shgqi/transform.h"


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Sudhir K. Pathak";

  SYNOPSIS = "Closed-form SH-GQI: map DWI signals to spherical-harmonic ODF "
             "coefficients via the Funk-Hecke theorem applied to the GQI sinc kernel";

  DESCRIPTION
    + "Implements the analytical mapping c = M(grad, sigma, D, mu, p, lmax) * E "
      "where E is the per-voxel signal vector normalised by the mean b=0 image "
      "and c are the even-order real-SH coefficients of the GQI spin-distribution "
      "function. The transfer matrix M is acquisition-only (depends on the b-values "
      "and gradient directions, not on the voxel data) and is built once. "
      "Per-voxel cost is a single matrix-vector product."

    + "The kernel argument is a_i = sigma * sqrt(6 D b_i) per Yeh et al. 2010 (Eq 9). "
      "Funk-Hecke gives K_l(a) = (4 pi (-1)^{l/2} / a) integral_0^a j_l(t) dt for "
      "even l, computed here by 30-term power series for a <= 0.5 and 64-point "
      "Gauss-Legendre quadrature otherwise. Tikhonov regularisation with the "
      "Laplace-Beltrami operator is diagonal in the SH basis: c_lm /= (1 + mu [l(l+1)]^p)."

    + "The output is a standard MRtrix3 SH coefficient image (Descoteaux 2007 "
      "real basis, even orders only) and is directly consumable by tckgen, sh2peaks, "
      "mrview, etc."

    + Math::SH::encoding_description;

  REFERENCES
    + "Yeh, F.-C.; Wedeen, V.J. & Tseng, W.-Y.I. " // Internal
      "Generalized q-Sampling Imaging. "
      "IEEE Transactions on Medical Imaging, 2010, 29(9), 1626-1635"

    + "Descoteaux, M.; Angelino, E.; Fitzgibbons, S. & Deriche, R. " // Internal
      "Regularized, fast, and robust analytical Q-ball imaging. "
      "Magnetic Resonance in Medicine, 2007, 58(3), 497-510";

  ARGUMENTS
    + Argument ("dwi", "the input diffusion-weighted image").type_image_in()
    + Argument ("odf", "the output SH coefficient image").type_image_out();

  OPTIONS
    + Option ("lmax", "maximum even SH degree of the output (default: 8)")
    +   Argument ("L").type_integer (0, 30)

    + Option ("sigma", "GQI sampling-length factor sigma (dimensionless; default: 1.25)")
    +   Argument ("s").type_float (0.1, 10.0)

    + Option ("diff", "white-matter diffusivity D in mm^2/s (default: 3e-3)")
    +   Argument ("D").type_float (1e-5, 1e-2)

    + Option ("mu", "Laplace-Beltrami regularisation strength (default: 6e-3)")
    +   Argument ("mu").type_float (0.0, 1.0)

    + Option ("p", "Laplace-Beltrami regularisation exponent (default: 1.0)")
    +   Argument ("p").type_float (0.0, 4.0)

    + Option ("mask", "process only voxels within the supplied binary mask")
    +   Argument ("image").type_image_in()

    + DWI::GradImportOptions()
    + Stride::Options;
}



using value_type = float;


class SHGQIProcessor { MEMALIGN(SHGQIProcessor)
  public:
    SHGQIProcessor (const Eigen::MatrixXd& M_in,
                    const std::vector<size_t>& bzeros_in,
                    Image<bool>& mask) :
      M (M_in),
      bzeros (bzeros_in),
      mask (mask),
      E (M_in.cols()),
      c (M_in.rows()) { }

    template <class DwiImageType, class ShImageType>
      void operator() (DwiImageType& dwi, ShImageType& odf)
      {
        if (mask.valid()) {
          assign_pos_of (dwi, 0, 3).to (mask);
          if (!mask.value()) {
            for (auto l = Loop (3) (odf); l; ++l)
              odf.value() = value_type (0);
            return;
          }
        }

        // 1. Compute mean b=0 (S0).
        double S0 = 0.0;
        if (!bzeros.empty()) {
          for (size_t i : bzeros) {
            dwi.index (3) = i;
            S0 += double (dwi.value());
          }
          S0 /= double (bzeros.size());
        }
        if (!std::isfinite (S0) || S0 <= 0.0) {
          for (auto l = Loop (3) (odf); l; ++l)
            odf.value() = value_type (0);
          return;
        }
        const double inv_S0 = 1.0 / S0;

        // 2. Read full DWI vector and normalise.
        const Eigen::Index G = M.cols();
        for (Eigen::Index i = 0; i < G; ++i) {
          dwi.index (3) = i;
          double v = double (dwi.value()) * inv_S0;
          if (!std::isfinite (v))
            v = 0.0;
          E[i] = v;
        }

        // 3. Closed-form SH coefficients.
        c.noalias() = M * E;

        // 4. Write out.
        for (auto l = Loop (3) (odf); l; ++l)
          odf.value() = value_type (c[odf.index (3)]);
      }

  private:
    const Eigen::MatrixXd&     M;
    const std::vector<size_t>& bzeros;
    Image<bool>                mask;
    Eigen::VectorXd            E, c;
};



void run ()
{
  Header header_in (Header::open (argument[0]));

  DWI::SH_GQI::Params params;
  auto opt = get_options ("lmax");
  if (opt.size())
    params.L_max = int (opt[0][0]);
  if (params.L_max & 1)
    throw Exception ("-lmax must be a non-negative even integer (got " + str (params.L_max) + ")");

  opt = get_options ("sigma");
  if (opt.size()) params.sigma = double (opt[0][0]);

  opt = get_options ("diff");
  if (opt.size()) params.D = double (opt[0][0]);

  opt = get_options ("mu");
  if (opt.size()) params.mu = double (opt[0][0]);

  opt = get_options ("p");
  if (opt.size()) params.p = double (opt[0][0]);

  Image<bool> mask;
  opt = get_options ("mask");
  if (opt.size()) {
    mask = Header::open (opt[0][0]).get_image<bool>();
    check_dimensions (header_in, mask, 0, 3);
  }

  // Gradient table (G x 4) [gx gy gz b].
  Eigen::MatrixXd grad = DWI::get_DW_scheme (header_in);
  if (grad.rows() != header_in.size (3))
    throw Exception ("number of gradient entries (" + str (grad.rows()) +
                     ") does not match volume axis (" + str (header_in.size (3)) + ")");

  // Identify b=0 volumes for normalisation.
  DWI::Shells shells (grad);
  std::vector<size_t> bzeros;
  if (shells.has_bzero())
    bzeros = shells.smallest().get_volumes();
  if (bzeros.empty())
    throw Exception ("no b=0 volumes detected; SH-GQI requires at least one b=0 image for signal normalisation");

  INFO ("SH-GQI: G = " + str (grad.rows()) + ", b=0 volumes = " + str (bzeros.size())
        + ", L_max = " + str (params.L_max)
        + ", sigma = " + str (params.sigma)
        + ", D = " + str (params.D)
        + ", mu = " + str (params.mu)
        + ", p = " + str (params.p));

  // Build the transfer matrix once.
  Eigen::MatrixXd M = DWI::SH_GQI::transfer_matrix (grad, params);

  // Output header: 4D SH image with (L+1)(L+2)/2 volumes.
  Header header_out (header_in);
  header_out.ndim() = 4;
  header_out.size (3) = M.rows();
  header_out.datatype() = DataType::Float32;
  header_out.datatype().set_byte_order_native();
  Stride::set_from_command_line (header_out, Stride::contiguous_along_axis (3, header_in));
  DWI::stash_DW_scheme (header_out, grad);
  Metadata::PhaseEncoding::clear_scheme (header_out.keyval());

  auto dwi = header_in.get_image<value_type>().with_direct_io (3);
  auto odf = Image<value_type>::create (argument[1], header_out);

  SHGQIProcessor processor (M, bzeros, mask);
  ThreadedLoop ("performing closed-form SH-GQI reconstruction", dwi, 0, 3)
      .run (processor, dwi, odf);
}
