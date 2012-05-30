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

#include <QLayout>
#include <QStyle>
#include <QPushButton>
#include <QTreeView>
#include <QSplitter>
#include <QLineEdit>
#include <QLabel>
#include <QTimer>
#include <QSortFilterProxyModel>

#include "app.h"
#include "file/path.h"
#include "file/dicom/quick_scan.h"
#include "file/dicom/image.h"
#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/dicom/patient.h"
#include "file/dicom/mapper.h"
#include "image/handler/default.h"
#include "gui/dialog/file.h"

#define FILE_DIALOG_BUSY_INTERVAL 0.1
#define SPACING 12

// TODO: volumes under Windows

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      std::string File::cwd (Path::cwd());
      QPoint File::window_position (-1, -1);
      QSize File::window_size (500, 500);

      /*************************************************
       *                 FolderModel                   *
       *************************************************/

      void FolderModel::add_entries (const QStringList& more)
      {
        if (more.empty()) return;
        beginInsertRows (QModelIndex(), list.size(), list.size() + more.size() - 1);
        list += more;
        endInsertRows();
        emit layoutChanged();
      }

      void FolderModel::clear ()
      {
        if (list.size()) {
          beginRemoveRows (QModelIndex(), 0, list.size()-1);
          list.clear();
          endRemoveRows();
        }
        emit layoutChanged();
      }

      int FolderModel::rowCount (const QModelIndex& parent) const
      {
        return list.size();
      }

      QVariant FolderModel::data (const QModelIndex& index, int role) const
      {
        if (index.isValid())
          if (index.row() < list.size() && role == Qt::DisplayRole)
            return (list.at (index.row()));
        return QVariant();
      }

      QVariant FolderModel::headerData (int section, Qt::Orientation orientation, int role) const
      {
        if (role != Qt::DisplayRole) return QVariant();
        return QString ("Folders");
      }




      /*************************************************
       *                 FileModel                     *
       *************************************************/

      QVariant FileModel::headerData (int section, Qt::Orientation orientation, int role) const
      {
        if (role != Qt::DisplayRole) return QVariant();
        return QString ("Files");
      }







      /*************************************************
       *                 Dialog::File                  *
       *************************************************/

      File::File (QWidget* parent, const std::string& message, bool multiselection, bool images_only) :
        QDialog (parent),
        filter_images (images_only),
        updating_selection (false),
        DICOM_import (false)
      {
        setWindowTitle (QString (message.c_str()));
        setModal (true);
        setSizeGripEnabled (true);
        resize (window_size);
        move (window_position);

        idle_timer = new QTimer (this);
        connect (idle_timer, SIGNAL (timeout()), this, SLOT (idle_slot()));

        QPushButton* button;
        QVBoxLayout* main_layout = new QVBoxLayout;

        QHBoxLayout* buttons_layout = new QHBoxLayout;

        button = new QPushButton (style()->standardIcon (QStyle::SP_FileDialogToParent), tr ("Up"));
        connect (button, SIGNAL (clicked()), this, SLOT (up_slot()));
        buttons_layout->addWidget (button);

        button = new QPushButton (style()->standardIcon (QStyle::SP_DirHomeIcon), tr ("Home"));
        connect (button, SIGNAL (clicked()), this, SLOT (home_slot()));
        buttons_layout->addWidget (button);

        button = new QPushButton (style()->standardIcon (QStyle::SP_DialogResetButton), tr ("Refresh"));
        connect (button, SIGNAL (clicked()), this, SLOT (update()));
        buttons_layout->addWidget (button);

        main_layout->addLayout (buttons_layout);
        main_layout->addSpacing (SPACING);

        QHBoxLayout* h_layout = new QHBoxLayout;
        h_layout->addWidget (new QLabel ("Path:"));
        path_entry = new QLineEdit;
        h_layout->addWidget (path_entry);
        main_layout->addLayout (h_layout);

        main_layout->addSpacing (SPACING);

        if (filter_images) {
          button = new QPushButton (style()->standardIcon (QStyle::SP_DirIcon), tr ("DICOM import..."));
          button->setToolTip ("recursively scan for DICOM images\nin this folder and all its subfolders");
          connect (button, SIGNAL (clicked()), this, SLOT (DICOM_import_slot()));
          main_layout->addWidget (button);

          main_layout->addSpacing (SPACING);
        }

        folders = new FolderModel;
        sorted_folders = new QSortFilterProxyModel;
        sorted_folders->setSourceModel (folders);
        sorted_folders->setSortCaseSensitivity (Qt::CaseInsensitive);

        folder_view = new QTreeView;
        folder_view->setModel (sorted_folders);
        folder_view->setRootIsDecorated (false);
        folder_view->setSortingEnabled (true);
        folder_view->sortByColumn (0, Qt::AscendingOrder);
        folder_view->setWordWrap (false);
        folder_view->setItemsExpandable (false);
        folder_view->setSelectionMode (QAbstractItemView::SingleSelection);

        files = new FileModel;
        sorted_files = new QSortFilterProxyModel;
        sorted_files->setSourceModel (files);
        sorted_files->setSortCaseSensitivity (Qt::CaseInsensitive);

        files_view = new QTreeView;
        files_view->setModel (sorted_files);
        files_view->setRootIsDecorated (false);
        files_view->setSortingEnabled (true);
        files_view->sortByColumn (0, Qt::AscendingOrder);
        files_view->setWordWrap (false);
        files_view->setItemsExpandable (false);
        files_view->setSelectionMode (multiselection ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection);

        QSplitter* splitter = new QSplitter;
        splitter->setChildrenCollapsible (false);
        splitter->addWidget (folder_view);
        splitter->addWidget (files_view);
        splitter->setStretchFactor (0, 1);
        splitter->setStretchFactor (1, 3);
        main_layout->addWidget (splitter);

        main_layout->addSpacing (SPACING);

        h_layout = new QHBoxLayout;
        h_layout->addWidget (new QLabel ("Selection:"));
        selection_entry = new QLineEdit;
        h_layout->addWidget (selection_entry);
        main_layout->addLayout (h_layout);

        main_layout->addSpacing (SPACING);

        buttons_layout = new QHBoxLayout;
        buttons_layout->addStretch (1);

        button = new QPushButton (tr ("Cancel"));
        connect (button, SIGNAL (clicked()), this, SLOT (reject()));
        buttons_layout->addWidget (button);

        ok_button = new QPushButton (tr ("OK"));
        ok_button->setDefault (true);
        ok_button->setEnabled (false);
        connect (ok_button, SIGNAL (clicked()), this, SLOT (accept()));
        buttons_layout->addWidget (ok_button);

        connect (folder_view, SIGNAL (activated (const QModelIndex&)), this, SLOT (folder_selected_slot (const QModelIndex&)));
        connect (files_view, SIGNAL (activated (const QModelIndex&)), this, SLOT (file_selected_slot (const QModelIndex&)));
        connect (files_view->selectionModel(), SIGNAL (selectionChanged (const QItemSelection&, const QItemSelection&)),
                 this, SLOT (file_selection_changed_slot()));

        main_layout->addLayout (buttons_layout);
        setLayout (main_layout);

        update();
      }





      File::~File ()
      {
        window_position = pos();
        window_size = size();
      }




      void File::update ()
      {
        setCursor (Qt::WaitCursor);
        folders->clear();
        files->clear();
        selection_entry->clear();
        path_entry->setText (cwd.c_str());
        dir = new Path::Dir (cwd);

        QStringList folder_list;
        std::string entry;
        while (! (entry = dir->read_name()).empty()) {
          if (entry[0] == '.') continue;
          if (Path::is_dir (Path::join (cwd, entry)))
            folder_list += entry.c_str();
        }
        folders->add_entries (folder_list);

        dir->rewind();
        idle_timer->start();
        elapsed_timer.start();
      }


      void File::up_slot ()
      {
        cwd = Path::dirname (cwd);
        update();
      }

      void File::home_slot ()
      {
        cwd = Path::home();
        update();
      }

      void File::folder_selected_slot (const QModelIndex& index)
      {
        cwd = Path::join (cwd, folders->name (sorted_folders->mapToSource (index)));
        update();
      }

      void File::file_selected_slot (const QModelIndex& index)
      {
        accept();
      }



      void File::file_selection_changed_slot ()
      {
        QModelIndexList indexes = files_view->selectionModel()->selectedIndexes();
        ok_button->setEnabled (indexes.size());
        if (indexes.size() == 1)
          selection_entry->setText (files->data (sorted_files->mapToSource (indexes[0]), Qt::DisplayRole).toString());
        else selection_entry->clear();
      }



      void File::DICOM_import_slot ()
      {
        files_view->selectionModel()->clear();
        selection_entry->clear();
        DICOM_import = true;
        accept();
      }



      void File::idle_slot ()
      {
        assert (dir);
        QStringList file_list;
        std::string entry;
        while (elapsed_timer.elapsed() < FILE_DIALOG_BUSY_INTERVAL) {
          entry = get_next_file();
          if (entry.empty()) {
            idle_timer->stop();
            files->add_entries (file_list);
            unsetCursor ();
            dir = NULL;
            return;
          }

          if (filter_images)
            if (!check_image (Path::join (cwd, entry)))
              continue;

          file_list += entry.c_str();
        }

        elapsed_timer.start();
        files->add_entries (file_list);
      }








      void File::get_selection (std::vector<std::string>& filenames)
      {
        assert (!filter_images);
        if (selection_entry->text().size())
          filenames.push_back (Path::join (cwd, selection_entry->text().toAscii().constData()));
        else {
          QModelIndexList indexes = files_view->selectionModel()->selectedIndexes();
          if (indexes.size()) {
            QModelIndex index;
            foreach (index, indexes)
            filenames.push_back (Path::join (cwd, files->name (sorted_files->mapToSource (index))));
          }
        }
      }





      void File::get_images (VecPtr<Image::Header>& images)
      {
        assert (filter_images);

        if (DICOM_import) {
          try {
            Image::Header* H = new Image::Header (cwd);
            images.push_back (H);
          }
          catch (Exception& E) {
            E.display();
          }
          return;
        }

        QModelIndexList indexes = files_view->selectionModel()->selectedIndexes();
        QModelIndex index;
        foreach (index, indexes) {
          try {
            Image::Header* H = new Image::Header (Path::join (cwd, files->name (sorted_files->mapToSource (index))));
            images.push_back (H);
          }
          catch (Exception& E) {
            E.display();
          }
        }
      }



    }
  }
}


