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

#include "mrtrix.h"
#include "gui/dialog/file.h"
#include "gui/dialog/lighting.h"
#include "gui/dwi/render_frame.h"
#include "gui/mrview/window.h"
#include "gui/mrview/tool/odf.h"
#include "gui/mrview/tool/odf_preview.h"
#include "gui/mrview/mode/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        class ODF::Image {
          public:
            Image (const MR::Image::Header& header, int lmax, float scale, bool hide_negative_lobes, bool color_by_direction) :
              image (header),
              lmax (lmax),
              scale (scale),
              hide_negative_lobes (hide_negative_lobes),
              color_by_direction (color_by_direction) { }

            MRView::Image image;
            int lmax;
            float scale;
            bool hide_negative_lobes, color_by_direction;
        };



        class ODF::Model : public QAbstractItemModel
        {
          public:

            Model (QObject* parent) :
              QAbstractItemModel (parent) { }

            QVariant data (const QModelIndex& index, int role) const {
              if (!index.isValid()) return QVariant();
              if (role != Qt::DisplayRole) return QVariant();
              return shorten (items[index.row()]->image.get_filename(), 35, 0).c_str();
            }

            bool setData (const QModelIndex& index, const QVariant& value, int role) {
              return QAbstractItemModel::setData (index, value, role);
            }

            Qt::ItemFlags flags (const QModelIndex& index) const {
              if (!index.isValid()) return 0;
              return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            }

            QModelIndex parent (const QModelIndex&) const {
              return QModelIndex(); 
            }

            int rowCount (const QModelIndex& parent = QModelIndex()) const {
              (void) parent;  // to suppress warnings about unused parameters
              return items.size();
            }

            int columnCount (const QModelIndex& parent = QModelIndex()) const {
              (void) parent; // to suppress warnings about unused parameters
              return 1;
            }

            size_t add_items (const std::vector<std::string>& list, int lmax, bool colour_by_direction, bool hide_negative_lobes, float scale) {
              VecPtr<MR::Image::Header> hlist;
              for (size_t i = 0; i < list.size(); ++i) {
                try {
                  Ptr<MR::Image::Header> header (new MR::Image::Header (list[i]));
                  if (header->ndim() < 4) 
                    throw Exception ("image \"" + header->name() + "\" is not 4D");
                  if (header->dim(3) < 6)
                    throw Exception ("image \"" + header->name() + "\" does not contain enough SH coefficients (too few volumes along 4th axis)");
                  hlist.push_back (header.release());
                }
                catch (Exception& E) {
                  E.display();
                }
              }

              if (hlist.size()) {
                beginInsertRows (QModelIndex(), items.size(), items.size()+hlist.size());
                for (size_t i = 0; i < hlist.size(); ++i) 
                  items.push_back (new Image (*hlist[i], lmax, scale, hide_negative_lobes, colour_by_direction));
                endInsertRows();
              }

              return hlist.size();
            }

            QModelIndex index (int row, int column, const QModelIndex& parent = QModelIndex()) const { 
              (void ) parent; // to suppress warnings about unused parameters
              return createIndex (row, column); 
            }

            void remove_item (QModelIndex& index) {
              beginRemoveRows (QModelIndex(), index.row(), index.row());
              items.erase (items.begin() + index.row());
              endRemoveRows();
            }

            Image* get_image (QModelIndex& index) {
              return index.isValid() ? dynamic_cast<Image*>(items[index.row()]) : nullptr;
            }

            VecPtr<Image> items;
        };








        ODF::ODF (Window& main_window, Dock* parent) :
          Base (main_window, parent),
          renderer (nullptr),
          lighting_dialog (nullptr),
          lmax (0),
          level_of_detail (0) { 

            lighting = new GL::Lighting (this);

            VBoxLayout *main_box = new VBoxLayout (this);

            HBoxLayout* layout = new HBoxLayout;

            QPushButton* button = new QPushButton (this);
            button->setToolTip (tr ("Open ODF image"));
            button->setIcon (QIcon (":/open.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (image_open_slot ()));
            layout->addWidget (button, 1);

            button = new QPushButton (this);
            button->setToolTip (tr ("Close ODF image"));
            button->setIcon (QIcon (":/close.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (image_close_slot ()));
            layout->addWidget (button, 1);

            hide_all_button = new QPushButton (this);
            hide_all_button->setToolTip (tr ("Hide all ODFs"));
            hide_all_button->setIcon (QIcon (":/hide.svg"));
            hide_all_button->setCheckable (true);
            connect (hide_all_button, SIGNAL (clicked()), this, SLOT (hide_all_slot ()));
            layout->addWidget (hide_all_button, 1);

            main_box->addLayout (layout, 0);


            image_list_view = new QListView (this);
            image_list_view->setSelectionMode (QAbstractItemView::SingleSelection);
            image_list_view->setDragEnabled (true);
            image_list_view->viewport()->setAcceptDrops (true);
            image_list_view->setDropIndicatorShown (true);

            image_list_model = new Model (this);
            image_list_view->setModel (image_list_model);

            main_box->addWidget (image_list_view, 1);


            show_preview_button = new QPushButton ("Inspect ODF at focus",this);
            show_preview_button->setToolTip (tr ("Inspect ODF at focus<br>(opens separate window)"));
            show_preview_button->setIcon (QIcon (":/odf_preview.svg"));
            connect (show_preview_button, SIGNAL (clicked()), this, SLOT (show_preview_slot ()));
            main_box->addWidget (show_preview_button, 1);
            

            QGroupBox* group_box = new QGroupBox (tr("Display settings"));
            main_box->addWidget (group_box);
            GridLayout* box_layout = new GridLayout;
            group_box->setLayout (box_layout);

            QLabel* label = new QLabel ("scale");
            label->setAlignment (Qt::AlignHCenter);
            box_layout->addWidget (label, 0, 0);
            scale = new AdjustButton (this, 1.0);
            scale->setValue (1.0);
            scale->setMin (0.0);
            connect (scale, SIGNAL (valueChanged()), this, SLOT (adjust_scale_slot()));
            box_layout->addWidget (scale, 0, 1, 1, 3);

            label = new QLabel ("detail");
            label->setAlignment (Qt::AlignHCenter);
            box_layout->addWidget (label, 1, 0);
            level_of_detail_selector = new QSpinBox (this);
            level_of_detail_selector->setMinimum (1);
            level_of_detail_selector->setMaximum (6);
            level_of_detail_selector->setSingleStep (1);
            level_of_detail_selector->setValue (3);
            connect (level_of_detail_selector, SIGNAL (valueChanged(int)), this, SLOT(updateGL()));
            box_layout->addWidget (level_of_detail_selector, 1, 1);

            label = new QLabel ("lmax");
            label->setAlignment (Qt::AlignHCenter);
            box_layout->addWidget (label, 1, 2);
            lmax_selector = new QSpinBox (this);
            lmax_selector->setMinimum (2);
            lmax_selector->setMaximum (16);
            lmax_selector->setSingleStep (2);
            lmax_selector->setValue (8);
            connect (lmax_selector, SIGNAL (valueChanged(int)), this, SLOT(lmax_slot(int)));
            box_layout->addWidget (lmax_selector, 1, 3);
            
            interpolation_box = new QCheckBox ("interpolation");
            interpolation_box->setChecked (true);
            connect (interpolation_box, SIGNAL (stateChanged(int)), this, SLOT (updateGL()));
            box_layout->addWidget (interpolation_box, 2, 0, 1, 2);

            hide_negative_lobes_box = new QCheckBox ("hide negative lobes");
            hide_negative_lobes_box->setChecked (true);
            connect (hide_negative_lobes_box, SIGNAL (stateChanged(int)), this, SLOT (hide_negative_lobes_slot(int)));
            box_layout->addWidget (hide_negative_lobes_box, 2, 2, 1, 2);

            lock_to_grid_box = new QCheckBox ("lock to grid");
            lock_to_grid_box->setChecked (true);
            connect (lock_to_grid_box, SIGNAL (stateChanged(int)), this, SLOT (updateGL()));
            box_layout->addWidget (lock_to_grid_box, 3, 0, 1, 2);

            colour_by_direction_box = new QCheckBox ("colour by direction");
            colour_by_direction_box->setChecked (true);
            connect (colour_by_direction_box, SIGNAL (stateChanged(int)), this, SLOT (colour_by_direction_slot(int)));
            box_layout->addWidget (colour_by_direction_box, 3, 2, 1, 2);

            main_grid_box = new QCheckBox ("use main grid");
            main_grid_box->setToolTip (tr ("Show individual ODFs using the grid of the main image instead of the ODF image's own grid"));
            main_grid_box->setChecked (false);
            connect (main_grid_box, SIGNAL (stateChanged(int)), this, SLOT (updateGL()));
            box_layout->addWidget (main_grid_box, 4, 0, 1, 2);

            use_lighting_box = new QCheckBox ("use lighting");
            use_lighting_box->setCheckable (true);
            use_lighting_box->setChecked (true);
            connect (use_lighting_box, SIGNAL (stateChanged(int)), this, SLOT (use_lighting_slot(int)));
            box_layout->addWidget (use_lighting_box, 4, 2, 1, 2);

            QPushButton *lighting_settings_button = new QPushButton ("ODF colour and lighting...", this);
            connect (lighting_settings_button, SIGNAL(clicked(bool)), this, SLOT (lighting_settings_slot (bool)));
            box_layout->addWidget (lighting_settings_button, 5, 0, 1, 4);


            connect (image_list_view->selectionModel(),
                SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
                SLOT (selection_changed_slot(const QItemSelection &, const QItemSelection &)) );

            connect (lighting, SIGNAL (changed()), this, SLOT (updateGL()));


            hide_negative_lobes_slot (0);
            colour_by_direction_slot (0);
            lmax_slot (0);
            adjust_scale_slot ();

            preview = new Dock (&main_window,nullptr);
            main_window.addDockWidget (Qt::RightDockWidgetArea, preview);
            preview->tool = new ODF_Preview (window, preview, this);
            preview->tool->adjustSize();
            preview->setWidget (preview->tool);
            preview->setFloating (true);
            preview->hide();
            connect (lighting, SIGNAL (changed()), preview->tool, SLOT (lighting_update_slot()));
          }

        ODF::~ODF()
        {
          if (renderer) {
            delete renderer;
            renderer = nullptr;
          }
          if (lighting_dialog) {
            delete lighting_dialog;
            lighting_dialog = nullptr;
          }
        }






        void ODF::draw (const Projection& projection, bool is_3D, int, int)
        {
          if (is_3D) 
            return;


          Image* settings = get_image();
          if (!settings)
            return;

          MRView::Image& image (main_grid_box->isChecked() ? *window.image() : settings->image);

          if (!hide_all_button->isChecked()) {

            if (!renderer) {
              renderer = new DWI::Renderer;
              renderer->initGL();
            }

            if (lmax != settings->lmax || 
                level_of_detail != level_of_detail_selector->value()) {
              lmax = settings->lmax;
              level_of_detail = level_of_detail_selector->value();
              renderer->update_mesh (level_of_detail, lmax);
            }

            renderer->start (projection, *lighting, settings->scale, 
                use_lighting_box->isChecked(), settings->color_by_direction, settings->hide_negative_lobes, true);

            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);

            Point<> pos (window.target());
            pos += projection.screen_normal() * (projection.screen_normal().dot (window.focus() - window.target()));
            if (lock_to_grid_box->isChecked()) {
              Point<> p = image.interp.scanner2voxel (pos);
              p[0] = std::round (p[0]);
              p[1] = std::round (p[1]);
              p[2] = std::round (p[2]);
              pos = image.interp.voxel2scanner (p);
            }

            Point<> x_dir = projection.screen_to_model_direction (1.0, 0.0, projection.depth_of (pos));
            x_dir.normalise();
            x_dir = image.interp.scanner2image_dir (x_dir);
            x_dir[0] *= image.interp.vox(0);
            x_dir[1] *= image.interp.vox(1);
            x_dir[2] *= image.interp.vox(2);
            x_dir = image.interp.image2scanner_dir (x_dir);

            Point<> y_dir = projection.screen_to_model_direction (0.0, 1.0, projection.depth_of (pos));
            y_dir.normalise();
            y_dir = image.interp.scanner2image_dir (y_dir);
            y_dir[0] *= image.interp.vox(0);
            y_dir[1] *= image.interp.vox(1);
            y_dir[2] *= image.interp.vox(2);
            y_dir = image.interp.image2scanner_dir (y_dir);

            Point<> x_width = projection.screen_to_model_direction (projection.width()/2.0, 0.0, projection.depth_of (pos));
            int nx = std::ceil (x_width.norm() / x_dir.norm());
            Point<> y_width = projection.screen_to_model_direction (0.0, projection.height()/2.0, projection.depth_of (pos));
            int ny = std::ceil (y_width.norm() / y_dir.norm());

            Math::Vector<float> values (Math::SH::NforL (settings->lmax));
            Math::Vector<float> r_del_daz;

            for (int y = -ny; y <= ny; ++y) {
              for (int x = -nx; x <= nx; ++x) {
                Point<> p = pos + float(x)*x_dir + float(y)*y_dir;
                get_values (values, settings->image, p, interpolation_box->isChecked());
                if (!std::isfinite (values[0])) continue;
                if (values[0] == 0.0) continue;
                renderer->compute_r_del_daz (r_del_daz, values.sub (0, Math::SH::NforL (lmax)));
                renderer->set_data (r_del_daz);
                renderer->draw (p);
              }
            }

            renderer->stop();

            gl::Disable (gl::DEPTH_TEST);
            gl::DepthMask (gl::FALSE_);
          }
        }







        inline ODF::Image* ODF::get_image ()
        {
          QModelIndexList list = image_list_view->selectionModel()->selectedRows();
          if (!list.size())
            return nullptr;
          return image_list_model->get_image (list[0]);
        }






        void ODF::get_values (Math::Vector<float>& values, MRView::Image& image, const Point<>& pos, const bool interp)
        {
          Point<> p = image.interp.scanner2voxel (pos);
          if (!interp) {
            p[0] = std::round (p[0]);
            p[1] = std::round (p[1]);
            p[2] = std::round (p[2]);
          }
          image.interp.voxel (p);
          values.zero();
          for (image.interp[3] = 0; image.interp[3] < std::min (ssize_t(values.size()), image.interp.dim(3)); ++image.interp[3])
            values[image.interp[3]] = image.interp.value().real(); 
        }





        void ODF::add_images (std::vector<std::string>& list)
        {
          size_t previous_size = image_list_model->rowCount();
          if (!image_list_model->add_items (list,
                lmax_selector->value(), 
                colour_by_direction_box->isChecked(), 
                hide_negative_lobes_box->isChecked(), 
                scale->value())) 
            return;
          QModelIndex first = image_list_model->index (previous_size, 0, QModelIndex());
          image_list_view->selectionModel()->select (first, QItemSelectionModel::ClearAndSelect);
          updateGL();
        }



        void ODF::showEvent (QShowEvent*)
        {
          connect (&window, SIGNAL (focusChanged()), this, SLOT (onWindowChange()));
          connect (&window, SIGNAL (targetChanged()), this, SLOT (onWindowChange()));
          connect (&window, SIGNAL (orientationChanged()), this, SLOT (onWindowChange()));
          connect (&window, SIGNAL (planeChanged()), this, SLOT (onWindowChange()));
          onWindowChange();
        }

        void ODF::closeEvent (QCloseEvent*) {
          window.disconnect (this);
        }




        void ODF::onWindowChange ()
        {
          update_preview();
        }



        void ODF::onPreviewClosed ()
        {
          show_preview_button->setChecked (false);
        }





        void ODF::image_open_slot ()
        {
          std::vector<std::string> list = Dialog::File::get_images (&window, "Select overlay images to open");
          if (list.empty())
            return;

          add_images (list);
        }






        void ODF::image_close_slot ()
        {
          QModelIndexList indexes = image_list_view->selectionModel()->selectedIndexes();
          if (indexes.size())
            image_list_model->remove_item (indexes.first());
          updateGL();
        }


        void ODF::show_preview_slot ()
        {
          preview->show();
          preview->raise();
          update_preview();
        }


        void ODF::hide_all_slot ()
        {
          window.updateGL();
        }



        void ODF::colour_by_direction_slot (int) 
        { 
          Image* settings = get_image();
          if (!settings)
            return;
          settings->color_by_direction = colour_by_direction_box->isChecked();
          assert (preview);
          assert (preview->tool);
          dynamic_cast<ODF_Preview*>(preview->tool)->render_frame->set_color_by_dir (colour_by_direction_box->isChecked());
          updateGL();
        }





        void ODF::hide_negative_lobes_slot (int) 
        {
          Image* settings = get_image();
          if (!settings)
            return;
          settings->hide_negative_lobes = hide_negative_lobes_box->isChecked();
          assert (preview);
          assert (preview->tool);
          dynamic_cast<ODF_Preview*>(preview->tool)->render_frame->set_hide_neg_lobes (hide_negative_lobes_box->isChecked());
          updateGL();
        }







        void ODF::lmax_slot (int) 
        { 
          Image* settings = get_image();
          if (!settings)
            return;
          settings->lmax = lmax_selector->value();
          assert (preview);
          assert (preview->tool);
          dynamic_cast<ODF_Preview*>(preview->tool)->render_frame->set_lmax (lmax_selector->value());
          updateGL();
        }



        void ODF::use_lighting_slot (int)
        {
          assert (preview);
          assert (preview->tool);
          dynamic_cast<ODF_Preview*>(preview->tool)->render_frame->set_use_lighting (use_lighting_box->isChecked());
          updateGL();
        }




        void ODF::lighting_settings_slot (bool)
        {
          if (!lighting_dialog)
            lighting_dialog = new Dialog::Lighting (&window, "Advanced ODF lighting", *lighting);
          lighting_dialog->show();
        }





        void ODF::adjust_scale_slot () 
        { 
          scale->setRate (0.01 * scale->value());
          Image* settings = get_image();
          if (!settings)
            return;
          settings->scale = scale->value();
          assert (preview);
          assert (preview->tool);
          dynamic_cast<ODF_Preview*>(preview->tool)->render_frame->set_scale (scale->value());
          updateGL();
        }

        void ODF::hide_event ()
        {
          assert (preview);
          preview->hide();
          if (lighting_dialog)
            lighting_dialog->hide();
        }



        void ODF::updateGL () 
        {
          if (!hide_all_button->isChecked())
            window.updateGL();
        }

        void ODF::update_preview()
        {
          assert (preview);
          if (!preview->isVisible())
            return;
          Image* settings = get_image();
          if (!settings)
            return;
          MRView::Image& image (settings->image);
          ODF_Preview* preview_tool = dynamic_cast<ODF_Preview*>(preview->tool);
          Math::Vector<float> values (Math::SH::NforL (lmax_selector->value()));
          get_values (values, image, window.focus(), preview_tool->interpolate());
          preview_tool->set (values);
        }




        void ODF::selection_changed_slot (const QItemSelection &, const QItemSelection &)
        {
          Image* settings = get_image();
          if (!settings)
            return;
          scale->setValue (settings->scale);
          lmax_selector->setValue (settings->lmax);
          hide_negative_lobes_box->setChecked (settings->hide_negative_lobes);
          colour_by_direction_box->setChecked (settings->color_by_direction);
          updateGL();
          update_preview();
        }





        void ODF::add_commandline_options (MR::App::OptionList& options) 
        { 
          using namespace MR::App;
          options
            + OptionGroup ("ODF tool options")

            + Option ("odf.load", "Loads the specified image on the ODF tool.")
            +   Argument ("image").type_image_in();
            
        }





        bool ODF::process_commandline_option (const MR::App::ParsedOption& opt) 
        {
          if (opt.opt->is ("odf.load")) {
            try {
              std::vector<std::string> list (1, opt[0]);
              add_images (list);
            }
            catch (Exception& e) {
              e.display();
            }
            return true;
          }

          return false;
        }



      }
    }
  }
}





