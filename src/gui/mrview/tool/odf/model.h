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


#ifndef __gui_mrview_tool_odf_model_h__
#define __gui_mrview_tool_odf_model_h__

#include <memory>
#include <string>
#include <vector>

#include "gui/mrview/tool/odf/item.h"
#include "gui/mrview/tool/odf/type.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {



        class ODF_Model : public QAbstractItemModel
        { MEMALIGN(ODF_Model)
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

            size_t add_items (const vector<std::string>& list, const odf_type_t type, bool colour_by_direction, bool hide_negative_lobes, float scale);

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

            vector<std::unique_ptr<ODF_Item>> items;
        };

      }
    }
  }
}

#endif





