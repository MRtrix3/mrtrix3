/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

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

#include "gui/mrview/tool/odf/model.h"

#include "header.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        size_t ODF_Model::add_items (const std::vector<std::string>& list, bool colour_by_direction, bool hide_negative_lobes, float scale) {
          std::vector<std::unique_ptr<MR::Header>> hlist;
          for (size_t i = 0; i < list.size(); ++i) {
            try {
              std::unique_ptr<MR::Header> header (new MR::Header (MR::Header::open (list[i])));
              hlist.push_back (std::move (header));
            }
            catch (Exception& E) {
              E.display();
            }
          }

          if (hlist.size()) {
            beginInsertRows (QModelIndex(), items.size(), items.size()+hlist.size());
            for (size_t i = 0; i < hlist.size(); ++i)
              items.push_back (std::unique_ptr<ODF_Item> (new ODF_Item (std::move (*hlist[i]), scale, hide_negative_lobes, colour_by_direction)));
            endInsertRows();
          }

          return hlist.size();
        }



      }
    }
  }
}





