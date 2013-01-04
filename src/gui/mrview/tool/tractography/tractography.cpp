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
#include "gui/mrview/tool/tractography/tractography.h"
#include "gui/mrview/tool/tractography/tractogram.h"
#include "gui/dialog/file.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        class Tractography::Model : public QAbstractItemModel
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
            void remove_tractograms (QModelIndexList& indexes);
            VecPtr<Tractogram> tractograms;
            std::vector<bool> shown;
        };


        void Tractography::Model::add_tractograms (std::vector<std::string>& list)
        {
          beginInsertRows (QModelIndex(), tractograms.size(), tractograms.size() + list.size());
          for (size_t i = 0; i < list.size(); ++i)
            tractograms.push_back (new Tractogram (list[i]));
          shown.resize (tractograms.size(), true);
          endInsertRows();
        }

        void Tractography::Model::remove_tractograms (QModelIndexList& indexes)
        {
          // TODO fix problem with multiple selection remove
          for (int i = 0; i < indexes.size(); ++i) {
            beginRemoveRows (QModelIndex(), indexes.at(i).row(), indexes.at(i).row());
            tractograms.erase (tractograms.begin() + indexes.first().row());
            shown.resize (tractograms.size(), true);
            endRemoveRows();
          }
        }




        Tractography::Tractography (Window& main_window, Dock* parent) :
          Base (main_window, parent) { 
            QVBoxLayout* main_box = new QVBoxLayout (this);
            QHBoxLayout* layout = new QHBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);

            QPushButton* button = new QPushButton (this);
            button->setToolTip (tr ("Open Tracks"));
            button->setIcon (QIcon (":/open.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (tractogram_open_slot ()));
            layout->addWidget (button, 1);

            button = new QPushButton (this);
            button->setToolTip (tr ("Close Tracks"));
            button->setIcon (QIcon (":/close.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (tractogram_close_slot ()));
            layout->addWidget (button, 1);

            main_box->addLayout (layout, 0);

            tractogram_list_view = new QListView (this);
            tractogram_list_view->setSelectionMode (QAbstractItemView::MultiSelection);
            tractogram_list_view->setDragEnabled (true);
            tractogram_list_view->viewport()->setAcceptDrops (true);
            tractogram_list_view->setDropIndicatorShown (true);

            tractogram_list_model = new Model (this);
            tractogram_list_view->setModel (tractogram_list_model);

            main_box->addWidget (tractogram_list_view, 1);
          }


        void Tractography::tractogram_open_slot ()
        {
          Dialog::File dialog (this, "Select tractograms to open", true, false);
          if (dialog.exec()) {
            std::vector<std::string> list;
            dialog.get_selection (list);
            tractogram_list_model->add_tractograms (list);
          }
        }


        void Tractography::tractogram_close_slot ()
        {
          QModelIndexList indexes = tractogram_list_view->selectionModel()->selectedIndexes();
          tractogram_list_model->remove_tractograms (indexes);
        }

      }
    }
  }
}





