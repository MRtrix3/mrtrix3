/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __phaseencoding_h__
#define __phaseencoding_h__


#include <Eigen/Dense>

#include "app.h"
#include "axes.h"
#include "header.h"
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
    template <class MatrixType>
    void check (const Header& header, const MatrixType& PE)
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
       *    will be stored
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
      PhaseEncoding::check (header, PE);
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
        erase ("PhaseEncodingDirection");
        erase ("TotalReadoutTime");
      } else {
        erase ("pe_scheme");
        const Eigen::Vector3 dir { PE(0, 0), PE(0, 1), PE(0, 2) };
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

    //! Convert phase-encoding informat from the EDDY config / indices format into a standard scheme
    Eigen::MatrixXd eddy2scheme (const Eigen::MatrixXd&, const Eigen::Array<int, Eigen::Dynamic, 1>&);



    //! Save a phase-encoding scheme to file
    template <class MatrixType>
    void save (const MatrixType& PE, const std::string& path)
    {
      try {
        check (PE);
      } catch (Exception& e) {
        throw Exception (e, "Cannot export phase-encoding table to file \"" + path + "\"");
      }

      File::OFStream out (path);
      for (ssize_t row = 0; row != PE.rows(); ++row) {
        // Write phase-encode direction as integers; other information as floating-point
        out << PE.template block<1, 3>(row, 0).template cast<int>();
        if (PE.cols() > 3)
          out << " " << PE.block(row, 3, 1, PE.cols()-3);
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



    //! Save the phase-encoding scheme from a header to file depending on command-line options
    void export_commandline (const Header&);



    //! Load a phase-encoding scheme from either a matrix file, or an EDDY-format comfig / indices file pair
    Eigen::MatrixXd load (const std::string&);
    Eigen::MatrixXd load_eddy (const std::string&, const std::string&);



  }
}

#endif

