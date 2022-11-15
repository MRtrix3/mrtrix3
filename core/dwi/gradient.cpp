/* Copyright (c) 2008-2022 the MRtrix3 contributors.
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

#include "dwi/gradient.h"
#include "file/nifti_utils.h"
#include "dwi/shells.h"

namespace MR
{
  namespace DWI
  {

    using namespace App;
    using namespace Eigen;

    OptionGroup GradImportOptions()
    {
      OptionGroup group ("DW gradient table import options");

      group
        + Option ("grad",
            "Provide the diffusion-weighted gradient scheme used in the acquisition "
            "in a text file. This should be supplied as a 4xN text file with each line "
            "is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the "
            "applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion "
            "gradient scheme is present in the input image header, the data provided with "
            "this option will be instead used.")
        +   Argument ("file").type_file_in()

        + Option ("fslgrad",
            "Provide the diffusion-weighted gradient scheme used in the acquisition in FSL "
            "bvecs/bvals format files. If a diffusion gradient scheme is present in the "
            "input image header, the data provided with this option will be instead used.")
        +   Argument ("bvecs").type_file_in()
        +   Argument ("bvals").type_file_in();

      return group;
    }


    OptionGroup GradExportOptions()
    {
      return OptionGroup ("DW gradient table export options")

        + Option ("export_grad_mrtrix", "export the diffusion-weighted gradient table to file in MRtrix format")
        +   Argument ("path").type_file_out()

        + Option ("export_grad_fsl", "export the diffusion-weighted gradient table to files in FSL (bvecs / bvals) format")
        +   Argument ("bvecs_path").type_file_out()
        +   Argument ("bvals_path").type_file_out();
    }

    Option bvalue_scaling_option = Option ("bvalue_scaling",
        "enable or disable scaling of diffusion b-values by the square of the "
        "corresponding DW gradient norm (see Desciption). "
        "Valid choices are yes/no, true/false, 0/1 (default: automatic).")
    +   Argument ("mode").type_bool();


    const char* const bvalue_scaling_description (
      "The -bvalue_scaling option controls an aspect of the import of "
      "diffusion gradient tables. When the input diffusion-weighting "
      "direction vectors have norms that differ substantially from unity, "
      "the b-values will be scaled by the square of their corresponding "
      "vector norm (this is how multi-shell acquisitions are frequently "
      "achieved on scanner platforms). However in some rare instances, the "
      "b-values may be correct, despite the vectors not being of unit norm "
      "(or conversely, the b-values may need to be rescaled even though the "
      "vectors are close to unit norm). This option allows the user to "
      "control this operation and override MRrtix3's automatic detection."
    );



    BValueScalingBehaviour get_cmdline_bvalue_scaling_behaviour ()
    {
      auto opt = App::get_options ("bvalue_scaling");
      if (opt.empty())
        return BValueScalingBehaviour::Auto;
      if (opt[0][0])
        return BValueScalingBehaviour::UserOn;
      return BValueScalingBehaviour::UserOff;
    }


    Eigen::MatrixXd parse_DW_scheme (const Header& header)
    {
      Eigen::MatrixXd G;
      const auto it = header.keyval().find ("dw_scheme");
      if (it != header.keyval().end()) {
        try {
          G = parse_matrix (it->second);
        } catch (Exception& e) {
          throw Exception (e, "malformed DW scheme in image \"" + header.name() + "\"");
        }
      }
      return G;
    }




    Eigen::MatrixXd load_bvecs_bvals (const Header& header, const std::string& bvecs_path, const std::string& bvals_path)
    {
      Eigen::MatrixXd bvals, bvecs;
      try {
        bvals = load_matrix<> (bvals_path);
        bvecs = load_matrix<> (bvecs_path);
      } catch (Exception& e) {
        throw Exception (e, "Unable to import files \"" + bvecs_path + "\" and \"" + bvals_path + "\" as FSL bvecs/bvals pair");
      }

      if (bvals.rows() != 1) {
        if (bvals.cols() == 1)
          bvals.transposeInPlace();  // transpose if file contains column vector
        else
          throw Exception ("bvals file must contain 1 row or column only (file \"" + bvals_path + "\" has " + str(bvals.rows()) + ")");
      }
      if (bvecs.rows() != 3) {
        if (bvecs.cols() == 3)
          bvecs.transposeInPlace();
        else
          throw Exception ("bvecs file must contain exactly 3 rows or columns (file \"" + bvecs_path + "\" has " + str(bvecs.rows()) + ")");
      }

      if (bvals.cols() != bvecs.cols())
        throw Exception ("bvecs and bvals files must have same number of diffusion directions (file \"" + bvecs_path + "\" has " + str(bvecs.cols()) + ", file \"" + bvals_path + "\" has " + str(bvals.cols()) + ")");

      const size_t num_volumes = header.ndim() < 4 ? 1 : header.size(3);
      if (size_t(bvals.cols()) != num_volumes)
        throw Exception ("bvecs and bvals files must have same number of diffusion directions as DW-image (gradients: " + str(bvecs.cols()) + ", image: " + str(num_volumes) + ")");

      // bvecs format actually assumes a LHS coordinate system even if image is
      // stored using RHS - x axis is flipped to make linear 3x3 part of
      // transform have negative determinant:
      vector<size_t> order;
      auto adjusted_transform = File::NIfTI::adjust_transform (header, order);
      if (adjusted_transform.linear().determinant() > 0.0)
        bvecs.row(0) = -bvecs.row(0);

      // account for the fact that bvecs are specified wrt original image axes,
      // which may have been re-ordered and/or inverted by MRtrix to match the
      // expected anatomical frame of reference:
      Eigen::MatrixXd G (bvecs.cols(), 3);
      for (ssize_t n = 0; n < G.rows(); ++n) {
        G(n,order[0]) = header.stride(order[0]) > 0 ? bvecs(0,n) : -bvecs(0,n);
        G(n,order[1]) = header.stride(order[1]) > 0 ? bvecs(1,n) : -bvecs(1,n);
        G(n,order[2]) = header.stride(order[2]) > 0 ? bvecs(2,n) : -bvecs(2,n);
      }

      // rotate gradients into scanner coordinate system:
      Eigen::MatrixXd grad (G.rows(), 4);

      grad.leftCols<3>().transpose() = header.transform().rotation() * G.transpose();
      grad.col(3) = bvals.row(0);

      return grad;
    }





    void save_bvecs_bvals (const Header& header, const std::string& bvecs_path, const std::string& bvals_path)
    {
      const auto grad = parse_DW_scheme (header);

      // rotate vectors from scanner space to image space
      Eigen::MatrixXd G = grad.leftCols<3>() * header.transform().rotation();

      // deal with FSL requiring gradient directions to coincide with data strides
      // also transpose matrices in preparation for file output
      vector<size_t> order;
      auto adjusted_transform = File::NIfTI::adjust_transform (header, order);
      Eigen::MatrixXd bvecs (3, grad.rows());
      Eigen::MatrixXd bvals (1, grad.rows());
      for (ssize_t n = 0; n < G.rows(); ++n) {
        bvecs(0,n) = header.stride(order[0]) > 0 ? G(n,order[0]) : -G(n,order[0]);
        bvecs(1,n) = header.stride(order[1]) > 0 ? G(n,order[1]) : -G(n,order[1]);
        bvecs(2,n) = header.stride(order[2]) > 0 ? G(n,order[2]) : -G(n,order[2]);
        bvals(0,n) = grad(n,3);
      }

      // bvecs format actually assumes a LHS coordinate system even if image is
      // stored using RHS - x axis is flipped to make linear 3x3 part of
      // transform have negative determinant:
      if (adjusted_transform.linear().determinant() > 0.0)
        bvecs.row(0) = -bvecs.row(0);

      save_matrix (bvecs, bvecs_path, KeyValues(), false);
      save_matrix (bvals, bvals_path, KeyValues(), false);
    }



    void clear_DW_scheme (Header& header)
    {
      auto it = header.keyval().find ("dw_scheme");
      if (it != header.keyval().end())
        header.keyval().erase (it);
    }



    Eigen::MatrixXd get_raw_DW_scheme (const Header& header)
    {
      DEBUG ("searching for suitable gradient encoding...");
      using namespace App;
      Eigen::MatrixXd grad;

      // check whether the DW scheme has been provided via the command-line:
      const auto opt_mrtrix = get_options ("grad");
      if (opt_mrtrix.size())
        grad = load_matrix<> (opt_mrtrix[0][0]);

      const auto opt_fsl = get_options ("fslgrad");
      if (opt_fsl.size()) {
        if (opt_mrtrix.size())
          throw Exception ("Diffusion gradient table can be provided using either -grad or -fslgrad option, but NOT both");
        grad = load_bvecs_bvals (header, opt_fsl[0][0], opt_fsl[0][1]);
      }

      // otherwise use the information from the header:
      if (!opt_mrtrix.size() && !opt_fsl.size())
        grad = parse_DW_scheme (header);

      return grad;
    }






    Eigen::MatrixXd get_DW_scheme (const Header& header, BValueScalingBehaviour bvalue_scaling)
    {
      try {
        auto grad = get_raw_DW_scheme (header);
        check_DW_scheme (header, grad);

        Eigen::Array<default_type, Eigen::Dynamic, 1> squared_norms = grad.leftCols(3).rowwise().squaredNorm();
        // ensure interpreted directions are always normalised
        // also make sure that directions of [0, 0, 0] don't affect subsequent calculations
        bool warnambiguous = false;
        for (ssize_t row = 0; row != grad.rows(); ++row) {
          if (squared_norms[row])
            grad.row(row).template head<3>().array() /= std::sqrt(squared_norms[row]);
          else
            warnambiguous = warnambiguous || ( grad.row(row)[3] > bzero_threshold() );
        }
        // modulate verbosity of message & whether or not header is modified
        // based on magnitude of effect of normalisation
        const default_type max_log_scaling_factor = squared_norms.unaryExpr ([](double v) {
            return v > 0.0 ? abs(log(v)) : 0.0; }).maxCoeff();
        const default_type max_scaling_factor = std::exp (max_log_scaling_factor);
        const bool exceeds_single_precision = max_log_scaling_factor > 1e-5;
        const bool requires_bvalue_scaling = max_log_scaling_factor > 0.01;

        DEBUG ("b-value scaling: max scaling factor = exp("
            + str(max_log_scaling_factor) + ") = " + str(max_scaling_factor));

        if (( requires_bvalue_scaling && bvalue_scaling == BValueScalingBehaviour::Auto ) ||
            bvalue_scaling == BValueScalingBehaviour::UserOn ) {
          grad.col(3).array() *= squared_norms;
          if (warnambiguous)
            WARN ("Ambiguous [ 0 0 0 non-zero ] entries found in DW gradient table. "
                  "These will be interpreted as b=0 volumes unless -bvalue_scaling is disabled.");
          INFO ("b-values scaled by the square of DW gradient norm "
              "(maximum scaling factor = " + str(max_scaling_factor) + ")");
        }
        else if (bvalue_scaling == BValueScalingBehaviour::UserOff ) {
          if (requires_bvalue_scaling) {
            CONSOLE ("disabling b-value scaling during normalisation of DW vectors on user request "
                "(maximum scaling factor would have been " + str(max_scaling_factor) + ")");
          } else {
            WARN ("use of -bvalue_scaling option had no effect: gradient vector norms are all within tolerance "
                "(maximum scaling factor = " + str(max_scaling_factor) + ")");
          }
        }

        // write the scheme as interpreted back into the header if:
        // - vector normalisation effect is large, regardless of whether or not b-value scaling was applied
        // - gradient information was pulled from file
        // - explicit b-value scaling is requested
        if (exceeds_single_precision || get_options ("grad").size() || get_options ("fslgrad").size() || bvalue_scaling != BValueScalingBehaviour::Auto)
          set_DW_scheme (const_cast<Header&> (header), grad);

        INFO ("found " + str (grad.rows()) + "x" + str (grad.cols()) + " diffusion gradient table");
        return grad;
      }
      catch (Exception& e) {
        clear_DW_scheme (const_cast<Header&> (header));
        throw Exception (e, "error importing diffusion gradient table for image \"" + header.name() + "\"");
      }
    }



    void export_grad_commandline (const Header& header)
    {
      auto check = [](const Header& h) -> const Header& {
        if (h.keyval().find("dw_scheme") == h.keyval().end())
          throw Exception ("no gradient information found within image \"" + h.name() + "\"");
        return h;
      };

      auto opt = get_options ("export_grad_mrtrix");
      if (opt.size())
        save_matrix (parse_DW_scheme (check (header)), opt[0][0]);

      opt = get_options ("export_grad_fsl");
      if (opt.size())
        save_bvecs_bvals (check (header), opt[0][0], opt[0][1]);
    }




  }
}


