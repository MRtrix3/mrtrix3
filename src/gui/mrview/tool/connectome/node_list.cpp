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


#include "gui/mrview/tool/connectome/node_list.h"

#include <QVector>

#include "gui/mrview/tool/base.h"
#include "gui/mrview/tool/connectome/connectome.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {



      Node_list_model::Node_list_model (Connectome* parent) :
          QAbstractItemModel (dynamic_cast<QObject*>(parent)),
          connectome (*parent) { }



      QVariant Node_list_model::data (const QModelIndex& index, int role) const
      {
        if (!index.isValid()) return QVariant();
        if (role == Qt::TextAlignmentRole) {
          switch (index.column()) {
            case 0: return Qt::AlignRight;
            case 1: return Qt::AlignCenter;
            case 2: return Qt::AlignLeft;
            default: assert (0); return QVariant();
          }
        }
        if (index.column() == 0 && role == Qt::DisplayRole)
          return str(index.row()).c_str();
        else if (index.column() == 1 && role == Qt::DecorationRole)
          return connectome.nodes[index.row()].get_pixmap();
        else if (index.column() == 2 && role == Qt::DisplayRole)
          return connectome.nodes[index.row()].get_name().c_str();
        else
          return QVariant();
      }

      QVariant Node_list_model::headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const
      {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
          return QVariant();
        switch (section) {
          case 0: return "Index";
          case 1: return "";
          case 2: return "Name";
          default: assert (0); return "";
        }
      }



      int Node_list_model::rowCount (const QModelIndex& parent) const
      {
        (void) parent;  // to suppress warnings about unused parameters
        return (connectome.num_nodes() ? connectome.num_nodes() + 1 : 0);
      }
      int Node_list_model::columnCount (const QModelIndex& parent) const
      {
        (void) parent; // to suppress warnings about unused parameters
        return 3;
      }



      void Node_list_model::reset_pixmaps()
      {
        QModelIndex topleft = createIndex (0, 0);
        QModelIndex bottomright = createIndex (rowCount()-1, 0);
#if QT_VERSION >= 0x050400
        QVector<int> roles (1, Qt::DecorationRole);
        emit dataChanged (topleft, bottomright, roles);
#else
         emit dataChanged (topleft, bottomright);
#endif
      }







      Node_list::Node_list (Tool::Dock* dock, Connectome* master) :
          Tool::Base (dock),
          connectome (*master),
          node_selection_dialog (nullptr)
      {
        VBoxLayout* main_box = new VBoxLayout (this);

        HBoxLayout* hlayout = new HBoxLayout;
        main_box->addLayout (hlayout);
        clear_selection_button = new QPushButton (this);
        clear_selection_button->setToolTip (tr ("Clear node selection"));
        clear_selection_button->setIcon (QIcon (":/clear.svg"));
        connect (clear_selection_button, SIGNAL(clicked()), this, SLOT (clear_selection_slot()));
        hlayout->addWidget (clear_selection_button);
        node_selection_settings_button = new QPushButton (this);
        node_selection_settings_button->setToolTip (tr ("Visual settings for selections"));
        node_selection_settings_button->setIcon (QIcon (":/settings.svg"));
        connect (node_selection_settings_button, SIGNAL(clicked()), this, SLOT (node_selection_settings_dialog_slot()));
        hlayout->addWidget (node_selection_settings_button);

        node_list_model = new Node_list_model (master);
        node_list_view = new Node_list_view (master);
        node_list_view->setModel (node_list_model);
        node_list_view->setAcceptDrops (false);
        node_list_view->setAlternatingRowColors (true);
        node_list_view->setCornerButtonEnabled (false);
        node_list_view->setDragEnabled (false);
        node_list_view->setDropIndicatorShown (false);
        node_list_view->setEditTriggers (QAbstractItemView::NoEditTriggers);
        node_list_view->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
        node_list_view->setObjectName ("Node list view");
        node_list_view->resizeColumnsToContents();
        node_list_view->resizeRowsToContents();
        node_list_view->setSelectionBehavior (QAbstractItemView::SelectRows);
        node_list_view->setSelectionMode (QAbstractItemView::ExtendedSelection);
        node_list_view->horizontalHeader()->setStretchLastSection (true);
        node_list_view->verticalHeader()->hide();
        node_list_view->verticalHeader()->setDefaultSectionSize (row_height());
        connect (node_list_view->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
            SLOT (node_selection_changed_slot(const QItemSelection &, const QItemSelection &)) );
        main_box->addWidget (node_list_view);
      }


      void Node_list::initialize()
      {
        node_list_model->initialize();
        node_list_view->hideRow (0);
      }



      void Node_list::colours_changed()
      {
        node_list_model->reset_pixmaps();
      }



      int Node_list::row_height() const
      {
        return node_list_view->fontMetrics().height();
      }



      void Node_list::clear_selection_slot()
      {
        node_list_view->clearSelection();
        vector<node_t> empty_node_list;
        connectome.node_selection_changed (empty_node_list);
      }


      void Node_list::node_selection_settings_dialog_slot()
      {
        if (!node_selection_dialog)
          node_selection_dialog.reset (new NodeSelectionSettingsDialog (&window(), "Node selection visual settings", connectome.node_selection_settings));
        node_selection_dialog->show();
      }



      void Node_list::node_selection_changed_slot (const QItemSelection&, const QItemSelection&)
      {
        QModelIndexList list = node_list_view->selectionModel()->selectedRows();
        vector<node_t> nodes;
        for (int i = 0; i != list.size(); ++i)
          nodes.push_back (list[i].row());
        connectome.node_selection_changed (nodes);
      }






      }
    }
  }
}



