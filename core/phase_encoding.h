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

#ifndef __phaseencoding_h__
#define __phaseencoding_h__


#include <Eigen/Dense>

#include "app.h"
#include "axes.h"
#include "header.h"
#include "file/nifti_utils.h"
#include "file/ofstream.h"



namespace MR
{
  namespace PhaseEncoding
  {



    extern const App::OptionGroup ImportOptions;
    extern const App::OptionGroup SelectOptions;
    extern const App::OptionGroup ExportOptions;



    //! check that a phase-encoding table is valid
    template <class MatrixType>
    void check (const MatrixType& PE)
    {
      if (!PE.rows())
        throw Exception ("No valid phase encoding table found");

      if (PE.cols() < 3)
        throw Exception ("Phase-encoding matrix must have at least 3 columns");

      for (ssize_t row = 0; row != PE.rows(); ++row) {
        for (ssize_t axis = 0; axis != 3; ++axis) {
          if (std::round (PE(row, axis)) != PE(row, axis))
            throw Exception ("Phase-encoding matrix contains non-integral axis designation");
        }
      }
    }



    //! check that the PE scheme matches the DWI data in \a header
    template <class MatrixType, class HeaderType>
    void check (const MatrixType& PE, const HeaderType& header)
    {
      check (PE);
      const ssize_t num_volumes = (header.ndim() > 3) ? header.size (3) : 1;
      if (num_volumes != PE.rows())
        throw Exception ("Number of volumes in image \"" + header.name() + "\" (" + str(num_volumes) + ") does not match that in phase encoding table (" + str(PE.rows()) + ")");
    }







    //! store the phase encoding matrix in a header
    /*! this will store the phase encoding matrix into the
       *  Header::keyval() structure of \a header.
       *  - If the phase encoding direction and/or total readout
       *    time varies between volumes, then the information
       *    will be stored in field "pe_scheme"; if not, it
       *    will instead be stored in fields "PhaseEncodingDirection"
       *    and "TotalReadoutTime"
       */
    template <class MatrixType>
    void set_scheme (Header& header, const MatrixType& PE)
    {
      auto erase = [&] (const std::string& s) { auto it = header.keyval().find (s); if (it != header.keyval().end()) header.keyval().erase (it); };
      if (!PE.rows()) {
        erase ("pe_scheme");
        erase ("PhaseEncodingDirection");
        erase ("TotalReadoutTime");
        return;
      }
      check (PE, header);
      std::string pe_scheme;
      std::string first_line;
      bool variation = false;
      for (ssize_t row = 0; row < PE.rows(); ++row) {
        std::string line = str(PE(row,0));
        for (ssize_t col = 1; col < PE.cols(); ++col)
          line += "," + str(PE(row,col), 3);
        add_line (pe_scheme, line);
        if (first_line.empty())
          first_line = line;
        else if (line != first_line)
          variation = true;
      }
      if (variation) {
        header.keyval()["pe_scheme"] = pe_scheme;
        erase ("PhaseEncodingDirection");
        erase ("TotalReadoutTime");
      } else {
        erase ("pe_scheme");
        const Eigen::Vector3d dir { PE(0, 0), PE(0, 1), PE(0, 2) };
        header.keyval()["PhaseEncodingDirection"] = Axes::dir2id (dir);
        if (PE.cols() >= 4)
          header.keyval()["TotalReadoutTime"] = str(PE(0, 3), 3);
        else
          erase ("TotalReadoutTime");
      }
    }



    //! clear the phase encoding matrix from a header
    /*! this will delete any trace of phase encoding information
     *  from the Header::keyval() structure of \a header.
     */
    void clear_scheme (Header& header);



    //! parse the phase encoding matrix from a header
    /*! extract the phase encoding matrix stored in the \a header if one
     *  is present. This is expected to be stored in the Header::keyval()
     *  structure, under the key 'pe_scheme'. Alternatively, if the phase
     *  encoding direction and bandwidth is fixed for all volumes in the
     *  series, this information may be stored using the keys
     *  'PhaseEncodingDirection' and 'TotalReadoutTime'.
     */
    Eigen::MatrixXd parse_scheme (const Header&);



    //! get a phase encoding matrix
    /*! get a valid phase-encoding matrix, either from files specified at
     *  the command-line, or from the contents of the image header.
     */
    Eigen::MatrixXd get_scheme (const Header&);



    //! Convert a phase-encoding scheme into the EDDY config / indices format
    template <class MatrixType>
    void scheme2eddy (const MatrixType& PE, Eigen::MatrixXd& config, Eigen::Array<int, Eigen::Dynamic, 1>& indices)
    {
      try {
        check (PE);
      } catch (Exception& e) {
        throw Exception (e, "Cannot convert phase-encoding scheme to eddy format");
      }
      if (PE.cols() != 4)
        throw Exception ("Phase-encoding matrix requires 4 columns to convert to eddy format");
      config.resize (0, 0);
      indices = Eigen::Array<int, Eigen::Dynamic, 1>::Constant (PE.rows(), PE.rows());
      for (ssize_t PE_row = 0; PE_row != PE.rows(); ++PE_row) {
        for (ssize_t config_row = 0; config_row != config.rows(); ++config_row) {
          bool dir_match = PE.template block<1,3>(PE_row, 0).isApprox (config.block<1,3>(config_row, 0));
          bool time_match = abs (PE(PE_row, 3) - config(config_row, 3)) < 1e-3;
          if (dir_match && time_match) {
            // FSL-style index file indexes from 1
            indices[PE_row] = config_row + 1;
            break;
          }
        }
        if (indices[PE_row] == PE.rows()) {
          // No corresponding match found in config matrix; create a new entry
          config.conservativeResize (config.rows()+1, 4);
          config.row(config.rows()-1) = PE.row(PE_row);
          indices[PE_row] = config.rows();
        }
      }
    }

    //! Convert phase-encoding infor from the EDDY config / indices format into a standard scheme
    Eigen::MatrixXd eddy2scheme (const Eigen::MatrixXd&, const Eigen::Array<int, Eigen::Dynamic, 1>&);





    //! Modifies a phase encoding scheme if being imported alongside a non-RAS image
    template <class MatrixType, class HeaderType>
    Eigen::MatrixXd transform_for_image_load (const MatrixType& pe_scheme, const HeaderType& H)
    {
      std::array<size_t, 3> perm;
      std::array<bool, 3> flip;
      H.realignment (perm, flip);
      if (perm[0] == 0 && perm[1] == 1 && perm[2] == 2 && !flip[0] && !flip[1] && !flip[2]) {
        INFO ("No transformation of external phase encoding data required to accompany image \"" + H.name() + "\"");
        return pe_scheme;
      }
      Eigen::MatrixXd result (pe_scheme.rows(), pe_scheme.cols());
      for (ssize_t row = 0; row != pe_scheme.rows(); ++row) {
        Eigen::VectorXd new_line = pe_scheme.row (row);
        for (ssize_t axis = 0; axis != 3; ++axis) {
          new_line[axis] = pe_scheme(row, perm[axis]);
          if (new_line[axis] && flip[perm[axis]])
            new_line[axis] = -new_line[axis];
        }
        result.row (row) = new_line;
      }
      INFO ("External phase encoding data transformed to match RAS realignment of image \"" + H.name() + "\"");
      return result;
    }

    
    

    //! Modifies a phase encoding scheme if being exported alongside a NIfTI image
    template <class MatrixType, class HeaderType>
    Eigen::MatrixXd transform_for_nifti_write (const MatrixType& pe_scheme, const HeaderType& H)
    {
      vector<size_t> order;
      vector<bool> flip;
      File::NIfTI::axes_on_write (H, order, flip);
      if (order[0] == 0 && order[1] == 1 && order[2] == 2 && !flip[0] && !flip[1] && !flip[2]) {
        INFO ("No transformation of phase encoding data required for export to file");
        return pe_scheme;
      }
      Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> result (pe_scheme.rows(), pe_scheme.cols());
      for (ssize_t row = 0; row != pe_scheme.rows(); ++row) {
        Eigen::VectorXd new_line = pe_scheme.row (row);
        for (ssize_t axis = 0; axis != 3; ++axis)
          new_line[axis] = pe_scheme(row, order[axis]) && flip[axis] ?
                           -pe_scheme(row, order[axis]) :
                           pe_scheme(row, order[axis]);
        result.row (row) = new_line;
      }
      INFO ("Phase encoding data transformed to match NIfTI / MGH image export prior to writing to file");
      return result;
    }




    namespace
    {
      template <class MatrixType>
      void __save (const MatrixType& PE, const std::string& path)
      {
        File::OFStream out (path);
        for (ssize_t row = 0; row != PE.rows(); ++row) {
          // Write phase-encode direction as integers; other information as floating-point
          out << PE.template block<1, 3>(row, 0).template cast<int>();
          if (PE.cols() > 3)
            out << " " << PE.block(row, 3, 1, PE.cols()-3);
          out << "\n";
        }
      }
    }

    //! Save a phase-encoding scheme to file
    // Note that because the output table requires permutation / sign flipping
    //   only if the output target image is a NIfTI, the output file name must have
    //   already been set
    template <class MatrixType, class HeaderType>
    void save (const MatrixType& PE, const HeaderType& header, const std::string& path)
    {
      try {
        check (PE, header);
      } catch (Exception& e) {
        throw Exception (e, "Cannot export phase-encoding table to file \"" + path + "\"");
      }

      if (Path::has_suffix (header.name(), {".mgh", ".mgz", ".nii", ".nii.gz", ".img"})) {
        __save (transform_for_nifti_write (PE, header), path);
      } else {
        __save (PE, path);
      }
    }

    //! Save a phase-encoding scheme to EDDY format config / index files
    template <class MatrixType, class HeaderType>
    void save_eddy (const MatrixType& PE, const HeaderType& header, const std::string& config_path, const std::string& index_path)
    {
      Eigen::MatrixXd config;
      Eigen::Array<int, Eigen::Dynamic, 1> indices;
      scheme2eddy (transform_for_nifti_write (PE, header), config, indices);
      save_matrix (config, config_path, KeyValues(), false);
      save_vector (indices, index_path, KeyValues(), false);
    }



    //! Save the phase-encoding scheme from a header to file depending on command-line options
    void export_commandline (const Header&);



    //! Load a phase-encoding scheme from a matrix text file
    template <class HeaderType>
    Eigen::MatrixXd load (const std::string& path, const HeaderType& header)
    {
      const Eigen::MatrixXd PE = load_matrix (path);
      check (PE, header);
      // As with JSON import, need to query the header to discover if the 
      //   strides / transform were modified on image load to make the image
      //   data appear approximately axial, in which case we need to apply the
      //   same transforms to the phase encoding data on load
      return transform_for_image_load (PE, header);
    }

    //! Load a phase-encoding scheme from an EDDY-format config / indices file pair
    template <class HeaderType>
    Eigen::MatrixXd load_eddy (const std::string& config_path, const std::string& index_path, const HeaderType& header)
    {
      const Eigen::MatrixXd config = load_matrix (config_path);
      const Eigen::Array<int, Eigen::Dynamic, 1> indices = load_vector<int> (index_path);
      const Eigen::MatrixXd PE = eddy2scheme (config, indices);
      check (PE, header);
      return transform_for_image_load (PE, header);
    }



  }
}

#endif

