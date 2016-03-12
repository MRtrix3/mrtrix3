/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
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




