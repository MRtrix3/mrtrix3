/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
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
        { MEMALIGN(Node_list_model)
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

            QModelIndex index (int row, int column, const QModelIndex& parent = QModelIndex()) const override {
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
        { NOMEMALIGN
          public:
            Node_list_view (QWidget* parent) :
                QTableView (parent) { }
            void setModel (QAbstractItemModel* model)
            {
              QTableView::setModel (model);
              //setColumnWidth (0, model->original_headerData (0, Qt::Horizontal, Qt::SizeHintRole).toInt());
              //setColumnWidth (1, model->original_headerData (1, Qt::Horizontal, Qt::SizeHintRole).toInt());
            }
        };



        class Node_list : public Tool::Base
        { MEMALIGN(Node_list)

            Q_OBJECT

          public:
            Node_list (Tool::Dock*, Connectome*);

            void initialize();
            void colours_changed();
            int row_height() const;

          private slots:
            void clear_selection_slot();
            void node_selection_changed_slot (const QItemSelection&, const QItemSelection&);
            void node_selection_settings_dialog_slot();

          private:
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



