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

#include "dwi/gradient.h"
#include "file/nifti_utils.h"

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

    Option no_bvalue_scaling_option
    = Option ("no_bvalue_scaling",
              "disable scaling of diffusion b-values by the square of the "
              "corresponding DW gradient norm (see Desciption).");

    const char* const no_bvalue_scaling_description (
      "The -no_bvalue_scaling option is reserved for use in importing malformed "
      "diffusion gradient tables. Typically, when the input diffusion-weighting "
      "directions are not of unit norm, they are rescaled to unit norm by MRtrix3, "
      "with corresponding scaling of the b-values by the squares of these vector "
      "norms (this is how multi-shell acquisitions are frequently achieved on "
      "scanner platforms). However in some rare instances, the b-values may be "
      "correct, despite the vectors not being of unit norm. In such cases, use of "
      "this option will result in the vectors still being normalised, but the "
      "corresponding b-value scaling not being applied."
    );




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






    Eigen::MatrixXd get_DW_scheme (const Header& header, bool bvalue_scaling)
    {
      try {
        auto grad = get_raw_DW_scheme (header);
        check_DW_scheme (header, grad);

        Eigen::Array<default_type, Eigen::Dynamic, 1> squared_norms = grad.leftCols(3).rowwise().squaredNorm();
        // ensure interpreted directions are always normalised
        // also make sure that directions of [0, 0, 0] don't affect subsequent calculations
        for (ssize_t row = 0; row != grad.rows(); ++row) {
          if (squared_norms[row])
            grad.row(row).template head<3>().array() /= std::sqrt(squared_norms[row]);
          else
            squared_norms[row] = 1.0;
        }
        // only update input header if the normalisation is significant
        const default_type max_log_scaling_factor = squared_norms.log().abs().maxCoeff();
        if (max_log_scaling_factor > 1e-14) {
          const default_type max_scaling_factor = std::exp (max_log_scaling_factor);
          if (bvalue_scaling) {
            grad.col(3).array() *= squared_norms;
            if (max_log_scaling_factor > 1e-5) {
              CONSOLE ("b-values scaled by the square of DW gradient norm (maximum scaling factor " + str(max_scaling_factor, 6) + ")");
            } else {
              INFO ("b-values corrected for double-precision normalisation of DW vectors (maximum scaling factor " + str(max_scaling_factor, 6) + ")");
            }
          } else {
            CONSOLE ("b-value scaling explicitly disabled by user; maximum scaling factor would have been " + str(max_scaling_factor, 6));
          }
          // write the scheme as interpreted back into the header
          set_DW_scheme (const_cast<Header&> (header), grad);
        } else if (bvalue_scaling) {
          WARN ("Use of -no_bvalue_scaling had no effect; gradient vectors are all of unit norm");
        }

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


