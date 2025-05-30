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

#ifndef __gui_mrview_tool_connectome_filedatavector_h__
#define __gui_mrview_tool_connectome_filedatavector_h__

#include <QString>

#include "gui/gui.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

      // Vector that stores the name of the file imported, so it can be displayed in the GUI
      class FileDataVector : public Eigen::VectorXf
      { MEMALIGN(FileDataVector)
        public:
          using base_t = Eigen::VectorXf;
          FileDataVector ();
          FileDataVector (const FileDataVector&);
          FileDataVector (FileDataVector&&);
          FileDataVector (const size_t);
          FileDataVector (const std::string&);

          FileDataVector& operator= (const FileDataVector&);
          FileDataVector& operator= (FileDataVector&&);

          FileDataVector& load (const std::string&);
          FileDataVector& clear();

          const QString& get_name() const { return name; }
          void set_name (const std::string& s) { name = qstr (s); }

          float get_min()  const { return min; }
          float get_mean() const { return mean; }
          float get_max()  const { return max; }

          void calc_stats();

        private:
          QString name;
          float min, mean, max;

      };

      }
    }
  }
}

#endif




