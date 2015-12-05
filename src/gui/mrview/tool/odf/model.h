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

#ifndef __gui_mrview_tool_odf_model_h__
#define __gui_mrview_tool_odf_model_h__

#include <memory>
#include <string>
#include <vector>

#include "gui/mrview/tool/odf/item.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {



        class ODF_Model : public QAbstractItemModel
        {
          public:

            ODF_Model (QObject* parent) :
              QAbstractItemModel (parent) { }

            QVariant data (const QModelIndex& index, int role) const {
              if (!index.isValid()) return QVariant();
              if (role != Qt::DisplayRole) return QVariant();
              return shorten (items[index.row()]->image.get_filename(), 35, 0).c_str();
            }

            bool setData (const QModelIndex& index, const QVariant& value, int role) {
              return QAbstractItemModel::setData (index, value, role);
            }

            Qt::ItemFlags flags (const QModelIndex& index) const {
              if (!index.isValid()) return 0;
              return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            }

            QModelIndex parent (const QModelIndex&) const {
              return QModelIndex();
            }

            int rowCount (const QModelIndex& parent = QModelIndex()) const {
              (void) parent;  // to suppress warnings about unused parameters
              return items.size();
            }

            int columnCount (const QModelIndex& parent = QModelIndex()) const {
              (void) parent; // to suppress warnings about unused parameters
              return 1;
            }

            size_t add_items (const std::vector<std::string>& list, bool colour_by_direction, bool hide_negative_lobes, float scale);

            QModelIndex index (int row, int column, const QModelIndex& parent = QModelIndex()) const {
              (void ) parent; // to suppress warnings about unused parameters
              return createIndex (row, column);
            }

            void remove_item (QModelIndex& index) {
              beginRemoveRows (QModelIndex(), index.row(), index.row());
              items.erase (items.begin() + index.row());
              endRemoveRows();
            }

            ODF_Item* get_image (QModelIndex& index) {
              return index.isValid() ? dynamic_cast<ODF_Item*>(items[index.row()].get()) : NULL;
            }

            std::vector<std::unique_ptr<ODF_Item>> items;
        };

      }
    }
  }
}

#endif





