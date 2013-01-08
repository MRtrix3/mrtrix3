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

#include <QAbstractItemModel>

#include "gui/mrview/displayable.h"

#ifndef __gui_mrview_tool_list_model_base_h__
#define __gui_mrview_tool_list_model_base_h__


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
                return shown[index.row()] ? Qt::Checked : Qt::Unchecked;
              }
              if (role != Qt::DisplayRole) return QVariant();
              return shorten (tractograms[index.row()]->get_filename(), 20, 0).c_str();
            }
            bool setData (const QModelIndex& index, const QVariant& value, int role) {
              if (role == Qt::CheckStateRole) {
                shown[index.row()] =  (value == Qt::Checked);
                emit dataChanged(index, index);
                return true;
              }
              return QAbstractItemModel::setData (index, value, role);
            }

            Qt::ItemFlags flags (const QModelIndex& index) const {
              if (!index.isValid()) return 0;
              return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
            }
            QModelIndex index (int row, int column, const QModelIndex& parent = QModelIndex()) const { return createIndex (row, column); }
            QModelIndex parent (const QModelIndex& index) const { return QModelIndex(); }

            int rowCount (const QModelIndex& parent = QModelIndex()) const { return tractograms.size(); }
            int columnCount (const QModelIndex& parent = QModelIndex()) const { return 1; }

            void add_tractograms (std::vector<std::string>& filenames);
            void remove_tractogram (QModelIndex& index);
            VecPtr<Displayable> items;
            std::vector<bool> shown;
        };


      }
    }
  }
}

#endif




