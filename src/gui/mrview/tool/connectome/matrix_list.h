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

#ifndef __gui_mrview_tool_connectome_matrix_list_h__
#define __gui_mrview_tool_connectome_matrix_list_h__

#include <memory>

#include <QAbstractItemModel>

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
        {
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



