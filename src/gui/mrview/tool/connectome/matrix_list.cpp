/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
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



