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
#include "image/header.h"
#include "image/stride.h"
#include "image/transform.h"
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
    extern const App::OptionGroup GradOption;


    //! ensure each non-b=0 gradient vector is normalised to unit amplitude
    template <typename ValueType> 
      Math::Matrix<ValueType>& normalise_grad (Math::Matrix<ValueType>& grad)
      {
        if (grad.columns() != 4)
          throw Exception ("invalid gradient matrix dimensions");
        for (size_t i = 0; i < grad.rows(); i++) {
          ValueType norm = Math::norm (grad.row (i).sub (0,3));
          if (norm) {
            grad.row (i).sub (0,3) /= norm;
          } else {
            grad (i,3) = 0;
          }
        }
        return grad;
      }


    /*! \brief convert the DW encoding matrix in \a grad into a
     * azimuth/elevation direction set, using only the DWI volumes as per \a
     * dwi */
    template <typename ValueType> 
      inline Math::Matrix<ValueType>& gen_direction_matrix (
          Math::Matrix<ValueType>& dirs, 
          const Math::Matrix<ValueType>& grad, 
          const std::vector<size_t>& dwi)
      {
        dirs.allocate (dwi.size(),2);
        for (size_t i = 0; i < dwi.size(); i++) {
          dirs (i,0) = Math::atan2 (grad (dwi[i],1), grad (dwi[i],0));
          ValueType z = grad (dwi[i],2) / Math::norm (grad.row (dwi[i]).sub (0,3));
          if (z >= 1.0) 
            dirs(i,1) = 0.0;
          else if (z <= -1.0)
            dirs (i,1) = M_PI;
          else 
            dirs (i,1) = Math::acos (z);
        }
        return dirs;
      }



    //! load and rectify FSL-style bvecs/bvals DW encoding files
    /*! This will load the bvecs/bvals files at the path specified, and convert
     * them to the format expected by MRtrix.  This involves rotating the
     * vectors into the scanner frame of reference, and may also involve
     * re-ordering and/or inverting of the vector elements to match the
     * re-ordering performed by MRtrix for non-axial scans. */
    template <typename ValueType> 
      void load_bvecs_bvals (Math::Matrix<ValueType>& grad, const Image::Header& header, const std::string& bvecs_path, const std::string& bvals_path)
      {
        Math::Matrix<ValueType> bvals, bvecs;
        bvals.load (bvals_path);
        bvecs.load (bvecs_path);

        if (bvals.rows() != 1) throw Exception ("bvals file must contain 1 row only");
        if (bvecs.rows() != 3) throw Exception ("bvecs file must contain exactly 3 rows");

        if (bvals.columns() != bvecs.columns() || bvals.columns() != size_t(header.dim (3)))
          throw Exception ("bvals and bvecs files must have same number of diffusion directions as DW-image");

        // account for the fact that bvecs are specified wrt original image axes,
        // which may have been re-ordered and/or inverted by MRtrix to match the
        // expected anatomical frame of reference:
        std::vector<size_t> order = Image::Stride::order (header, 0, 3);
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
    void save_bvecs_bvals (const Image::Header&, const std::string&);



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
      Math::Matrix<ValueType> get_DW_scheme (const Image::Header& header)
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
          if (!opt_mrtrix.size() && !opt_fsl.size() && header.DW_scheme().is_set())
            grad = header.DW_scheme();
        }
        catch (Exception& E) {
          E.display (3);
          throw Exception ("error importing diffusion encoding for image \"" + header.name() + "\"");
        }

        if (!grad.rows())
          return grad;

        if (grad.columns() != 4)
          throw Exception ("unexpected diffusion encoding matrix dimensions");

        INFO ("found " + str (grad.rows()) + "x" + str (grad.columns()) + " diffusion-weighted encoding");

        return grad;
      }


    //! check that the DW scheme matches the DWI data in \a header
    template <typename ValueType> 
      inline void check_DW_scheme (const Image::Header& header, const Math::Matrix<ValueType>& grad)
      {
        if (!grad.rows())
          throw Exception ("no valid diffusion encoding scheme found");

        if (header.ndim() != 4)
          throw Exception ("dwi image should contain 4 dimensions");

        if (header.dim (3) != (int) grad.rows())
          throw Exception ("number of studies in base image does not match that in encoding file");
      }



    //! internal function to determine b-value scaling mode
    int get_bvalue_scaling_mode();



    /*! \brief get the DW encoding matrix as per get_DW_scheme(), and
     * check that it matches the DW header in \a header 
     *
     * This is the version that should be used in any application that
     * processes the DWI raw data. */
    template <typename ValueType> 
      inline Math::Matrix<ValueType> get_valid_DW_scheme (const Image::Header& header)
      {
        Math::Matrix<ValueType> grad = get_DW_scheme<ValueType> (header);
        check_DW_scheme (header, grad);
        int bvalue_scaling_mode = get_bvalue_scaling_mode();
        if (bvalue_scaling_mode == 1) { // auto
          LogLevelLatch latch (0);
          bvalue_scaling_mode = !DWI::Shells (grad).is_single_shell();
          if (bvalue_scaling_mode) {
            INFO ("DW scheme is multi-shell - applying b-value scaling");
          } else {
            INFO ("DW scheme is single-shell - b-value scaling will NOT be applied");
          }
        }
        if (bvalue_scaling_mode)
          scale_bvalue_by_G_squared (grad);
        normalise_grad (grad);
        return grad;
      }


    //! \brief get the matrix mapping SH coefficients to directions
    /*! Computes the matrix mapping SH coefficients to the directions specified
     * in the DW_scheme of \a header, up to a given lmax. By default, this is
     * read from the -lmax command-line option (if \a lmax_from_command_line is
     * true), or otherwise computed from the number of DW directions in the
     * DW_scheme, up to a maximum value of \a max_lmax (defaults to 8). If \a
     * lmax is specified, this is the value used, unless overriden by the
     * command-line (if \a lmax_from_command_line is true).
     *
     * Note that this uses get_valid_DW_scheme() to get the DW_scheme, so will
     * check for the -grad option as required. */
    template <typename ValueType>
      inline Math::Matrix<ValueType> get_SH2amp_mapping (
          const Image::Header& header, 
          Math::Matrix<ValueType>& grad,
          Math::Matrix<ValueType>& directions,
          std::vector<size_t>& dwis, 
          std::vector<size_t>& bzeros, 
          bool lmax_from_command_line = true, 
          int lmax = -1, 
          int max_lmax = 8, 
          ValueType bvalue_threshold = NAN)
      {
        grad = get_valid_DW_scheme<ValueType> (header);

        DWI::Shells shells (grad);
        shells.select_shells (true, true);
        if (shells.smallest().is_bzero())
          bzeros = shells.smallest().get_volumes();
        dwis = shells.largest().get_volumes();

        if (lmax_from_command_line) {
          App::Options opt = App::get_options ("lmax");
          if (opt.size()) {
            lmax = to<int> (opt[0][0]);
            int lmax_from_DW = Math::SH::LforN (dwis.size());
            if (lmax > lmax_from_DW) {
              WARN ("not enough directions in DW scheme for lmax = " + str(lmax) + " - dropping down to " + str(lmax_from_DW));
              lmax = lmax_from_DW;
            }
          }
        }

        if (lmax < 0) {
          lmax = Math::SH::LforN (dwis.size());
          if (lmax > max_lmax)
            lmax = max_lmax;
        }

        INFO ("computing SH transform using lmax = " + str (lmax));

        gen_direction_matrix (directions, grad, dwis);

        Math::Matrix<ValueType> SH;
        Math::SH::init_transform (SH, directions, lmax);

        return SH;
      }


    template <typename ValueType>
      inline Math::Matrix<ValueType> get_SH2amp_mapping (
          const Image::Header& header, 
          bool lmax_from_command_line = true, 
          int lmax = -1, 
          int max_lmax = 8, 
          ValueType bvalue_threshold = NAN)
      {
        std::vector<size_t> dwis, bzeros;
        Math::Matrix<ValueType> grad, directions;
        return get_SH2amp_mapping (header, grad, directions, dwis, bzeros, lmax_from_command_line, lmax, max_lmax, bvalue_threshold);
      }


  }
}

#endif

