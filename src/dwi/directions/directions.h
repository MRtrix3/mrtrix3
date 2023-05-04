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

#ifndef __dwi_directions_directions_h__
#define __dwi_directions_directions_h__

#include "app.h"
#include "header.h"
#include "types.h"


namespace MR {
  namespace DWI {
    namespace Directions {



      using index_type = unsigned int;


      extern const char* directions_description;
      MR::App::Option directions_option (const std::string& purpose, const std::string& default_set);


      Eigen::MatrixXd load (const std::string& spec, const bool force_singleshell);
      Eigen::MatrixXd load (const Header& H, const bool force_singleshell);


    }
  }
}

#endif

