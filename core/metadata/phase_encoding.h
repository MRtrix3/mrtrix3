/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#ifndef __metadata__phase_encoding_h__
#define __metadata__phase_encoding_h__

#include <Eigen/Dense>
#include <array>

#include "app.h"
#include "axes.h"
#include "file/nifti_utils.h"
#include "file/ofstream.h"
#include "header.h"
#include "metadata/bids.h"
#include "types.h"

namespace MR {
  namespace Metadata {
    namespace PhaseEncoding {



      extern const App::OptionGroup ImportOptions;
      extern const App::OptionGroup SelectOptions;
      extern const App::OptionGroup ExportOptions;

      using scheme_type = Eigen::MatrixXd;

      //! check that a phase-encoding table is valid
      void check(const scheme_type& PE);

      //! check that the PE scheme matches the DWI data in \a header
      void check(const scheme_type& PE, const Header& header);

      //! store the phase encoding matrix in a header
      /*! this will store the phase encoding matrix into the
       *  Header::keyval() structure of \a header.
       *  - If the phase encoding direction and/or total readout
       *    time varies between volumes, then the information
       *    will be stored in field "pe_scheme"; if not, it
       *    will instead be stored in fields "PhaseEncodingDirection"
       *    and "TotalReadoutTime"
       */
      void set_scheme(KeyValues& keyval, const scheme_type& PE);

      //! clear the phase encoding matrix from a key-value dictionary
      /*! this will delete any trace of phase encoding information
       *  from the dictionary.
       */
      void clear_scheme(KeyValues& keyval);

      //! parse the phase encoding matrix from a key-value dictionary
      /*! extract the phase encoding matrix stored in \a the key-value dictionary
       *  if one is present. The key-value dictionary is not in all use cases
       *  the "keyval" member of the Header class.
       */
      Eigen::MatrixXd parse_scheme(const KeyValues&, const Header&);

      //! get a phase encoding matrix
      /*! get a valid phase-encoding matrix, either from files specified at
       *  the command-line that exclusively provide phase encoding information
       *  (ie. NOT from .json; that is handled elsewhere),
       *  or from the contents of the image header.
       */
      Eigen::MatrixXd get_scheme(const Header&);

      //! Convert a phase-encoding scheme into the EDDY config / indices format
      void scheme2eddy(const scheme_type& PE, Eigen::MatrixXd& config, Eigen::Array<int, Eigen::Dynamic, 1>& indices);

      //! Convert phase-encoding infor from the EDDY config / indices format into a standard scheme
      scheme_type eddy2scheme(const Eigen::MatrixXd&, const Eigen::Array<int, Eigen::Dynamic, 1>&);

      //! Modifies a phase encoding scheme if being imported alongside a non-RAS image
      //  and internal header realignment is performed
      void transform_for_image_load(KeyValues& keyval, const Header& H);
      scheme_type transform_for_image_load(const scheme_type& pe_scheme, const Header& H);

      //! Modifies a phase encoding scheme if being exported alongside a NIfTI image
      void transform_for_nifti_write(KeyValues& keyval, const Header& H);
      scheme_type transform_for_nifti_write(const scheme_type& pe_scheme, const Header& H);

      namespace {
        void _save(const scheme_type& PE, const std::string& path) {
          File::OFStream out(path);
          for (ssize_t row = 0; row != PE.rows(); ++row) {
            // Write phase-encode direction as integers; other information as floating-point
            out << PE.template block<1, 3>(row, 0).template cast<int>();
            if (PE.cols() > 3)
              out << " " << PE.block(row, 3, 1, PE.cols() - 3);
            out << "\n";
          }
        }
      } // namespace

      //! Save a phase-encoding scheme to file
      // Note that because the output table requires permutation / sign flipping
      //   only if the output target image is a NIfTI, the output file name must have
      //   already been set
      template <class HeaderType>
      void save(const scheme_type &PE, const HeaderType &header, const std::string &path) {
        try {
          check(PE, header);
        } catch (Exception &e) {
          throw Exception(e, "Cannot export phase-encoding table to file \"" + path + "\"");
        }

        if (Path::has_suffix(header.name(), {".mgh", ".mgz", ".nii", ".nii.gz", ".img"})) {
          _save(transform_for_nifti_write(PE, header), path);
        } else {
          _save(PE, path);
        }
      }

      //! Save a phase-encoding scheme to EDDY format config / index files
      template <class HeaderType>
      void save_eddy(const scheme_type& PE,
                     const HeaderType& header,
                     const std::string& config_path,
                     const std::string& index_path) {
        Eigen::MatrixXd config;
        Eigen::Array<int, Eigen::Dynamic, 1> indices;
        scheme2eddy(transform_for_nifti_write(PE, header), config, indices);
        save_matrix(config, config_path, KeyValues(), false);
        save_vector(indices, index_path, KeyValues(), false);
      }

      //! Save the phase-encoding scheme from a header to file depending on command-line options
      void export_commandline(const Header&);

      //! Load a phase-encoding scheme from a matrix text file
      scheme_type load(const std::string& path, const Header& header);

      //! Load a phase-encoding scheme from an EDDY-format config / indices file pair
      scheme_type load_eddy(const std::string& config_path, const std::string& index_path, const Header& header);



    }
  }
}

#endif
