/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 27/06/08.

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

#include "gui/dialog/list.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      QVariant TreeModel::data (const QModelIndex& index, int role) const
      {
        if (!index.isValid()) return (QVariant());
        if (role != Qt::DisplayRole) return (QVariant());
        return (static_cast<TreeItem*> (index.internalPointer())->data (index.column()));
      }

      Qt::ItemFlags TreeModel::flags (const QModelIndex& index) const
      {
        if (!index.isValid()) return (0);
        return (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      }

      QVariant TreeModel::headerData (int section, Qt::Orientation orientation, int role) const
      {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) return (rootItem->data (section));
        return (QVariant());
      }

      QModelIndex TreeModel::index (int row, int column, const QModelIndex& parent) const
      {
        if (!hasIndex (row, column, parent)) return (QModelIndex());
        TreeItem* parentItem;
        if (!parent.isValid()) parentItem = rootItem;
        else parentItem = static_cast<TreeItem*> (parent.internalPointer());
        TreeItem* childItem = parentItem->child (row);
        if (childItem) return (createIndex (row, column, childItem));
        else return (QModelIndex());
      }

      QModelIndex TreeModel::parent (const QModelIndex& index) const
      {
        if (!index.isValid()) return (QModelIndex());
        TreeItem* childItem = static_cast<TreeItem*> (index.internalPointer());
        TreeItem* parentItem = childItem->parent();
        if (parentItem == rootItem) return (QModelIndex());
        return (createIndex (parentItem->row(), 0, parentItem));
      }

      int TreeModel::rowCount (const QModelIndex& parent) const
      {
        if (parent.column() > 0) return (0);
        TreeItem* parentItem;
        if (!parent.isValid()) parentItem = rootItem;
        else parentItem = static_cast<TreeItem*> (parent.internalPointer());
        return (parentItem->childCount());
      }

      int TreeModel::columnCount (const QModelIndex& parent) const
      {
        if (parent.isValid()) return static_cast<TreeItem*> (parent.internalPointer())->columnCount();
        else return rootItem->columnCount();
      }


    }
  }
}



