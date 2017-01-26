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


#ifndef __gui_dialog_list_h__
#define __gui_dialog_list_h__

#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      class TreeItem
      { NOMEMALIGN
        public:
          TreeItem (const std::string& key, const std::string& value, TreeItem* parent = 0) {
            parentItem = parent;
            itemData << key.c_str() << value.c_str();
          }
          ~TreeItem() {
            qDeleteAll (childItems);
          }
          void appendChild (TreeItem* child)  {
            childItems.append (child);
          }
          TreeItem* child (int row)  {
            return childItems.value (row);
          }
          int childCount () const  {
            return childItems.count();
          }
          int columnCount () const  {
            return itemData.count();
          }
          QVariant data (int column) const  {
            return itemData.value (column);
          }
          int row () const  {
            if (parentItem) 
              return parentItem->childItems.indexOf (const_cast<TreeItem*> (this));
            return 0;
          }
          TreeItem* parent ()  {
            return parentItem;
          }

        private:
          QList<TreeItem*> childItems;
          QList<QVariant> itemData;
          TreeItem* parentItem;
      };


      class TreeModel : public QAbstractItemModel
      { NOMEMALIGN
        public:
          TreeModel (QObject* parent) : QAbstractItemModel (parent) {
            rootItem = new TreeItem ("Parameter", "Value");
          }
          ~TreeModel () {
            delete rootItem;
          }
          QVariant data (const QModelIndex& index, int role) const;
          Qt::ItemFlags flags (const QModelIndex& index) const;
          QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
          QModelIndex index (int row, int column, const QModelIndex& parent = QModelIndex()) const;
          QModelIndex parent (const QModelIndex& index) const;
          int rowCount (const QModelIndex& parent = QModelIndex()) const;
          int columnCount (const QModelIndex& parent = QModelIndex()) const;
          TreeItem* rootItem;
      };

    }
  }
}

#endif



