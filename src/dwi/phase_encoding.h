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

#ifndef __dwi_phaseencoding_h__
#define __dwi_phaseencoding_h__


#include "header.h"
#include "file/ofstream.h"


namespace MR
{
  namespace DWI
  {
    namespace PhaseEncoding
    {



      //! convert phase encoding direction between formats
      /*! these helper functions convert the definition of
       *  phase-encoding direction between a 3-vector (e.g.
       *  [0 1 0] ) and a NIfTI axis identifier (e.g. 'i-')
       */
      std::string    dir2id (const Eigen::Vector3&);
      Eigen::Vector3 id2dir (const std::string&);



      //! store the phase encoding matrix in a header
      /*! this will store the phase encoding matrix into the
       *  Header::keyval() structure of \a header.
       *  - If the phase encoding direction and/or total readout
       *    time varies between volumes, then the information
       *    will be stored
       */
      template <class MatrixType>
      void set_scheme (Header& header, const MatrixType& PE)
      {
        if (!PE.rows()) {
          header.keyval().erase ("pe_scheme");
          header.keyval().erase ("PhaseEncodingDirection");
          header.keyval().erase ("TotalReadoutTime");
          return;
        }
        check_scheme (header, PE);
        std::string pe_scheme;
        std::string first_line;
        bool variation = false;
        for (ssize_t row = 0; row < PE.rows(); ++row) {
          std::string line;
          for (ssize_t col = 0; col < PE.cols(); ++col) {
            line += str(PE(row,col), 3);
            if (col < PE.cols() - 1) line += ",";
          }
          add_line (pe_scheme, line);
          if (first_line.empty())
            first_line = line;
          else if (line != first_line)
            variation = true;
        }
        if (variation) {
          header.keyval()["pe_scheme"] = pe_scheme;
          header.keyval().erase ("PhaseEncodingDirection");
          header.keyval().erase ("TotalReadoutTime");
        } else {
          header.keyval().erase ("pe_scheme");
          header.keyval()["PhaseEncodingDirection"] = dir2id (PE.block<3, 1>(0, 0));
          if (PE.cols() >= 4)
            header.keyval()["TotalReadoutTime"] = PE(0, 3);
          else
            header.keyval().erase ("TotalReadoutTime");
        }
      }



      //! parse the phase encoding matrix from a header
      /*! extract the phase encoding matrix stored in the \a header if one
       *  is present. This is expected to be stored in the Header::keyval()
       *  structure, under the key 'pe_scheme'.
       */
      Eigen::MatrixXd get_scheme (const Header& header);



      //! check that a phase-encoding table is valid
      template <class MatrixType>
      inline void check (const MatrixType& PE)
      {
        if (!PE.rows())
          throw Exception ("No valid phase encoding table found");

        if (PE.cols() < 3)
          throw Exception ("Phase-encoding matrix must have at least 3 columns");

        for (size_t row = 0; row != PE.rows(); ++row) {
          for (size_t axis = 0; axis != 3; ++axis) {
            if (std::round (PE(row, axis)) != PE(row, axis))
              throw Exception ("Phase-encoding matrix contains non-integral axis designation");
          }
        }
      }



      //! check that the PE scheme matches the DWI data in \a header
      template <class MatrixType>
      inline void check (const Header& header, const MatrixType& PE)
      {
        check (PE);

        if (((header.ndim() > 3) ? header.size (3) : 1) != (int) PE.rows())
          throw Exception ("Number of volumes in image \"" + header.name() + "\" does not match that in phase encoding table");
      }



      //! Convert a phase-encoding scheme into the EDDY config / indices format
      template <class MatrixType>
      void scheme2eddy (const MatrixType& PE, Eigen::MatrixXd& config, Eigen::Array<int, Eigen::Dynamic, 1>& indices)
      {
        try {
          check_scheme (PE);
        } catch (Exception& e) {
          throw Exception (e, "Cannot convert phase-encoding scheme to eddy format");
        }
        if (PE.cols() != 4)
          throw Exception ("Phase-encode matrix requires 4 columns to convert to eddy format");
        config.resize (0, 0);
        indices.resize (PE.rows());
        for (size_t PE_row = 0; PE_row != PE.rows(); ++PE_row) {

          for (size_t config_row = 0; config_row != config.rows(); ++config_row) {
            if (PE.block<1,3>(PE_row, 0) == config.block<1,3>(config_row, 0)
                && ((std::abs(PE(PE_row, 3) - config(config_row, 3))) / (PE(PE_row, 3) + config(config_row, 3)) < 1e-3)) {

              // FSL-style index file indexes from 1
              indices[PE_row] = config_row + 1;
              continue;

            }
          }
          // No corresponding match found in config matrix; create a new entry
          config.conservativeResize (config.rows()+1, 4);
          config.row(config.rows()-1) = PE.row(PE_row);
          indices[PE_row] = config.rows();

        }
      }

      //! Convert phase-encoding informat from the EDDY config / indices format into a standard scheme
      Eigen::MatrixXd eddy2scheme (const Eigen::MatrixXd&, const Eigen::Array<int, Eigen::Dynamic, 1>&);



      //! Save a phase-encoding scheme to file
      template <class MatrixType>
      void save (const MatrixType& PE, const std::string& path)
      {
        try {
          check_scheme (PE);
        } catch (Exception& e) {
          throw Exception (e, "Cannot export phase-encoding table to file \"" + path + "\"");
        }

        File::OFStream out (path);
        for (size_t row = 0; row != PE.rows(); ++row) {
          // Write phase-encode direction as integers; other information as floating-point
          out << PE.block<1, 3>(row, 0).cast<int>();
          if (PE.cols() > 3)
            out << " " << PE.block(row, 0, 1, PE.cols()-3);
          out << "\n";
        }
      }

      //! Save a phase-encoding scheme to EDDY format config / index files
      template <class MatrixType>
      void save_eddy (const MatrixType& PE, const std::string& config_path, const std::string& index_path)
      {
        Eigen::MatrixXd config;
        Eigen::Array<int, Eigen::Dynamic, 1> indices;
        scheme2eddy (PE, config, indices);
        save_matrix (config, config_path);
        save_vector (indices, index_path);
      }



      //! Load a phase-encoding scheme from either a matrix file, or an EDDY-format comfig / indices file pair
      Eigen::MatrixXd load (const std::string&);
      Eigen::MatrixXd load_eddy (const std::string&, const std::string&);



    }
  }
}

#endif

