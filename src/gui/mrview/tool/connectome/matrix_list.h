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

#ifndef __gui_mrview_tool_connectome_matrix_list_h__
#define __gui_mrview_tool_connectome_matrix_list_h__

#include <memory>

#include <QAbstractItemModel>

#include "mrtrix.h"
#include "gui/mrview/tool/connectome/file_data_vector.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      class Window;

      namespace Tool
      {


        class Connectome;


        class Matrix_list_model : public QAbstractItemModel
        { MEMALIGN(Matrix_list_model)
          public:

            Matrix_list_model (Connectome* parent);

            QVariant data (const QModelIndex& index, int role) const override {
              if (!index.isValid()) return QVariant();
              if (role != Qt::DisplayRole) return QVariant();
              return shorten (items[index.row()].get_name().toStdString(), 35, 0).c_str();
            }

            Qt::ItemFlags flags (const QModelIndex& index) const override {
              if (!index.isValid()) return 0;
              return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            }

            QModelIndex parent (const QModelIndex&) const override {
              return QModelIndex(); 
            }

            int rowCount (const QModelIndex& parent = QModelIndex()) const override {
              (void) parent; // to suppress warnings about unused parameters
              return items.size();
            }

            int columnCount (const QModelIndex& parent = QModelIndex()) const override {
              (void) parent;
              return 1;
            }

            QModelIndex index (int row, int column, const QModelIndex& parent = QModelIndex()) const override {
              (void ) parent;
              return createIndex (row, column);
            }

            void remove_item (QModelIndex& index) {
              beginRemoveRows (QModelIndex(), index.row(), index.row());
              items.erase (items.begin() + index.row());
              endRemoveRows();
            }

            void clear() {
              beginRemoveRows (QModelIndex(), 0, items.size());
              items.clear();
              endRemoveRows();
            }

            void add_items (std::vector<FileDataVector>&);

            const FileDataVector& get (const size_t index) { assert (index < items.size()); return items[index]; }
            const FileDataVector& get (QModelIndex& index) { assert (size_t(index.row()) < items.size()); return items[index.row()]; }

          protected:
            std::vector<FileDataVector> items;

        };


      }
    }
  }
}

#endif



