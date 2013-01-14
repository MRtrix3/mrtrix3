/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier and David Raffelt, 13/11/09.

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
#include "gui/mrview/tool/tractography.h"
#include "gui/mrview/tractogram.h"
#include "gui/dialog/file.h"
#include "gui/mrview/adjust_button.h"
#include "gui/mrview/tool/list_model_base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        class Tractography::Model : public ListModelBase
        {
          public:
            Model (QObject* parent) :
              ListModelBase (parent) { }

            void add_items (std::vector<std::string>& filenames, Tractography& tractography_tool);
        };


        void Tractography::Model::add_items (std::vector<std::string>& list,
                                               Tractography& tractography_tool)
        {
          beginInsertRows (QModelIndex(), items.size(), items.size() + list.size());
          for (size_t i = 0; i < list.size(); ++i)
            items.push_back (new Tractogram (list[i], tractography_tool));
          shown.resize (items.size(), true);
          endInsertRows();
        }





        Tractography::Tractography (Window& main_window, Dock* parent) :
          Base (main_window, parent),
          line_thickness (1.0) {
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

            connect (tractogram_list_view, SIGNAL (clicked (const QModelIndex&)), this, SLOT (toggle_shown (const QModelIndex&)));

            main_box->addWidget (tractogram_list_view, 1);

            QGridLayout* default_opt_grid = new QGridLayout;

            QWidget::setStyleSheet("QSlider { margin: 5 0 5 0px;  }"
                                   "QGroupBox { padding:7 3 0 0px; margin: 10 0 5 0px; border: 1px solid gray; border-radius: 4px}"
                                   "QGroupBox::title { subcontrol-position: top left; top:-8px; left:5px}");

            QGroupBox* slab_group_box = new QGroupBox (tr("crop to slab"));
            slab_group_box->setCheckable (true);
            slab_group_box->setChecked (true);
            default_opt_grid->addWidget (slab_group_box, 0, 0, 1, 2);

            QGridLayout* slab_layout = new QGridLayout;
            slab_group_box->setLayout(slab_layout);
            slab_layout->addWidget (new QLabel ("thickness (mm)"), 0, 0);
            AdjustButton* slab_entry = new AdjustButton (this, 0.1);
            slab_entry->setValue (5.0);
            slab_entry->setMin(0.0);
            connect (slab_entry, SIGNAL (valueChanged()), this, SLOT (on_slab_thickness_change()));
            slab_layout->addWidget (slab_entry, 0, 1);

            QSlider* slider;
            slider = new QSlider (Qt::Horizontal);
            slider->setRange (0,1000);
            slider->setSliderPosition (int (1000));
            connect (slider, SIGNAL (valueChanged (int)), this, SLOT (opacity_slot (int)));
            default_opt_grid->addWidget (new QLabel ("opacity"), 1, 0);
            default_opt_grid->addWidget (slider, 1, 1);

            slider = new QSlider (Qt::Horizontal);
            slider->setRange (1,9);
            slider->setSliderPosition (int (1));
            connect (slider, SIGNAL (valueChanged (int)), this, SLOT (line_thickness_slot (int)));
            default_opt_grid->addWidget (new QLabel ("line thickness"), 2, 0);
            default_opt_grid->addWidget (slider, 2, 1);

            main_box->addLayout (default_opt_grid, 0);
        }

        void Tractography::draw2D (const Projection& transform) {
          for (int i = 0; i < tractogram_list_model->rowCount(); ++i) {
            if (tractogram_list_model->shown[i])
              dynamic_cast<Tractogram*>(tractogram_list_model->items[i])->render2D (transform);
          }
        }

        void Tractography::draw3D (const Projection& transform) {
          TEST;
        }

        int Tractography::get_line_thickness () {
          return line_thickness;
        }

        void Tractography::tractogram_open_slot ()
        {
          Dialog::File dialog (this, "Select tractograms to open", true, false);
          if (dialog.exec()) {
            std::vector<std::string> list;
            dialog.get_selection (list);
            tractogram_list_model->add_items (list, *this);
          }
        }

        void Tractography::tractogram_close_slot ()
        {
          QModelIndexList indexes = tractogram_list_view->selectionModel()->selectedIndexes();
          while (indexes.size()) {
            tractogram_list_model->remove_item (indexes.first());
            indexes = tractogram_list_view->selectionModel()->selectedIndexes();
          }
          window.updateGL();
        }

        void Tractography::opacity_slot (int opacity) {
          CONSOLE(str(opacity));
        }

        void Tractography::line_thickness_slot (int thickness) {
          line_thickness = thickness;
          window.updateGL();
        }

        void Tractography::on_slab_thickness_change() {
        }

        void Tractography::toggle_shown(const QModelIndex& index) {
          window.updateGL();
        }

      }
    }
  }
}





