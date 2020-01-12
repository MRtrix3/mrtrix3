/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include <Eigen/Geometry>

#include "mrtrix.h"
#include "file/path.h"
#include "gui/mrview/window.h"
#include "gui/mrview/mode/base.h"
#include "gui/mrview/tool/annotation.h"
#include "gui/opengl/transformation.h"
#include "gui/mrview/tool/list_model_base.h"
#include "gui/dialog/file.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        class Annotation::Item : public Image { MEMALIGN(Annotation::Item)
          public:
            Item (MR::Header&& H) : Image (std::move (H)) { }
            // Mode::Slice::Shader slice_shader;
        };


        class Annotation::Model : public ListModelBase
        { MEMALIGN(Annotation::Model)
          public:
            Model (QObject* parent) :
              ListModelBase (parent) { }

            void add_items (vector<Label>& list);

            // Item* get_image (QModelIndex& index) {
            //   return dynamic_cast<Item*>(items[index.row()].get());
            // }
        };

        void Annotation::Model::add_items (vector<Label>& list)
        {
          beginInsertRows (QModelIndex(), items.size(), items.size()+list.size());
          for (size_t i = 0; i < list.size(); ++i) {
            // Item* label = new Item (std::move (*list[i]));
            // label->set_allowed_features (true, true, false);
            // items.push_back (std::unique_ptr<Displayable> (label));
            // items.push_back (make_unique<Label> (list[i].centre, list[i].label, list[i].description));
          }
          endInsertRows();
        }

        Annotation::Annotation (Dock* parent) :
          Base (parent)
        {
          VBoxLayout* main_box = new VBoxLayout (this);

          QGroupBox* output_group_box = new QGroupBox (tr("Annotation file"));
          main_box->addWidget (output_group_box);
          GridLayout* output_grid_layout = new GridLayout;
          output_group_box->setLayout (output_grid_layout);

          open_button = new QPushButton (tr("Open annotation file"), this);
          open_button->setToolTip (tr ("Annotation file"));
          connect (open_button, SIGNAL (clicked()), this, SLOT (select_annotation_slot ()));
          output_grid_layout->addWidget (open_button, 0, 0);

          HBoxLayout* layout = new HBoxLayout;
          layout->setContentsMargins (0, 0, 0, 0);
          layout->setSpacing (0);
          main_box->addLayout (layout, 0);

          annotation_list_view = new QListView (this);
          annotation_list_view->setSelectionMode (QAbstractItemView::ExtendedSelection);
          annotation_list_view->setDragEnabled (true);
          annotation_list_view->setDragDropMode (QAbstractItemView::InternalMove);
          annotation_list_view->setAcceptDrops (true);
          annotation_list_view->viewport()->setAcceptDrops (true);
          annotation_list_view->setDropIndicatorShown (true);

          annotation_list_model = new Model (this);
          annotation_list_view->setModel (annotation_list_model);

          annotation_list_view->setContextMenuPolicy (Qt::CustomContextMenu);
          // connect (annotation_list_view, SIGNAL (customContextMenuRequested (const QPoint&)),
          //          this, SLOT (right_click_menu_slot (const QPoint&)));

          main_box->addWidget (annotation_list_view);

          main_box->addStretch ();

          connect (&window(), SIGNAL (imageChanged()), this, SLOT (on_image_changed()));
          on_image_changed();
        }


        void Annotation::load_annotation_slot ()
        {
          labels.clear();
          Eigen::Vector3f centre = Eigen::Vector3f::Zero();
          std::string label = "label";
          std::string description = "description";
          labels.push_back (Label (centre, label, description));
          
          size_t previous_size = annotation_list_model->rowCount();
          annotation_list_model->add_items (labels);

          QModelIndex first = annotation_list_model->index (previous_size, 0, QModelIndex());
          QModelIndex last = annotation_list_model->index (annotation_list_model->rowCount()-1, 0, QModelIndex());
          annotation_list_view->selectionModel()->select (QItemSelection (first, last), QItemSelectionModel::ClearAndSelect);
        }



        void Annotation::on_image_changed() {
          // cached_state.clear();
          const auto image = window().image();
          if(!image) return;

          // int max_axis = std::max((int)image->header().ndim() - 1, 0);
          // volume_axis->setMaximum (max_axis);
          // volume_axis->setValue (std::min (volume_axis->value(), max_axis));
        }


        void Annotation::select_annotation_slot ()
        {
          const std::string path = GUI::Dialog::File::get_file (this, "Select annotation file", "annotation files (*.csv)");
          if (!path.size()) return;

          // const QString path = QFileDialog::getOpenFileName (this, tr("File"), directory->path());
          // if (!path.size()) return;
          // directory->setPath (path);
          open_button->setText (shorten (path, 20, 0).c_str());
          // open_button->setText (shorten (path.toUtf8().constData(), 20, 0).c_str());
          on_annotation_update ();
        }


        void Annotation::on_annotation_update () {
          // start_index->setValue (0);
        }


        void Annotation::add_commandline_options (MR::App::OptionList& options)
        {
          using namespace MR::App;
          options
            + OptionGroup ("Annotation tool options")

            + Option ("annotation.load", "Load annotation file.").allow_multiple()
            +   Argument ("path").type_text();

        }

        bool Annotation::process_commandline_option (const MR::App::ParsedOption& opt)
        {
          if (opt.opt->is ("annotation.load")) {
            QString path (shorten(std::string(opt[0]).c_str(), 20, 0).c_str());
            open_button->setText(path);
            on_annotation_update ();
            return true;
          }

          return false;
        }


      }
    }
  }
}






