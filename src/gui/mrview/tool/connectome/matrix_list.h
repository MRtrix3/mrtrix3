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
              if (!index.isValid()) return {};
              if (role != Qt::DisplayRole) return {};
              return qstr (shorten (items[index.row()].get_name().toStdString(), 35, 0));
            }

            Qt::ItemFlags flags (const QModelIndex& index) const override {
              if (!index.isValid()) return {};
              return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            }

            QModelIndex parent (const QModelIndex&) const override {
              return {};
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

            void add_items (vector<FileDataVector>&);

            const FileDataVector& get (const size_t index) { assert (index < items.size()); return items[index]; }
            const FileDataVector& get (QModelIndex& index) { assert (size_t(index.row()) < items.size()); return items[index.row()]; }

          protected:
            vector<FileDataVector> items;

        };


      }
    }
  }
}

#endif



