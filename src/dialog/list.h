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

#ifndef __dialog_list_h__
#define __dialog_list_h__

#include <QModelIndex>
#include <QVariant>
#include <QList>
#include <QAbstractListModel>

namespace MR {
  namespace Dialog {

    class TreeItem
    {
      public:
        TreeItem (const std::string& key, const std::string& value, TreeItem *parent = 0) { 
          parentItem = parent;
          itemData << key.c_str() << value.c_str();
        }
        ~TreeItem() { qDeleteAll(childItems); }
        void appendChild (TreeItem *child)  { childItems.append (child); }
        TreeItem *child (int row)  { return (childItems.value(row)); }
        int childCount () const  { return (childItems.count()); } 
        int columnCount () const  { return (itemData.count()); }
        QVariant data (int column) const  { return (itemData.value (column)); } 
        int row () const  { if (parentItem) return (parentItem->childItems.indexOf(const_cast<TreeItem*>(this))); return (0); }
        TreeItem *parent ()  { return (parentItem); }

      private:
        QList<TreeItem*> childItems;
        QList<QVariant> itemData;
        TreeItem *parentItem;
    };


    class TreeModel : public QAbstractItemModel
    {
      public:
        TreeModel (QObject *parent) : QAbstractItemModel (parent) { 
          QList<QVariant> rootData;
          rootItem = new TreeItem ("Parameter", "Value");
        }
        ~TreeModel () { delete rootItem; }
        QVariant data (const QModelIndex &index, int role) const;
        Qt::ItemFlags flags (const QModelIndex &index) const;
        QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
        QModelIndex index (int row, int column, const QModelIndex &parent = QModelIndex()) const;
        QModelIndex parent (const QModelIndex &index) const;
        int rowCount (const QModelIndex &parent = QModelIndex()) const;
        int columnCount (const QModelIndex &parent = QModelIndex()) const;
        TreeItem *rootItem;
    };

  }
}

#endif



