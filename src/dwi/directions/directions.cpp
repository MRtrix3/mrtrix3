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

#include "dwi/directions/directions.h"

#include "exception.h"
#include "header.h"
#include "mrtrix.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "dwi/directions/file.h"
#include "dwi/directions/predefined.h"
#include "math/sphere.h"


namespace MR {
  namespace DWI {
    namespace Directions {



      const char* directions_description =
          "Where the user is permitted to provide a set of directions that is "
          "requisite for the relevant command operation "
          "(including as an alternative to data that may be already present in an input image header), "
          "there are a number of permissible inputs from which the user could choose, including: "
          "an integer value corresponding to a built-in direction set; "
          "a path to a text file containing directions "
          "(in either spherical, ie. [az el] pairs, or cartesian coordinates, ie. [x y z] triplets); "
          "a path to an image, where this set could be extracted from either key-value entry \"directions\" "
          "or from the diffusion gradient table.";

      MR::App::Option directions_option (const std::string& purpose, const std::string& default_set)
      {
        MR::App::Option opt = MR::App::Option ("directions", "Specify a source of a basis direction set to be used " + purpose + " (see Description); "
                                                             "default: " + default_set + "")
            + MR::App::Argument ("spec").type_various();
        return opt;
      }



      namespace
      {
        Eigen::MatrixXd process_dw_scheme (const Eigen::MatrixXd& grad, const bool force_singleshell)
        {
          if (!force_singleshell)
            return grad;
          auto dwi_volumes = DWI::Shells (grad).select_shells (true, false, true).largest().get_volumes();
          return DWI::gen_direction_matrix (grad, dwi_volumes);
        };
      }



      Eigen::MatrixXd load (const std::string& spec, const bool force_singleshell)
      {
        // Hierarchy of ways in which a direction set could be loaded:
        // 1. By specifying an integer that corresponds to a built-in set
        // 2. By loading from a text file (which could be in spherical or cartesian format)
        // 3. By loading from image header entry "directions" (which could be in spherical or cartesian format)
        // 4. By loading from image header entry "dw_scheme" (as long as it's single-shell)
        //    (if an app can support multi-shell, it'll need to make its own subsequent attempt at loading and interpreting)

        DEBUG ("Attempting to load direction set based on user specification \"" + spec + "\"");

        // Integer
        int count = 0;
        try {
          count = to<int> (spec);
        } catch (Exception&) {}
        if (count) {
          DEBUG ("Loading internally predefined " + str(count) + "-direction set");
          return load (count);
        }

        // Text file
        Eigen::MatrixXd from_file;
        try {
          from_file = load_matrix<> (spec);
        } catch (Exception&) {}
        if (from_file.rows()) {
          DEBUG ("Loading " + str(from_file.rows()) + "-direction set from external text file");
          switch (from_file.cols()) {
            case 2:
            case 3:
              Math::Sphere::check (from_file); return from_file;
            case 4:
              return process_dw_scheme (from_file, force_singleshell);
            default:
              throw Exception ("Unsupported number of columns (" + str(from_file.cols()) + ") in text file \"" + spec + "\" that is supposed to provide direction set");
          }
        }

        // Image
        try {
          Header H (Header::open (spec));
          DEBUG ("Loaded image \"" + H.name() + "\" for seeking direction set");
          return load (H, force_singleshell);
        } catch (Exception&) {}

        throw Exception ("Unable to load direction set based on specification \"" + spec + "\"");
      }



      Eigen::MatrixXd load (const Header& H, const bool force_singleshell)
      {
        // Header "directions"
        auto directions_it = H.keyval().find ("directions");
        if (directions_it != H.keyval().end()) {
          try {
            const Eigen::MatrixXd data = deserialise_matrix<> (directions_it->second);
            DEBUG ("Loading " + str(data.rows()) + "-direction set from key-value entry \"directions\" in image \"" + H.name() + "\"");
            Math::Sphere::check (data);
            return data;
          } catch (Exception&) {
            WARN ("Corrupt \"directions\" key-value field in image \"" + H.name() + "\" ignored");
          }
        } else {
          DEBUG ("Header key-value \"directions\" absent from image \"" + H.name() + "\"");
        }

        // Header "dw_scheme"
        try {
          auto grad = process_dw_scheme (DWI::get_DW_scheme (H), force_singleshell);
          DEBUG ("Loading " + str(grad.rows()) + "-direction set from diffusion gradient table in image \"" + H.name() + "\"");
          return grad;
        } catch (Exception&) {}

        throw Exception ("Unable to load direction set from image \"" + H.name() + "\"");
      }



    }
  }
}

