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

#ifndef __dwi_gradient_h__
#define __dwi_gradient_h__

#include "app.h"
#include "header.h"
#include "dwi/shells.h"
#include "file/config.h"
#include "file/path.h"
#include "math/condition_number.h"
#include "math/sphere.h"
#include "math/SH.h"


namespace MR
{
  namespace App { class OptionGroup; }

  namespace DWI
  {

    App::OptionGroup GradImportOptions();
    App::OptionGroup GradExportOptions();

    extern App::Option bvalue_scaling_option;
    extern const char* const bvalue_scaling_description;



    //! check that the DW scheme matches the DWI data in \a header
    /*! \note This is mostly for internal use. If you have obtained the DW
     * scheme using DWI::get_DW_scheme(), it should already by guaranteed to
     * match the corresponding header. */
    template <class MatrixType>
      inline void check_DW_scheme (const Header& header, const MatrixType& grad)
      {
        if (!grad.rows())
          throw Exception ("no valid diffusion gradient table found");
        if (grad.cols() < 4)
          throw Exception ("unexpected diffusion gradient table matrix dimensions");

        if (header.ndim() >= 4) {
          if (header.size (3) != (int) grad.rows())
            throw Exception ("number of studies in base image (" + str(header.size(3))
                + ") does not match number of rows in diffusion gradient table (" + str(grad.rows()) + ")");
        }
        else if (grad.rows() != 1)
          throw Exception ("For images with less than four dimensions, gradient table can have one row only");
      }






    /*! \brief convert the DW encoding matrix in \a grad into a
     * azimuth/elevation direction set, using only the DWI volumes as per \a
     * dwi */
    template <class MatrixType, class IndexVectorType>
      inline Eigen::MatrixXd gen_direction_matrix (
          const MatrixType& grad,
          const IndexVectorType& dwi)
      {
        Eigen::MatrixXd dirs (dwi.size(),2);
        for (size_t i = 0; i < dwi.size(); i++) {
          dirs (i,0) = std::atan2 (grad (dwi[i],1), grad (dwi[i],0));
          auto z = grad (dwi[i],2) / grad.row (dwi[i]).template head<3>().norm();
          if (z >= 1.0)
            dirs(i,1) = 0.0;
          else if (z <= -1.0)
            dirs (i,1) = Math::pi;
          else
            dirs (i,1) = std::acos (z);
        }
        return dirs;
      }





    template <class MatrixType>
    default_type condition_number_for_lmax (const MatrixType& dirs, int lmax)
    {
      Eigen::MatrixXd g;
      if (dirs.cols() == 2) // spherical coordinates:
        g = dirs;
      else // Cartesian to spherical:
        g = Math::Sphere::cartesian2spherical (dirs).leftCols(2);

      return Math::condition_number (Math::SH::init_transform (g, lmax));
    }



    //! load and rectify FSL-style bvecs/bvals DW encoding files
    /*! This will load the bvecs/bvals files at the path specified, and convert
     * them to the format expected by MRtrix.  This involves rotating the
     * vectors into the scanner frame of reference, and may also involve
     * re-ordering and/or inverting of the vector elements to match the
     * re-ordering performed by MRtrix for non-axial scans. */
    Eigen::MatrixXd load_bvecs_bvals (const Header& header, const std::string& bvecs_path, const std::string& bvals_path);



    //! export gradient table in FSL format (bvecs/bvals)
    /*! This will take the gradient table information from a header and export it
     * to a bvecs/bvals file pair. In addition to splitting the information over two
     * files, the vectors must be reoriented; firstly to change from scanner space to
     * image space, and then to compensate for the fact that FSL defines its vectors
     * with regards to the data strides in the image file.
     */
    void save_bvecs_bvals (const Header&, const std::string&, const std::string&);



    namespace
    {
      template <class MatrixType>
      std::string scheme2str (const MatrixType& G)
      {
        std::string dw_scheme;
        for (ssize_t row = 0; row < G.rows(); ++row) {
          std::string line = str(G(row,0), 10);
          for (ssize_t col = 1; col < G.cols(); ++col)
            line += "," + str(G(row,col), 10);
          add_line (dw_scheme, line);
        }
        return dw_scheme;
      }
    }



    //! store the DW gradient encoding matrix in a header
    /*! this will store the DW gradient encoding matrix into the
     * Header::keyval() structure of \a header, under the key 'dw_scheme'.
     */
    template <class MatrixType>
      void set_DW_scheme (Header& header, const MatrixType& G)
      {
        if (!G.rows()) {
          auto it = header.keyval().find ("dw_scheme");
          if (it != header.keyval().end())
            header.keyval().erase (it);
          return;
        }
        try {
          check_DW_scheme (header, G);
          header.keyval()["dw_scheme"] = scheme2str (G);
        } catch (Exception&) {
          WARN ("attempt to add non-matching DW scheme to header - ignored");
        }
      }



    //! parse the DW gradient encoding matrix from a header
    /*! extract the DW gradient encoding matrix stored in the \a header if one
     * is present. This is expected to be stored in the Header::keyval()
     * structure, under the key 'dw_scheme'.
     *
     * \note This is mostly for internal use. In general, you should use
     * DWI::get_DW_scheme()
     */
    Eigen::MatrixXd parse_DW_scheme (const Header& header);



    //! get the DW scheme as found in the headers or supplied at the command-line
    /*! return the DW gradient encoding matrix found from the command-line
     * arguments, or if not provided that way, as stored in the \a header.
     * This return the scheme prior to any modification or validation.
     *
     * \note This is mostly for internal use. In general, you should use
     * DWI::get_DW_scheme()
     */
    Eigen::MatrixXd get_raw_DW_scheme (const Header& header);



    //! clear any DW gradient encoding scheme from the header
    void clear_DW_scheme (Header&);



    //! 'stash' the DW gradient table
    /*! Store the _used_ DW gradient table to Header::keyval() key
     *  'prior_dw_scheme', and delete the key 'dw_scheme' if it exists.
     *  This means that the scheme will no longer be identified by function
     *  parse_DW_scheme(), but still resides within the header data and
     *  can be extracted manually. This should be used when
     *  diffusion-weighted images are used to generate something that is
     *  _not_ diffusion_weighted volumes.
     */
    template <class MatrixType>
    void stash_DW_scheme (Header& header, const MatrixType& grad)
    {
      clear_DW_scheme (header);
      if (grad.rows())
        header.keyval()["prior_dw_scheme"] = scheme2str (grad);
    }





    enum class BValueScalingBehaviour { Auto, UserOn, UserOff };
    BValueScalingBehaviour get_cmdline_bvalue_scaling_behaviour ();

    //! get the fully-interpreted DW gradient encoding matrix
    /*!  find and validate the DW gradient encoding matrix, using the following
     * procedure:
     * - if the \c -grad option has been supplied, then load the matrix assuming
     *     it is in MRtrix format, and return it;
     * - if the \c -fslgrad option has been supplied, then load and rectify the
     *     bvecs/bvals pair using load_bvecs_bvals() and return it;
     * - if the DW_scheme member of the header is non-empty, return it;
     * - validate the DW scheme and ensure it matches the header;
     * - if \a bvalue_scaling is \c true (the default), scale the b-values
     *   accordingly, but only if non-unit vectors are detected;
     * - normalise the gradient vectors;
     * - update the header with this information
     */
    Eigen::MatrixXd get_DW_scheme (const Header& header, BValueScalingBehaviour bvalue_scaling = BValueScalingBehaviour::Auto);




    //! process GradExportOptions command-line options
    /*! this checks for the \c -export_grad_mrtrix & \c -export_grad_fsl
     * options, and exports the DW schemes if and as requested. */
    void export_grad_commandline (const Header& header);


    //! \brief get the matrix mapping SH coefficients to amplitudes
    /*! Computes the matrix mapping SH coefficients to the directions specified
     * in \a directions (in spherical coordinates), up to a given lmax. By
     * default, this is computed from the number of DW directions, up to a
     * maximum value of \a default_lmax (defaults to 8), or the value specified
     * using c -lmax command-line option (if \a lmax_from_command_line is
     * true). If the resulting DW scheme is ill-posed (condition number less
     * than 10), lmax will be reduced until it becomes sufficiently well
     * conditioned (unless overridden on the command-line).
     *
     * Note that this uses get_valid_DW_scheme() to get the DW_scheme, so will
     * check for the -grad option as required. */
    template <class MatrixType>
      Eigen::MatrixXd compute_SH2amp_mapping (
          const MatrixType& directions,
          bool lmax_from_command_line = true,
          int default_lmax = 8)
      {
        int lmax = -1;
        int lmax_from_ndir = Math::SH::LforN (directions.rows());
        bool lmax_set_from_commandline = false;
        if (lmax_from_command_line) {
          auto opt = App::get_options ("lmax");
          if (opt.size()) {
            lmax_set_from_commandline = true;
            lmax = to<int> (opt[0][0]);
            if (lmax % 2)
              throw Exception ("lmax must be an even number");
            if (lmax < 0)
              throw Exception ("lmax must be a non-negative number");
            if (lmax > lmax_from_ndir) {
              WARN ("not enough directions for lmax = " + str(lmax) + " - dropping down to " + str(lmax_from_ndir));
              lmax = lmax_from_ndir;
            }
          }
        }

        if (lmax < 0) {
          lmax = lmax_from_ndir;
          if (lmax > default_lmax)
            lmax = default_lmax;
        }

        INFO ("computing SH transform using lmax = " + str (lmax));

        int lmax_prev = lmax;
        Eigen::MatrixXd mapping;
        do {
          mapping = Math::SH::init_transform (directions, lmax);
          const default_type cond = Math::condition_number (mapping);
          if (cond < 10.0)
            break;
          WARN ("directions are poorly distributed for lmax = " + str(lmax) + " (condition number = " + str (cond) + ")");
          if (cond < 100.0 || lmax_set_from_commandline)
            break;
          lmax -= 2;
        } while (lmax >= 0);

        if (lmax_prev != lmax)
          WARN ("reducing lmax to " + str(lmax) + " to improve conditioning");

        return mapping;
      }





    //! \brief get the maximum spherical harmonic order given a set of directions
    /*! Computes the maximum spherical harmonic order \a lmax given a set of
     *  directions on the sphere. This may be less than the value requested at
     *  the command-line, or that calculated from the number of directions, if
     *  the resulting transform matrix is ill-posed. */
      inline size_t lmax_for_directions (const Eigen::MatrixXd& directions,
          const bool lmax_from_command_line = true,
          const int default_lmax = 8)
      {
        const auto mapping = compute_SH2amp_mapping (directions, lmax_from_command_line, default_lmax);
        return Math::SH::LforN (mapping.cols());
      }


  }
}

#endif

