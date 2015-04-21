/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 27/06/08.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */

#ifndef __dwi_gradient_h__
#define __dwi_gradient_h__

#include "app.h"
#include "point.h"
#include "file/path.h"
#include "file/config.h"
#include "header.h"
#include "math/LU.h"
#include "math/SH.h"
#include "math/matrix.h"
#include "math/permutation.h"
#include "dwi/shells.h"


namespace MR
{
  namespace App { class OptionGroup; }

  namespace DWI
  {
    App::OptionGroup GradImportOptions (bool include_bvalue_scaling = true);
    App::OptionGroup GradExportOptions();


    //! ensure each non-b=0 gradient vector is normalised to unit amplitude
    template <typename ValueType> 
      Math::Matrix<ValueType>& normalise_grad (Math::Matrix<ValueType>& grad)
      {
        if (grad.columns() < 3)
          throw Exception ("invalid gradient matrix dimensions");
        for (size_t i = 0; i < grad.rows(); i++) {
          ValueType norm = Math::norm (grad.row (i).sub (0,3));
          if (norm) 
            grad.row (i).sub (0,3) /= norm;
        }
        return grad;
      }


    /*! \brief convert the DW encoding matrix in \a grad into a
     * azimuth/elevation direction set, using only the DWI volumes as per \a
     * dwi */
    template <typename ValueType> 
      inline Math::Matrix<ValueType> gen_direction_matrix (
          const Math::Matrix<ValueType>& grad, 
          const std::vector<size_t>& dwi)
      {
        Math::Matrix<ValueType> dirs (dwi.size(),2);
        for (size_t i = 0; i < dwi.size(); i++) {
          dirs (i,0) = std::atan2 (grad (dwi[i],1), grad (dwi[i],0));
          ValueType z = grad (dwi[i],2) / Math::norm (grad.row (dwi[i]).sub (0,3));
          if (z >= 1.0) 
            dirs(i,1) = 0.0;
          else if (z <= -1.0)
            dirs (i,1) = Math::pi;
          else 
            dirs (i,1) = std::acos (z);
        }
        return dirs;
      }

    template <typename ValueType>
      ValueType condition_number_for_lmax (const Math::Matrix<ValueType>& dirs, int lmax) 
      {
        Math::Matrix<double> g;
        if (dirs.columns() == 2) // spherical coordinates:
          g = dirs;
        else // Cartesian to spherical:
          g = Math::SH::C2S(dirs).sub(0, dirs.rows(), 0, 2);

        Math::Matrix<double> A;
        Math::SH::init_transform (A, g, lmax);

        return Math::cond (A);
      }



    //! load and rectify FSL-style bvecs/bvals DW encoding files
    /*! This will load the bvecs/bvals files at the path specified, and convert
     * them to the format expected by MRtrix.  This involves rotating the
     * vectors into the scanner frame of reference, and may also involve
     * re-ordering and/or inverting of the vector elements to match the
     * re-ordering performed by MRtrix for non-axial scans. */
    template <typename ValueType> 
      void load_bvecs_bvals (Math::Matrix<ValueType>& grad, const Header& header, const std::string& bvecs_path, const std::string& bvals_path)
      {
        Math::Matrix<ValueType> bvals, bvecs;
        bvals.load (bvals_path);
        bvecs.load (bvecs_path);

        if (bvals.rows() != 1) throw Exception ("bvals file must contain 1 row only");
        if (bvecs.rows() != 3) throw Exception ("bvecs file must contain exactly 3 rows");

        if (bvals.columns() != bvecs.columns() || bvals.columns() != size_t (header.size (3)))
          throw Exception ("bvals and bvecs files must have same number of diffusion directions as DW-image");

        // account for the fact that bvecs are specified wrt original image axes,
        // which may have been re-ordered and/or inverted by MRtrix to match the
        // expected anatomical frame of reference:
        std::vector<size_t> order = Stride::order (header, 0, 3);
        Math::Matrix<ValueType> G (bvecs.columns(), 3);
        for (size_t n = 0; n < G.rows(); ++n) {
          G(n,order[0]) = header.stride(order[0]) > 0 ? bvecs(0,n) : -bvecs(0,n);
          G(n,order[1]) = header.stride(order[1]) > 0 ? bvecs(1,n) : -bvecs(1,n);
          G(n,order[2]) = header.stride(order[2]) > 0 ? bvecs(2,n) : -bvecs(2,n);
        }

        // rotate gradients into scanner coordinate system:
        grad.allocate (G.rows(), 4);
        Math::Matrix<ValueType> grad_G = grad.sub (0, grad.rows(), 0, 3);
        Math::Matrix<ValueType> rotation = header.transform().sub (0,3,0,3);
        Math::mult (grad_G, ValueType(0.0), ValueType(1.0), CblasNoTrans, G, CblasTrans, rotation);

        grad.column(3) = bvals.row(0);
      }



    //! export gradient table in FSL format (bvecs/bvals)
    /*! This will take the gradient table information from a header and export it
     * to a bvecs/bvals file pair. In addition to splitting the information over two
     * files, the vectors must be reoriented; firstly to change from scanner space to
     * image space, and then to compensate for the fact that FSL defines its vectors
     * with regards to the data strides in the image file.
     */
    void save_bvecs_bvals (const Header&, const std::string&, const std::string&);



    //! scale b-values by square of gradient norm 
    template <typename ValueType>
      void scale_bvalue_by_G_squared (Math::Matrix<ValueType>& G) 
      {
        INFO ("b-values will be scaled by the square of DW gradient norm");
        for (size_t n = 0; n < G.rows(); ++n) {
          if (G(n,3)) {
            float norm = Math::norm (G.row(n).sub(0,3));
            G(n,3) *= norm*norm;
          }
        }
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
    template <typename ValueType> 
      Math::Matrix<ValueType> get_DW_scheme (const Header& header)
      {
        DEBUG ("searching for suitable gradient encoding...");
        using namespace App;
        Math::Matrix<ValueType> grad;

        try {
          const Options opt_mrtrix = get_options ("grad");
          if (opt_mrtrix.size())
            grad.load (opt_mrtrix[0][0]);
          const Options opt_fsl = get_options ("fslgrad");
          if (opt_fsl.size()) {
            if (opt_mrtrix.size())
              throw Exception ("Please provide diffusion encoding using either -grad or -fslgrad option (not both)");
            load_bvecs_bvals (grad, header, opt_fsl[0][0], opt_fsl[0][1]);
          }
          if (!opt_mrtrix.size() && !opt_fsl.size())
            grad = header.parse_DW_scheme<ValueType>();
        }
        catch (Exception& E) {
          E.display (3);
          throw Exception ("error importing diffusion encoding for image \"" + header.name() + "\"");
        }

        if (!grad.rows())
          return grad;

        if (grad.columns() < 4)
          throw Exception ("unexpected diffusion encoding matrix dimensions");

        INFO ("found " + str (grad.rows()) + "x" + str (grad.columns()) + " diffusion-weighted encoding");

        return grad;
      }


    //! check that the DW scheme matches the DWI data in \a header
    template <typename ValueType> 
      inline void check_DW_scheme (const Header& header, const Math::Matrix<ValueType>& grad)
      {
        if (!grad.rows())
          throw Exception ("no valid diffusion encoding scheme found");

        if (header.ndim() != 4)
          throw Exception ("dwi image should contain 4 dimensions");

        if (header.size (3) != (int) grad.rows())
          throw Exception ("number of studies in base image does not match that in encoding file");
      }


    //! process GradExportOptions command-line options
    /*! this checks for the \c -export_grad_mrtrix & \c -export_grad_fsl
     * options, and exports the DW schemes if and as requested. */
    void export_grad_commandline (const Header& header);


    //CONF option: BValueScaling
    //CONF default: yes
    //CONF specifies whether b-values should be scaled according the DW gradient
    //CONF amplitudes - see the -bvalue_scaling option for details.


    /*! \brief get the DW encoding matrix as per get_DW_scheme(), and
     * check that it matches the DW header in \a header 
     *
     * This is the version that should be used in any application that
     * processes the DWI raw data. */
    template <typename ValueType> 
      inline Math::Matrix<ValueType> get_valid_DW_scheme (const Header& header)
      {
        Math::Matrix<ValueType> grad = get_DW_scheme<ValueType> (header);
        check_DW_scheme (header, grad);

        //CONF option: BValueScaling
        //CONF default: 1 (true)
        //CONF specifies whether the b-values should be scaled by the squared
        //CONF norm of the gradient vectors when loading a DW gradient scheme. 
        //CONF This is commonly required to correctly interpret images acquired
        //CONF on scanners that nominally only allow a single b-value, as the
        //CONF common workaround is to scale the gradient vectors to modulate
        //CONF the actual b-value. 
        bool scale_bvalues = true;
        App::Options opt = App::get_options ("bvalue_scaling");
        if (opt.size()) 
          scale_bvalues = opt[0][0];
        else
          scale_bvalues = File::Config::get_bool ("BValueScaling", scale_bvalues);

        if (scale_bvalues)
          scale_bvalue_by_G_squared (grad);
        normalise_grad (grad);
        return grad;
      }


    //! \brief get the matrix mapping SH coefficients to amplitudes
    /*! Computes the matrix mapping SH coefficients to the directions specified
     * in \a directions (in spherical coordinates), up to a given lmax. By
     * default, this is computed from the number of DW directions, up to a
     * maximum value of \a default_lmax (defaults to 8), or the value specified
     * using c -lmax command-line option (if \a lmax_from_command_line is
     * true). If the resulting DW scheme is ill-posed (condition number less
     * than \a max_cond, default 2), lmax will be reduced until it becomes
     * sufficiently well conditioned (unless overridden on the command-line).
     *
     * Note that this uses get_valid_DW_scheme() to get the DW_scheme, so will
     * check for the -grad option as required. */
    template <typename ValueType>
      inline Math::Matrix<ValueType> compute_SH2amp_mapping (
          const Math::Matrix<ValueType>& directions,
          bool lmax_from_command_line = true, 
          int default_lmax = 8, 
          ValueType max_cond = 2.0)
      {
        int lmax = -1;
        int lmax_from_ndir = Math::SH::LforN (directions.rows());
        bool lmax_set_from_commandline = false;
        if (lmax_from_command_line) {
          App::Options opt = App::get_options ("lmax");
          if (opt.size()) {
            lmax_set_from_commandline = true;
            lmax = to<int> (opt[0][0]);
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
        Math::Matrix<ValueType> mapping;
        do {
          Math::SH::init_transform (mapping, directions, lmax);
          double cond = Math::cond (mapping);
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


  }
}

#endif

