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


#include "gui/dialog/list.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      QVariant TreeModel::data (const QModelIndex& index, int role) const
      {
        if (!index.isValid()) return QVariant();
        if (role != Qt::DisplayRole) return QVariant();
        return static_cast<TreeItem*> (index.internalPointer())->data (index.column());
      }

      Qt::ItemFlags TreeModel::flags (const QModelIndex& index) const
      {
        if (!index.isValid()) return 0;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
      }

      QVariant TreeModel::headerData (int section, Qt::Orientation orientation, int role) const
      {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) return rootItem->data (section);
        return QVariant();
      }

      QModelIndex TreeModel::index (int row, int column, const QModelIndex& parent) const
      {
        if (!hasIndex (row, column, parent)) return QModelIndex();
        TreeItem* parentItem;
        if (!parent.isValid()) parentItem = rootItem;
        else parentItem = static_cast<TreeItem*> (parent.internalPointer());
        TreeItem* childItem = parentItem->child (row);
        if (childItem) return createIndex (row, column, childItem);
        else return QModelIndex();
      }

      QModelIndex TreeModel::parent (const QModelIndex& index) const
      {
        if (!index.isValid()) return QModelIndex();
        TreeItem* childItem = static_cast<TreeItem*> (index.internalPointer());
        TreeItem* parentItem = childItem->parent();
        if (parentItem == rootItem) return QModelIndex();
        return createIndex (parentItem->row(), 0, parentItem);
      }

      int TreeModel::rowCount (const QModelIndex& parent) const
      {
        if (parent.column() > 0) return 0;
        TreeItem* parentItem;
        if (!parent.isValid()) parentItem = rootItem;
        else parentItem = static_cast<TreeItem*> (parent.internalPointer());
        return parentItem->childCount();
      }

      int TreeModel::columnCount (const QModelIndex& parent) const
      {
        if (parent.isValid()) return static_cast<TreeItem*> (parent.internalPointer())->columnCount();
        else return rootItem->columnCount();
      }


    }
  }
}



