/*
   Copyright 2013 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt, 08/01/13.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "gui/mrview/tool/list_model_base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        void ListModelBase::add_item (std::vector<std::string>& list)
        {
          beginInsertRows (QModelIndex(), items.size(), items.size() + list.size());
          for (size_t i = 0; i < list.size(); ++i)
            items.push_back (new Tractogram (list[i]));
          shown.resize (items.size(), true);
          endInsertRows();
        }

        void ListModelBase::remove_item (QModelIndex& index)
        {
          beginRemoveRows (QModelIndex(), index.row(), index.row());
          tractograms.erase (tractograms.begin() + index.row());
          shown.resize (tractograms.size(), true);
          endRemoveRows();
        }

      }
    }
  }
}





