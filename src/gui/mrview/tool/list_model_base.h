/*
   Copyright 2012 Brain Research Institute, Melbourne, Australia

   Written by Donald Tournier and David Raffelt, 08/01/13.

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

#ifndef __gui_mrview_tool_list_model_base_h__
#define __gui_mrview_tool_list_model_base_h__

#include "gui/mrview/displayable.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        class ListModelBase : public QAbstractItemModel
        {
          public:

            ListModelBase (QObject* parent) :
              QAbstractItemModel (parent) { }

            QVariant data (const QModelIndex& index, int role) const {
              if (!index.isValid()) return QVariant();
              if (role == Qt::CheckStateRole) {
                return items[index.row()]->show ? Qt::Checked : Qt::Unchecked;
              }
              if (role != Qt::DisplayRole) return QVariant();
              return shorten (items[index.row()]->get_filename(), 35, 0).c_str();
            }

            bool setData (const QModelIndex& idx, const QVariant& value, int role) {
              if (role == Qt::CheckStateRole) {
                Qt::KeyboardModifiers keyMod = QApplication::keyboardModifiers ();
                if (keyMod.testFlag (Qt::ShiftModifier)) {
                  for (int i = 0; i < (int)items.size(); ++i) {
                    if (i == idx.row())
                      items[i]->show = true;
                    else
                      items[i]->show = false;
                  }
                  emit dataChanged (index(0, 0), index(items.size(), 0));
                } else {
                  items[idx.row()]->show = (value == Qt::Checked);
                  emit dataChanged (idx, idx);
                }
                return true;
              }
              return QAbstractItemModel::setData (idx, value, role);
            }

            Qt::ItemFlags flags (const QModelIndex& index) const {
              if (!index.isValid()) return 0;
              return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
            }

            QModelIndex index (int row, int column, const QModelIndex& parent = QModelIndex()) const { 
              (void) parent; // to suppress warnings about unused parameters
              return createIndex (row, column); 
            }

            QModelIndex parent (const QModelIndex&) const { return QModelIndex(); }

            int rowCount (const QModelIndex& parent = QModelIndex()) const { 
              (void) parent;  // to suppress warnings about unused parameters
              return items.size(); 
            }

            int columnCount (const QModelIndex& parent = QModelIndex()) const { 
              (void) parent;  // to suppress warnings about unused parameters
              return 1; 
            }

            void remove_item (QModelIndex& index) {
              beginRemoveRows (QModelIndex(), index.row(), index.row());
              items.erase (items.begin() + index.row());
              endRemoveRows();
            }

            std::vector<std::unique_ptr<Displayable>> items;
        };


      }
    }
  }
}

#endif




