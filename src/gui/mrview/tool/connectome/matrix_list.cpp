/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2015.

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






      void Matrix_list_model::add_items (std::vector<FileDataVector>& list) {
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



