/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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
      return (grad);
    }


    /*! \brief find which volumes correspond to b=0 and which to DWIs, assuming
     * a simple threshold on the b-value 
     *
     * By default, any volume with a b-value <= 10 is considered a b=0. This
     * can be modified using the argument \a bvalue_threshold, or by specifying
     * the desired value in the configuration file, under the key
     * "BValueThreshold". */
    template <typename ValueType> 
      inline void guess_DW_directions (
          std::vector<int>& dwi, 
          std::vector<int>& bzero, 
          const Math::Matrix<ValueType>& grad,
          ValueType bvalue_threshold = NAN)
    {
      if (!std::isfinite (bvalue_threshold))
        bvalue_threshold = File::Config::get_float ("BValueThreshold", 10.0);
      if (grad.columns() != 4)
        throw Exception ("invalid gradient encoding matrix: expecting 4 columns.");
      Shells<ValueType> shells(grad);
      int shell_count = shells.count();
      if (shell_count < 1 || shell_count > sqrt(grad.rows()))
        throw Exception ("Gradient encoding matrix does not represent a HARDI sequence!");
      INFO ("found " + str (shell_count) + " shells");
      Shell<ValueType> bzeroShell;
      Shell<ValueType> dwiShell;
      if (shell_count>1)
        bzeroShell = shells.first();
      dwiShell = shells.last();
      if (shell_count>1)
        INFO ("using " + str (bzeroShell.count()) + " volumes with b-value " + str (bzeroShell.avg_bval()) + " +/-" + str (bzeroShell.std_bval()) + " as b=0 volumes");
      INFO ("using " + str (dwiShell.count()) + " volumes with b-value " + str (dwiShell.avg_bval()) + " +/-" + str (dwiShell.std_bval()) + " as diffusion-weighted volumes");
      bzero = bzeroShell.idx();
      dwi = dwiShell.idx();
    }



    /*! \brief convert the DW encoding matrix in \a grad into a
     * azimuth/elevation direction set, using only the DWI volumes as per \a
     * dwi */
    template <typename ValueType> 
      inline Math::Matrix<ValueType>& gen_direction_matrix (
          Math::Matrix<ValueType>& dirs, 
          const Math::Matrix<ValueType>& grad, 
          const std::vector<int>& dwi)
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





    //! locate, load and rectify FSL-style bvecs/bvals DW encoding files
    /*! This will first look for files names 'bvecs' & 'bvals' in the same
     * folder as the image, assuming its path is stored in header.name(). If
     * not found, then it will look for files with the same prefix as the
     * image and the '_bvecs' & '_bvals' extension. 
     *
     * Once loaded, these are then converted to the format expected by MRtrix.
     * This involves rotating the vectors into the scanner frame of reference,
     * and may also involve re-ordering and/or inverting of the vector elements
     * to match the re-ordering performed by MRtrix for non-axial scans. */
    template <typename ValueType> 
      void load_bvecs_bvals (Math::Matrix<ValueType>& grad, const Image::Header& header)
    {
      std::string dir_path = Path::dirname (header.name());
      std::string bvals_path = Path::join (dir_path, "bvals");
      std::string bvecs_path = Path::join (dir_path, "bvecs");
      bool found_bvals = Path::is_file (bvals_path);
      bool found_bvecs = Path::is_file (bvecs_path);

      if (!found_bvals && !found_bvecs) {
        const std::string prefix = header.name().substr (0, header.name().find_last_of ('.'));
        bvals_path = prefix + "_bvals";
        bvecs_path = prefix + "_bvecs";
        found_bvals = Path::is_file (bvals_path);
        found_bvecs = Path::is_file (bvecs_path);
      }

      if (found_bvals && !found_bvecs)
        throw Exception ("found bvals file but not bvecs file");
      else if (!found_bvals && found_bvecs)
        throw Exception ("found bvecs file but not bvals file");
      else if (!found_bvals && !found_bvecs)
        throw Exception ("could not find either bvecs or bvals gradient files");

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



    //! get the DW gradient encoding matrix
    /*! attempts to find the DW gradient encoding matrix, using the following
     * procedure: 
     * - if the -grad option has been supplied, then:
     *   - if the file supplied the -grad option ends with 'bvals' or 'bvecs',
     *   then load and rectify the bvecs/bvals pair using load_bvecs_bvals()
     *   and return it; 
     *   - otherwise load the matrix assuming it is in MRtrix
     *   format, and return it;
     * - if the DW_scheme member of the header is non-empty, return it;
     * - otherwise, if a bvecs/bvals pair can be found in the same folder as
     * the image, named either 'bvecs' & 'bvals', or with the same prefix as
     * the image and the '_bvecs' & '_bvals' extension, then load and rectify
     * that and return it.  */
    template <typename ValueType> 
      Math::Matrix<ValueType> get_DW_scheme (const Image::Header& header)
      {
        DEBUG ("searching for suitable gradient encoding...");
        using namespace App;
        Math::Matrix<ValueType> grad;

        Options opt = get_options ("grad");
        if (opt.size()) {
          if (Path::has_suffix (opt[0][0], "bvals") || 
              Path::has_suffix (opt[0][0], "bvecs"))
            load_bvecs_bvals (grad, header);
          else
            grad.load (opt[0][0]);
        }
        else if (header.DW_scheme().is_set()) {
          grad = header.DW_scheme();
        }
        else {
          try {
            load_bvecs_bvals (grad, header);
          } 
          catch (Exception& E) {
            E.display (3);
            throw Exception ("no diffusion encoding found in image \"" + header.name() + "\" or corresponding directory");
          }
        }

        if (grad.columns() != 4)
          throw Exception ("unexpected diffusion encoding matrix dimensions");

        INFO ("found " + str (grad.rows()) + "x" + str (grad.columns()) + " diffusion-weighted encoding");

        DWI::normalise_grad (grad);

        return grad;
      }


    //! check that the DW scheme matches the DWI data in \a header
    template <typename ValueType> 
      inline void check_DW_scheme (const Image::Header& header, const Math::Matrix<ValueType>& grad)
      {
        if (header.ndim() != 4)
          throw Exception ("dwi image should contain 4 dimensions");

        if (header.dim (3) != (int) grad.rows())
          throw Exception ("number of studies in base image does not match that in encoding file");
      }




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
          std::vector<int>& dwis, 
          std::vector<int>& bzeros, 
          bool lmax_from_command_line = true, 
          int lmax = -1, 
          int max_lmax = 8, 
          ValueType bvalue_threshold = NAN)
      {
        grad = get_valid_DW_scheme<ValueType> (header);
        normalise_grad (grad);

        guess_DW_directions (dwis, bzeros, grad, bvalue_threshold);

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
        std::vector<int> dwis, bzeros;
        Math::Matrix<ValueType> grad, directions;
        return get_SH2amp_mapping (header, grad, directions, dwis, bzeros, lmax_from_command_line, lmax, max_lmax, bvalue_threshold);
      }


  }
}

#endif

