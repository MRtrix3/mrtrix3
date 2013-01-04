/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

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

#include <QLabel>
#include <QListView>
#include <QStringListModel>

#include "mrtrix.h"
#include "gui/mrview/window.h"
#include "gui/mrview/tool/overlay.h"
#include "gui/dialog/file.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        class Overlay::Model : public QAbstractItemModel
        {
          public:
            Model (QObject* parent) : 
              QAbstractItemModel (parent) { }

            QVariant data (const QModelIndex& index, int role) const {
              if (!index.isValid()) return QVariant();
              if (role == Qt::CheckStateRole) {
                return shown[index.row()] ? Qt::Checked : Qt::Unchecked;
              }
              if (role != Qt::DisplayRole) return QVariant();
              return shorten (images[index.row()]->header().name(), 20, 0).c_str();
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
            int rowCount (const QModelIndex& parent = QModelIndex()) const { return images.size(); }
            int columnCount (const QModelIndex& parent = QModelIndex()) const { return 1; }

            void add_images (VecPtr<MR::Image::Header>& list);
            void remove_images (QModelIndexList& indexes);
            VecPtr<Image> images;
            std::vector<bool> shown;
        };


        void Overlay::Model::add_images (VecPtr<MR::Image::Header>& list)
        {
          beginInsertRows (QModelIndex(), images.size(), images.size()+list.size());
          for (size_t i = 0; i < list.size(); ++i) {
            images.push_back (new Image (*list[i]));
            //setData (createIndex (images.size()-1,0), Qt::Checked, Qt::CheckStateRole);
          }
          shown.resize (images.size(), true);
          endInsertRows();
        }

        void Overlay::Model::remove_images (QModelIndexList& indexes)
        {
          beginRemoveRows (QModelIndex(), indexes.first().row(), indexes.first().row());
          images.erase (images.begin() + indexes.first().row());
          shown.resize (images.size(), true);
          endRemoveRows();
        }




        Overlay::Overlay (Window& main_window, Dock* parent) :
          Base (main_window, parent) { 
            QVBoxLayout* main_box = new QVBoxLayout (this);
            QHBoxLayout* layout = new QHBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);

            QPushButton* button = new QPushButton (this);
            button->setToolTip (tr ("Open Image"));
            button->setIcon (QIcon (":/open.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (image_open_slot ()));
            layout->addWidget (button, 1);

            button = new QPushButton (this);
            button->setToolTip (tr ("Close Image"));
            button->setIcon (QIcon (":/close.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (image_close_slot ()));
            layout->addWidget (button, 1);

            main_box->addLayout (layout, 0);

            image_list_view = new QListView(this);
            image_list_view->setSelectionMode (QAbstractItemView::MultiSelection);
            image_list_view->setDragEnabled (true);
            image_list_view->viewport()->setAcceptDrops (true);
            image_list_view->setDropIndicatorShown (true);

            image_list_model = new Model (this);
            image_list_view->setModel (image_list_model);

            main_box->addWidget (image_list_view, 1);
          }


        void Overlay::image_open_slot ()
        {
          Dialog::File dialog (this, "Select overlay images to open", true, true);
          if (dialog.exec()) {
            VecPtr<MR::Image::Header> list;
            dialog.get_images (list);
            image_list_model->add_images (list);
          }
        }



        void Overlay::image_close_slot ()
        {
          QModelIndexList indexes = image_list_view->selectionModel()->selectedIndexes();
          image_list_model->remove_images (indexes);
        }

      }
    }
  }
}





