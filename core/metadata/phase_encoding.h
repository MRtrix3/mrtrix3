/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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
#include "version.h"

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
      scheme_type parse_scheme(const KeyValues&, const Header&);

      //! get a phase encoding matrix
      /*! get a valid phase-encoding matrix, either from files specified at
       *  the command-line that exclusively provide phase encoding information
       *  (ie. NOT from .json; that is handled elsewhere),
       *  or from the contents of the image header.
       */
      scheme_type get_scheme(const Header&);

      //! Convert a phase-encoding scheme in TOPUP format into the EDDY config / indices format
      void topup2eddy(const scheme_type& PE, Eigen::MatrixXd& config, Eigen::Array<int, Eigen::Dynamic, 1>& indices);

      //! Convert phase-encoding infor from the EDDY config / indices format into a TOPUP format scheme
      scheme_type eddy2topup(const Eigen::MatrixXd&, const Eigen::Array<int, Eigen::Dynamic, 1>&);

      //! Modifies a phase encoding scheme if being imported alongside a non-RAS image
      //  and internal header realignment is performed
      void transform_for_image_load(KeyValues& keyval, const Header& H);
      scheme_type transform_for_image_load(const scheme_type& pe_scheme, const Header& H);

      //! Modifies a phase encoding scheme if being exported alongside a NIfTI image
      void transform_for_nifti_write(KeyValues& keyval, const Header& H);
      scheme_type transform_for_nifti_write(const scheme_type& pe_scheme, const Header& H);

      void save_table(const scheme_type& PE, const std::string& path, bool write_command_history);

      template <class HeaderType>
      void save_table(const HeaderType &header, const std::string &path) {
        const scheme_type scheme = get_scheme(header);
        if (scheme.rows() == 0)
          throw Exception ("No phase encoding scheme in header of image \"" + header.name() + "\" to save");
        save(scheme, header, path);
      }

      //! Save a phase-encoding scheme associated with an image to file
      // Note that because the output table requires permutation / sign flipping
      //   only if the output target image is a NIfTI,
      //   for this function to operate as intended it is necessary for it to be executed
      //   after having set the output file name in the image header
      template <class HeaderType>
      void save_table(const scheme_type &PE, const HeaderType &header, const std::string &path) {
        try {
          check(PE, header);
        } catch (Exception &e) {
          throw Exception(e, "Cannot export phase-encoding table to file \"" + path + "\"");
        }

        if (Path::has_suffix(header.name(), {".mgh", ".mgz", ".nii", ".nii.gz", ".img"})) {
          WARN("External phase encoding table \"" + path + "\" for image \"" + header.name() + "\""
               " may not be suitable for FSL topup; consider use of -export_pe_topup instead"
               " (see: mrtrix.readthedocs.org/en/" MRTRIX_BASE_VERSION "/concepts/pe_scheme.html#reference-axes-for-phase-encoding-directions)");
          save_table(transform_for_nifti_write(PE, header), path, true);
        } else {
          save_table(PE, path, true);
        }
      }

      template <class HeaderType>
      void save_topup(const scheme_type &PE, const HeaderType &header, const std::string &path) {
        try {
          check(PE, header);
        } catch (Exception &e) {
          throw Exception(e, "Cannot export phase-encoding table to file \"" + path + "\"");
        }

        if (!Path::has_suffix(header.name(), {".mgh", ".mgz", ".nii", ".nii.gz", ".img"})) {
          WARN("Beware use of -export_pe_topup in conjunction image format other than MGH / NIfTI;"
               " -export_pe_table may be more suitable"
               " (see: mrtrix.readthedocs.org/en/" MRTRIX_BASE_VERSION "/concepts/pe_scheme.html#reference-axes-for-phase-encoding-directions)");
        }

        scheme_type table = transform_for_nifti_write(PE, header);
        // The flipping of first axis based on the determinant of the image header transform
        //   applies to what is stored on disk
        Axes::permutations_type order;
        const auto adjusted_transform = File::NIfTI::adjust_transform(header, order);
        if (adjusted_transform.linear().determinant() > 0.0)
          table.col(0) *= -1.0;
        save_table(table, path, false);
      }

      //! Save a phase-encoding scheme to EDDY format config / index files
      template <class HeaderType>
      void save_eddy(const scheme_type& PE,
                     const HeaderType& header,
                     const std::string& config_path,
                     const std::string& index_path) {
        if (!Path::has_suffix(header.name(), {".mgh", ".mgz", ".nii", ".nii.gz", ".img"})) {
          WARN("Exporting phase encoding table to FSL eddy format"
               " in conjunction with format other than MGH / NIfTI"
               " risks erroneous interpretation due to possible flipping of first image axis"
               " (see: mrtrix.readthedocs.org/en/" MRTRIX_BASE_VERSION "/concepts/pe_scheme.html#reference-axes-for-phase-encoding-directions)");
        }
        scheme_type table = transform_for_nifti_write(PE, header);
        Axes::permutations_type order;
        const auto adjusted_transform = File::NIfTI::adjust_transform(header, order);
        if (adjusted_transform.linear().determinant() > 0.0)
          table.col(0) *= -1.0;
        Eigen::MatrixXd config;
        Eigen::Array<int, Eigen::Dynamic, 1> indices;
        topup2eddy(table, config, indices);
        save_matrix(config, config_path, KeyValues(), false);
        save_vector(indices, index_path, KeyValues(), false);
      }

      //! Save the phase-encoding scheme from a header to file depending on command-line options
      void export_commandline(const Header&);

      //! Load a phase-encoding scheme from a matrix text file
      scheme_type load_table(const std::string& path, const Header& header);

      //! Load a phase-encoding scheme from a FSL topup format text file
      scheme_type load_topup(const std::string& path, const Header& header);

      //! Load a phase-encoding scheme from an EDDY-format config / indices file pair
      scheme_type load_eddy(const std::string& config_path, const std::string& index_path, const Header& header);



    }
  }
}

#endif
