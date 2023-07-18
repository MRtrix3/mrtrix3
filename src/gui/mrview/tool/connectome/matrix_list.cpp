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

#include "gui/mrview/tool/connectome/matrix_list.h"

#include "gui/mrview/tool/connectome/connectome.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {



      Matrix_list_model::Matrix_list_model (Connectome* parent) :
          QAbstractItemModel (dynamic_cast<QObject*>(parent)) { }






      void Matrix_list_model::add_items (vector<FileDataVector>& list) {
        beginInsertRows (QModelIndex(), items.size(), items.size() + list.size());
        items.reserve (items.size() + list.size());
        std::move (std::begin (list), std::end (list), std::back_inserter (items));
        list.clear();
        endInsertRows();
      }





      }
    }
  }
}



