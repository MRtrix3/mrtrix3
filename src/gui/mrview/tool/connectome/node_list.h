/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2015.

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

#ifndef __gui_mrview_tool_connectome_node_list_h__
#define __gui_mrview_tool_connectome_node_list_h__

#include <memory>

#include "gui/mrview/tool/base.h"
#include "gui/mrview/tool/connectome/selection.h"

#include <QAbstractItemModel>
#include <QTableView>


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


        class Node_list_model : public QAbstractItemModel
        {
          public:

            Node_list_model (Connectome* parent);

            QVariant data (const QModelIndex& index, int role) const override;
            QVariant headerData (int section, Qt::Orientation orientation, int role) const override;

            Qt::ItemFlags flags (const QModelIndex& index) const override {
              if (!index.isValid()) return 0;
              return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            }

            QModelIndex parent (const QModelIndex&) const override {
              return QModelIndex(); 
            }

            int rowCount (const QModelIndex& parent = QModelIndex()) const override;
            int columnCount (const QModelIndex& parent = QModelIndex()) const override;

            QModelIndex index (int row, int column, const QModelIndex& parent = QModelIndex()) const {
              (void ) parent;
              return createIndex (row, column);
            }

            void clear()
            {
              beginRemoveRows (QModelIndex(), 0, rowCount()-1);
              endRemoveRows();
            }

            void initialize()
            {
              beginInsertRows (QModelIndex(), 0, rowCount()-1);
              endInsertRows();
            }

            void reset_pixmaps();

          private:
            Connectome& connectome;
        };


        class Node_list_view : public QTableView
        {
          public:
            Node_list_view (QWidget* parent) :
                QTableView (parent) { }
            void setModel (QAbstractItemModel* model)
            {
              QTableView::setModel (model);
              //setColumnWidth (0, model->headerData (0, Qt::Horizontal, Qt::SizeHintRole).toInt());
              //setColumnWidth (1, model->headerData (1, Qt::Horizontal, Qt::SizeHintRole).toInt());
            }
        };



        class Node_list : public Tool::Base
        {

            Q_OBJECT

          public:
            Node_list (Window&, Tool::Dock*, Connectome*);

            void initialize();
            void colours_changed();
            int row_height() const;

          private slots:
            void clear_selection_slot();
            void node_selection_changed_slot (const QItemSelection&, const QItemSelection&);
            void node_selection_settings_dialog_slot();

          private:
            Window& window;
            Connectome& connectome;

            QPushButton *clear_selection_button;
            QPushButton *node_selection_settings_button;
            Node_list_model *node_list_model;
            Node_list_view *node_list_view;

            // Settings related to how visual elements are changed on selection / non-selection
            std::unique_ptr<NodeSelectionSettingsDialog> node_selection_dialog;

        };



      }
    }
  }
}

#endif



