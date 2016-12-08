/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __dwi_gradient_h__
#define __dwi_gradient_h__

// These lines are to silence deprecation warnings with Eigen & GCC v5
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <Eigen/SVD>
#pragma GCC diagnostic pop


#include "app.h"
#include "file/path.h"
#include "file/config.h"
#include "header.h"
#include "math/SH.h"
#include "dwi/shells.h"


namespace MR
{
  namespace App { class OptionGroup; }

  namespace DWI
  {

    App::OptionGroup GradImportOptions (bool include_bvalue_scaling = true);
    App::OptionGroup GradExportOptions();


    //! ensure each non-b=0 gradient vector is normalised to unit amplitude
    template <class MatrixType>
      Eigen::MatrixXd& normalise_grad (MatrixType& grad)
      {
        if (grad.cols() < 3)
          throw Exception ("invalid diffusion gradient table dimensions");
        for (ssize_t i = 0; i < grad.rows(); i++) {
          auto norm = grad.row(i).template head<3>().norm();
          if (norm) 
            grad.row(i).template head<3>().array() /= norm;
        }
        return grad;
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
        g = Math::SH::cartesian2spherical (dirs).leftCols(2);

      auto v = Eigen::JacobiSVD<Eigen::MatrixXd> (Math::SH::init_transform (g, lmax)).singularValues();
      return v[0] / v[v.size()-1];
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



    //! scale b-values by square of gradient norm 
    template <class MatrixType>
      void scale_bvalue_by_G_squared (MatrixType& G) 
      {
        INFO ("b-values will be scaled by the square of DW gradient norm");
        for (ssize_t n = 0; n < G.rows(); ++n) 
          if (G(n,3)) 
            G(n,3) *= G.row(n).template head<3>().squaredNorm();
      }




    //! store the DW gradient encoding matrix in a header
    /*! this will store the DW gradient encoding matrix into the
     * Header::keyval() structure of \a header, under the key 'dw_scheme'.  
     */
    template <class MatrixType> 
      void set_DW_scheme (Header& header, const MatrixType& G)
      {
        if (!G.rows()) {
          header.keyval().erase ("dw_scheme");
          return;
        }
        std::string dw_scheme;
        for (ssize_t row = 0; row < G.rows(); ++row) {
          std::string line;
          for (ssize_t col = 0; col < G.cols(); ++col) {
            line += str(G(row,col), 10);
            if (col < G.cols() - 1) line += ",";
          }
          add_line (dw_scheme, line);
        }
        if (dw_scheme.size()) 
          header.keyval()["dw_scheme"] = dw_scheme;
        else
          WARN ("attempt to add empty DW scheme to header - ignored");
      }



    //! parse the DW gradient encoding matrix from a header
    /*! extract the DW gradient encoding matrix stored in the \a header if one
     * is present. This is expected to be stored in the Header::keyval()
     * structure, under the key 'dw_scheme'.
     */
    Eigen::MatrixXd parse_DW_scheme (const Header& header);



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
      set_DW_scheme (header, grad);
      auto dw_scheme = header.keyval().find ("dw_scheme");
      header.keyval()["prior_dw_scheme"] = dw_scheme->second;
      header.keyval().erase (dw_scheme);
    }




    //! get the DW gradient encoding matrix
    /*! attempts to find the DW gradient encoding matrix, using the following
     * procedure: 
     * - if the -grad option has been supplied, then load the matrix assuming
     *     it is in MRtrix format, and return it;
     * - if the -fslgrad option has been supplied, then load and rectify the
     *     bvecs/bvals pair using load_bvecs_bvals() and return it;
     * - if the DW_scheme member of the header is non-empty, return it;
     * - if no source of gradient encoding is found, return an empty matrix.
     */
    Eigen::MatrixXd get_DW_scheme (const Header& header);


    //! check that the DW scheme matches the DWI data in \a header
    template <class MatrixType> 
      inline void check_DW_scheme (const Header& header, const MatrixType& grad)
      {
        if (!grad.rows())
          throw Exception ("no valid diffusion gradient table found");

        if (header.ndim() != 4)
          throw Exception ("dwi image should contain 4 dimensions");

        if (header.size (3) != (int) grad.rows())
          throw Exception ("number of studies in base image does not match that in diffusion gradient table");
      }


    //! process GradExportOptions command-line options
    /*! this checks for the \c -export_grad_mrtrix & \c -export_grad_fsl
     * options, and exports the DW schemes if and as requested. */
    void export_grad_commandline (const Header& header);



    /*! \brief get the DW encoding matrix as per get_DW_scheme(), and
     * check that it matches the DW header in \a header 
     *
     * This is the version that should be used in any application that
     * processes the DWI raw data. */
    Eigen::MatrixXd get_valid_DW_scheme (const Header& header, bool nofail = false);


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
          auto v = Eigen::JacobiSVD<Eigen::MatrixXd> (mapping).singularValues();
          auto cond = v[0] / v[v.size()-1];
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

