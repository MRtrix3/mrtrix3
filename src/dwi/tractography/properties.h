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

#ifndef __dwi_tractography_properties_h__
#define __dwi_tractography_properties_h__

#include <map>
#include "app.h"
#include "timer.h"
#include "types.h"
#include "dwi/tractography/roi.h"
#include "dwi/tractography/seeding/list.h"


#define TRACTOGRAPHY_FILE_TIMESTAMP_PRECISION 20


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {



      void check_timestamps (const Properties&, const Properties&, const std::string&);
      void check_counts (const Properties&, const Properties&, const std::string&, bool abort_on_fail);



      class Properties : public KeyValues { MEMALIGN(Properties)
        public:

          Properties () {
            set_timestamp();
          }

          void set_timestamp();
          void set_version_info();
          void update_command_history();
          void clear();

          template <typename T> void set (T& variable, const std::string& name) {
            if ((*this)[name].empty()) (*this)[name] = str (variable);
            else variable = to<T> ((*this)[name]);
          }

          float get_stepsize() const;
          void compare_stepsize_rois() const;

          // In use at time of execution
          ROIUnorderedSet include, exclude, mask;
          ROIOrderedSet ordered_include;
          Seeding::List seeds;

          // As stored within the header of an existing .tck file
          std::multimap<std::string, std::string> prior_rois;

          vector<std::string> comments;

          friend std::ostream& operator<< (std::ostream& stream, const Properties& P);
      };



    }
  }
}

#endif
