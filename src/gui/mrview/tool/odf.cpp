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
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "dwi/directions/set.h"
#include "gui/dialog/file.h"
#include "gui/lighting_dock.h"
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
            Image (MR::Header&& H, const float scale, const bool hide_negative, const bool color_by_direction) :
                image (std::move (H)),
                is_SH (true),
                lmax (Math::SH::LforN (image.header().size (3))),
                scale (scale),
                hide_negative (hide_negative),
                color_by_direction (color_by_direction),
                dixel (image.header())
            {
              // Make an informed guess as to whether or not this is an SH image
              // If it's not, try to initialise the dixel plugin
              try {
                Math::SH::check (image.header());
                DEBUG ("Image " + image.header().name() + " initialised as SH ODF");
              } catch (...) {
                lmax = -1;
                is_SH = false;
                try {
                  if (!dixel.shells)
                    throw Exception ("No shell data");
                  dixel.set_shell (dixel.shells->count() - 1);
                  DEBUG ("Image " + image.header().name() + " initialised as dixel ODF using DW scheme");
                } catch (...) {
                  try {
                    dixel.set_header();
                    DEBUG ("Image " + image.header().name() + " initialised as dixel ODF using header directions field");
                  } catch (...) {
                    try {
                      dixel.set_internal (image.header().size (3));
                      DEBUG ("Image " + image.header().name() + " initialised as dixel ODF using internal direction set");
                    } catch (...) {
                      DEBUG ("Image " + image.header().name() + " left uninitialised in ODF tool");
                    }
                  }
                }
              }
            }

            bool valid() const {
              if (is_SH)
                return true;
              if (!dixel.dirs)
                return false;
              return dixel.dirs->size();
            }

            MRView::Image image;
            bool is_SH;
            int lmax;
            float scale;
            bool hide_negative, color_by_direction;

            class DixelPlugin
            {
              public:
                enum dir_t { DW_SCHEME, HEADER, INTERNAL, NONE, FILE };

                DixelPlugin (const MR::Header& H) :
                    dir_type (dir_t::NONE),
                    shell_index (0)
                {
                  try {
                    grad = MR::DWI::get_valid_DW_scheme (H);
                    shells.reset (new MR::DWI::Shells (grad));
                    shell_index = shells->count() - 1;
                  } catch (...) { }
                  auto entry = H.keyval().find ("directions");
                  if (entry != H.keyval().end()) {
                    try {
                      const auto lines = split_lines (entry->second);
                      if (lines.size() != size_t(H.size (3)))
                        throw Exception ("malformed directions field in image \"" + H.name() + "\" - incorrect number of rows");
                      for (size_t row = 0; row < lines.size(); ++row) {
                        const auto values = parse_floats (lines[row]);
                        if (!header_dirs.rows()) {
                          if (values.size() != 2 && values.size() != 3)
                            throw Exception ("malformed directions field in image \"" + H.name() + "\" - should have 2 or 3 columns");
                          header_dirs.resize (lines.size(), values.size());
                        } else if (values.size() != size_t(header_dirs.cols())) {
                          header_dirs.resize (0, 0);
                          throw Exception ("malformed directions field in image \"" + H.name() + "\" - variable number of columns");
                        }
                        for (size_t col = 0; col < values.size(); ++col)
                          header_dirs(row, col) = values[col];
                      }
                    } catch (Exception& e) {
                      DEBUG (e[0]);
                      header_dirs.resize (0, 0);
                    }
                  }
                }

                void set_shell (size_t index) {
                  if (!shells)
                    throw Exception ("No valid DW scheme defined in header");
                  if (index >= shells->count())
                    throw Exception ("Shell index is outside valid range");
                  Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> shell_dirs ((*shells)[index].count(), 3);
                  const std::vector<size_t>& volumes = (*shells)[index].get_volumes();
                  for (size_t row = 0; row != volumes.size(); ++row)
                    shell_dirs.row (row) = grad.row (volumes[row]).head<3>().cast<float>();
                  std::unique_ptr<MR::DWI::Directions::Set> new_dirs (new MR::DWI::Directions::Set (shell_dirs));
                  std::swap (dirs, new_dirs);
                  shell_index = index;
                  dir_type = DixelPlugin::dir_t::DW_SCHEME;
                }

                void set_header() {
                  if (!header_dirs.rows())
                    throw Exception ("No direction scheme defined in header");
                  std::unique_ptr<MR::DWI::Directions::Set> new_dirs (new MR::DWI::Directions::Set (header_dirs));
                  std::swap (dirs, new_dirs);
                  dir_type = DixelPlugin::dir_t::HEADER;
                }

                void set_internal (const size_t n) {
                  std::unique_ptr<MR::DWI::Directions::Set> new_dirs (new MR::DWI::Directions::Set (n));
                  std::swap (dirs, new_dirs);
                  dir_type = DixelPlugin::dir_t::INTERNAL;
                }

                void set_none() {
                  if (dirs)
                    delete dirs.release();
                  dir_type = DixelPlugin::dir_t::NONE;
                }

                void set_from_file (const std::string& path)
                {
                  std::unique_ptr<MR::DWI::Directions::Set> new_dirs (new MR::DWI::Directions::Set (path));
                  std::swap (dirs, new_dirs);
                  dir_type = DixelPlugin::dir_t::FILE;
                }

                Eigen::VectorXf get_shell_data (const Eigen::VectorXf& values) const
                {
                  assert (shells);
                  const std::vector<size_t>& volumes ((*shells)[shell_index].get_volumes());
                  Eigen::VectorXf result (volumes.size());
                  for (size_t i = 0; i != volumes.size(); ++i)
                    result[i] = values[volumes[i]];
                  return result;
                }

                size_t num_DW_shells() const {
                  if (!shells) return 0;
                  return shells->count();
                }

                dir_t dir_type;
                Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> header_dirs;
                Eigen::Matrix<double, Eigen::Dynamic, 4> grad;
                std::unique_ptr<MR::DWI::Shells> shells;
                size_t shell_index;
                std::unique_ptr<MR::DWI::Directions::Set> dirs;

            } dixel;
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

            size_t add_items (const std::vector<std::string>& list, bool colour_by_direction, bool hide_negative_lobes, float scale) {
              std::vector<std::unique_ptr<MR::Header>> hlist;
              for (size_t i = 0; i < list.size(); ++i) {
                try {
                  std::unique_ptr<MR::Header> header (new MR::Header (MR::Header::open (list[i])));
                  hlist.push_back (std::move (header));
                }
                catch (Exception& E) {
                  E.display();
                }
              }

              if (hlist.size()) {
                beginInsertRows (QModelIndex(), items.size(), items.size()+hlist.size());
                for (size_t i = 0; i < hlist.size(); ++i) 
                  items.push_back (std::unique_ptr<Image> (new Image (std::move (*hlist[i]), scale, hide_negative_lobes, colour_by_direction)));
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
              return index.isValid() ? dynamic_cast<Image*>(items[index.row()].get()) : NULL;
            }

            std::vector<std::unique_ptr<Image>> items;
        };








        ODF::ODF (Dock* parent) :
          Base (parent),
          preview (nullptr),
          renderer (nullptr),
          lighting_dock (nullptr),
          lmax (0),
          level_of_detail (0) {
            lighting = new GL::Lighting (this);

            VBoxLayout *main_box = new VBoxLayout (this);

            HBoxLayout* layout = new HBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);

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

            QLabel* label = new QLabel ("data type");
            label->setAlignment (Qt::AlignHCenter);
            box_layout->addWidget (label, 0, 0);
            type_selector = new QComboBox (this);
            type_selector->addItem ("SH");
            type_selector->addItem ("amps");
            connect (type_selector, SIGNAL (currentIndexChanged(int)), this, SLOT(type_change_slot()));
            box_layout->addWidget (type_selector, 0, 1);

            level_of_detail_label = new QLabel ("detail");
            level_of_detail_label->setAlignment (Qt::AlignHCenter);
            box_layout->addWidget (level_of_detail_label, 1, 0);
            level_of_detail_selector = new SpinBox (this);
            level_of_detail_selector->setMinimum (1);
            level_of_detail_selector->setMaximum (6);
            level_of_detail_selector->setSingleStep (1);
            level_of_detail_selector->setValue (3);
            connect (level_of_detail_selector, SIGNAL (valueChanged(int)), this, SLOT(updateGL()));
            box_layout->addWidget (level_of_detail_selector, 1, 1);

            lmax_label = new QLabel ("lmax");
            lmax_label->setAlignment (Qt::AlignHCenter);
            box_layout->addWidget (lmax_label, 1, 2);
            lmax_selector = new SpinBox (this);
            lmax_selector->setMinimum (2);
            lmax_selector->setMaximum (16);
            lmax_selector->setSingleStep (2);
            lmax_selector->setValue (8);
            connect (lmax_selector, SIGNAL (valueChanged(int)), this, SLOT(lmax_slot(int)));
            box_layout->addWidget (lmax_selector, 1, 3);

            dirs_label = new QLabel ("directions");
            dirs_label->setAlignment (Qt::AlignHCenter);
            dirs_label->setVisible (false);
            box_layout->addWidget (dirs_label, 2, 0);
            dirs_selector = new QComboBox (this);
            dirs_selector->addItem ("DW scheme");
            dirs_selector->addItem ("Header");
            dirs_selector->addItem ("Internal");
            dirs_selector->addItem ("None");
            dirs_selector->addItem ("From file");
            dirs_selector->setVisible (false);
            connect (dirs_selector, SIGNAL (currentIndexChanged(int)), this, SLOT(dirs_slot()));
            box_layout->addWidget (dirs_selector, 2, 1);

            shell_label = new QLabel ("shell");
            shell_label->setAlignment (Qt::AlignHCenter);
            shell_label->setVisible (false);
            box_layout->addWidget (shell_label, 2, 2);
            shell_selector = new QComboBox (this);
            shell_selector->setVisible (false);
            connect (shell_selector, SIGNAL (currentIndexChanged(int)), this, SLOT(shell_slot()));
            box_layout->addWidget (shell_selector, 2, 3);

            label = new QLabel ("scale");
            label->setAlignment (Qt::AlignHCenter);
            box_layout->addWidget (label, 3, 0);
            scale = new AdjustButton (this, 1.0);
            scale->setValue (1.0);
            scale->setMin (0.0);
            connect (scale, SIGNAL (valueChanged()), this, SLOT (adjust_scale_slot()));
            box_layout->addWidget (scale, 3, 1, 1, 3);
            
            interpolation_box = new QCheckBox ("interpolation");
            interpolation_box->setChecked (true);
            connect (interpolation_box, SIGNAL (stateChanged(int)), this, SLOT (updateGL()));
            box_layout->addWidget (interpolation_box, 4, 0, 1, 2);

            hide_negative_values_box = new QCheckBox ("hide negative values");
            hide_negative_values_box->setChecked (true);
            connect (hide_negative_values_box, SIGNAL (stateChanged(int)), this, SLOT (hide_negative_values_slot(int)));
            box_layout->addWidget (hide_negative_values_box, 4, 2, 1, 2);

            lock_to_grid_box = new QCheckBox ("lock to grid");
            lock_to_grid_box->setChecked (true);
            connect (lock_to_grid_box, SIGNAL (stateChanged(int)), this, SLOT (updateGL()));
            box_layout->addWidget (lock_to_grid_box, 5, 0, 1, 2);

            colour_by_direction_box = new QCheckBox ("colour by direction");
            colour_by_direction_box->setChecked (true);
            connect (colour_by_direction_box, SIGNAL (stateChanged(int)), this, SLOT (colour_by_direction_slot(int)));
            box_layout->addWidget (colour_by_direction_box, 5, 2, 1, 2);

            main_grid_box = new QCheckBox ("use main grid");
            main_grid_box->setToolTip (tr ("Show individual ODFs at the spatial resolution of the main image instead of the ODF image's own spatial resolution"));
            main_grid_box->setChecked (false);
            connect (main_grid_box, SIGNAL (stateChanged(int)), this, SLOT (updateGL()));
            box_layout->addWidget (main_grid_box, 6, 0, 1, 2);

            use_lighting_box = new QCheckBox ("use lighting");
            use_lighting_box->setCheckable (true);
            use_lighting_box->setChecked (true);
            connect (use_lighting_box, SIGNAL (stateChanged(int)), this, SLOT (use_lighting_slot(int)));
            box_layout->addWidget (use_lighting_box, 6, 2, 1, 2);

            QPushButton *lighting_settings_button = new QPushButton ("ODF colour and lighting...", this);
            connect (lighting_settings_button, SIGNAL(clicked(bool)), this, SLOT (lighting_settings_slot (bool)));
            box_layout->addWidget (lighting_settings_button, 7, 0, 1, 4);


            connect (image_list_view->selectionModel(),
                SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
                SLOT (selection_changed_slot(const QItemSelection &, const QItemSelection &)) );

            connect (lighting, SIGNAL (changed()), this, SLOT (updateGL()));


            renderer = new DWI::Renderer;
            renderer->initGL();


            hide_negative_values_slot (0);
            colour_by_direction_slot (0);
            lmax_slot (0);
            adjust_scale_slot ();

          }

        ODF::~ODF()
        {
          if (renderer) {
            delete renderer;
            renderer = nullptr;
          }
          if (preview) {
            delete preview;
            preview = nullptr;
          }
          if (lighting_dock) {
            delete lighting_dock;
            lighting_dock = nullptr;
          }
        }






        void ODF::draw (const Projection& projection, bool is_3D, int, int)
        {
          if (is_3D) 
            return;

          Image* settings = get_image();
          if (!settings)
            return;

          if (!settings->is_SH && settings->dixel.dir_type == Image::DixelPlugin::dir_t::NONE)
            return;

          MRView::Image& image (main_grid_box->isChecked() ? *window().image() : settings->image);

          if (!hide_all_button->isChecked()) {

            if (settings->is_SH) {
              if (lmax != settings->lmax || level_of_detail != level_of_detail_selector->value()) {
                lmax = settings->lmax;
                level_of_detail = level_of_detail_selector->value();
                renderer->sh.update_mesh (level_of_detail, lmax);
              }
            }

            renderer->set_mode (settings->is_SH);

            renderer->start (projection, *lighting, settings->scale, 
                use_lighting_box->isChecked(), settings->color_by_direction, settings->hide_negative, true);

            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);

            Eigen::Vector3f pos (window().target());
            pos += projection.screen_normal() * (projection.screen_normal().dot (window().focus() - window().target()));
            if (lock_to_grid_box->isChecked()) {
              Eigen::Vector3f p = image.transform().scanner2voxel.cast<float>() * pos;
              p[0] = std::round (p[0]);
              p[1] = std::round (p[1]);
              p[2] = std::round (p[2]);
              pos = image.transform().voxel2scanner.cast<float>() * p;
            }

            Eigen::Vector3f x_dir = projection.screen_to_model_direction (1.0, 0.0, projection.depth_of (pos));
            x_dir.normalize();
            x_dir = image.transform().scanner2image.rotation().cast<float>() * x_dir;
            x_dir[0] *= image.header().spacing (0);
            x_dir[1] *= image.header().spacing (1);
            x_dir[2] *= image.header().spacing (2);
            x_dir = image.transform().image2scanner.rotation().cast<float>() * x_dir;

            Eigen::Vector3f y_dir = projection.screen_to_model_direction (0.0, 1.0, projection.depth_of (pos));
            y_dir.normalize();
            y_dir = image.transform().scanner2image.rotation().cast<float>() * y_dir;
            y_dir[0] *= image.header().spacing (0);
            y_dir[1] *= image.header().spacing (1);
            y_dir[2] *= image.header().spacing (2);
            y_dir = image.transform().image2scanner.rotation().cast<float>() * y_dir;

            const Eigen::Vector3f x_width = projection.screen_to_model_direction (projection.width()/2.0, 0.0, projection.depth_of (pos));
            const int nx = std::ceil (x_width.norm() / x_dir.norm());
            const Eigen::Vector3f y_width = projection.screen_to_model_direction (0.0, projection.height()/2.0, projection.depth_of (pos));
            const int ny = std::ceil (y_width.norm() / y_dir.norm());

            Eigen::VectorXf values (settings->is_SH ? Math::SH::NforL (settings->lmax) : settings->image.header().size (3));
            Eigen::VectorXf r_del_daz;

            for (int y = -ny; y <= ny; ++y) {
              for (int x = -nx; x <= nx; ++x) {
                Eigen::Vector3f p = pos + float(x)*x_dir + float(y)*y_dir;
                get_values (values, settings->image, p, interpolation_box->isChecked());
                if (!std::isfinite (values[0])) continue;
                if (values[0] == 0.0) continue;
                if (settings->is_SH) {
                  renderer->sh.compute_r_del_daz (r_del_daz, values.topRows (Math::SH::NforL (lmax)));
                  renderer->sh.set_data (r_del_daz);
                } else {
                  if (settings->dixel.dir_type == Image::DixelPlugin::dir_t::DW_SCHEME) {
                    const Eigen::VectorXf shell_values = settings->dixel.get_shell_data (values);
                    renderer->dixel.set_data (shell_values);
                  } else {
                    renderer->dixel.set_data (values);
                  }
                }
                GL_CHECK_ERROR;
                renderer->draw (p);
                GL_CHECK_ERROR;
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






        void ODF::get_values (Eigen::VectorXf& values, MRView::Image& image, const Eigen::Vector3f& pos, const bool interp)
        {
          values.setZero();
          if (interp) {
            if (image.linear_interp.scanner (pos))
              return;
            for (image.linear_interp.index(3) = 0; image.linear_interp.index(3) < std::min (ssize_t(values.size()), image.linear_interp.size(3)); ++image.linear_interp.index(3))
              values[image.linear_interp.index(3)] = image.linear_interp.value().real();
          } else {
            if (image.nearest_interp.scanner (pos))
              return;
            for (image.nearest_interp.index(3) = 0; image.nearest_interp.index(3) < std::min (ssize_t(values.size()), image.nearest_interp.size(3)); ++image.nearest_interp.index(3))
              values[image.nearest_interp.index(3)] = image.nearest_interp.value().real();
          }
        }




        void ODF::setup_ODFtype_UI (const Image* image)
        {
          assert (image);
          type_selector->blockSignals (true);
          type_selector->setCurrentIndex (image->is_SH ? 0 : 1);
          type_selector->setEnabled (image->lmax >= 0);
          type_selector->blockSignals (false);
          lmax_label->setVisible (image->is_SH);
          level_of_detail_label->setVisible (image->is_SH);
          lmax_selector->setVisible (image->is_SH);
          if (image->lmax >= 0)
            lmax_selector->setValue (image->lmax);
          level_of_detail_selector->setVisible (image->is_SH);
          dirs_label->setVisible (!image->is_SH);
          shell_label->setVisible (!image->is_SH);
          dirs_selector->setVisible (!image->is_SH);
          if (!image->is_SH)
            dirs_selector->setCurrentIndex (image->dixel.dir_type);
          shell_selector->setVisible (!image->is_SH);
          shell_selector->blockSignals (true);
          shell_selector->clear();
          if (image->dixel.shells) {
            for (size_t i = 0; i != image->dixel.shells->count(); ++i)
              shell_selector->addItem (QString::fromStdString (str (int (std::round ((*image->dixel.shells)[i].get_mean())))));
          }
          shell_selector->blockSignals (false);
          if (!image->is_SH && shell_selector->count() && image->dixel.dir_type == Image::DixelPlugin::dir_t::DW_SCHEME)
            shell_selector->setCurrentIndex (image->dixel.shell_index);
          shell_selector->setEnabled (!image->is_SH && image->dixel.dir_type == Image::DixelPlugin::dir_t::DW_SCHEME && image->dixel.shells && image->dixel.shells->count() > 1);
        }





        void ODF::add_images (std::vector<std::string>& list)
        {
          size_t previous_size = image_list_model->rowCount();
          if (!image_list_model->add_items (list,
                                            colour_by_direction_box->isChecked(),
                                            hide_negative_values_box->isChecked(),
                                            scale->value()))
            return;
          QModelIndex first = image_list_model->index (previous_size, 0, QModelIndex());
          image_list_view->selectionModel()->select (first, QItemSelectionModel::ClearAndSelect);
          Image* settings = get_image();
          setup_ODFtype_UI (settings);
          assert (renderer);
          if (!settings->is_SH) {
            assert (settings->dixel.dirs);
            renderer->dixel.update_mesh (*(settings->dixel.dirs));
          }
          updateGL();
        }



        void ODF::showEvent (QShowEvent*)
        {
          connect (&window(), SIGNAL (focusChanged()), this, SLOT (onWindowChange()));
          connect (&window(), SIGNAL (targetChanged()), this, SLOT (onWindowChange()));
          connect (&window(), SIGNAL (orientationChanged()), this, SLOT (onWindowChange()));
          connect (&window(), SIGNAL (planeChanged()), this, SLOT (onWindowChange()));
          onWindowChange();
        }

        void ODF::closeEvent (QCloseEvent*) {
          window().disconnect (this);
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
          std::vector<std::string> list = Dialog::File::get_images (&window(), "Select overlay images to open");
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
          if (!preview) {
            preview = new Preview (this);
            connect (lighting, SIGNAL (changed()), preview, SLOT (lighting_update_slot()));
          }

          preview->show();
          update_preview();
        }


        void ODF::hide_all_slot ()
        {
          window().updateGL();
        }



        void ODF::colour_by_direction_slot (int) 
        { 
          Image* settings = get_image();
          if (!settings)
            return;
          settings->color_by_direction = colour_by_direction_box->isChecked();
          if (preview) {
            preview->render_frame->set_color_by_dir (colour_by_direction_box->isChecked());
          }
          updateGL();
        }





        void ODF::hide_negative_values_slot (int)
        {
          Image* settings = get_image();
          if (!settings)
            return;
          settings->hide_negative = hide_negative_values_box->isChecked();
          if (preview) 
            preview->render_frame->set_hide_neg_lobes (hide_negative_values_box->isChecked());
          updateGL();
        }





        void ODF::type_change_slot ()
        {
          Image* settings = get_image();
          if (!settings)
            return;
          const bool mode_is_SH = !type_selector->currentIndex();
          if (settings->is_SH == mode_is_SH)
            return;
          settings->is_SH = mode_is_SH;
          // If image isn't compatible with SH, this shouldn't be accessible in the UI
          if (mode_is_SH)
            assert (settings->lmax);
          setup_ODFtype_UI (settings);
          updateGL();
        }



        void ODF::lmax_slot (int) 
        { 
          Image* settings = get_image();
          if (!settings)
            return;
          assert (settings->is_SH);
          settings->lmax = lmax_selector->value();
          if (preview) 
            preview->render_frame->set_lmax (lmax_selector->value());
          updateGL();
        }



        void ODF::dirs_slot ()
        {
          Image* settings = get_image();
          if (!settings)
            return;
          assert (!settings->is_SH);
          const size_t mode = dirs_selector->currentIndex();
          if (mode == settings->dixel.dir_type)
            return;
          try {
            switch (mode) {
              case 0: // DW scheme
                if (!settings->dixel.num_DW_shells())
                  throw Exception ("Cannot draw orientation information from DW scheme: no such scheme stored in header");
                settings->dixel.set_shell (settings->dixel.shell_index);
                break;
              case 1: // Header
                if (!settings->dixel.header_dirs.rows())
                  throw Exception ("Cannot draw orientation information from header: no such data exist");
                settings->dixel.set_header();
                break;
              case 2: // Internal
                settings->dixel.set_internal (settings->image.header().size (3));
                break;
              case 3: // None
                settings->dixel.set_none();
                break;
              case 4: // From file
                const std::string path = Dialog::File::get_file (this, "Select directions file", "Text files (*.txt)");
                if (!path.size()) {
                  dirs_selector->setCurrentIndex (settings->dixel.dir_type);
                  return;
                }
                settings->dixel.set_from_file (path);
                break;
            }
            shell_selector->setEnabled (mode == 0 && settings->dixel.shells && settings->dixel.shells->count() > 1);
            if (mode != 3) {
              assert (settings->dixel.dirs);
              assert (renderer);
              renderer->dixel.update_mesh (*(settings->dixel.dirs));
            }
          } catch (Exception& e) {
            e.display();
            dirs_selector->setCurrentIndex (settings->dixel.dir_type);
          }

          updateGL();
        }



        void ODF::shell_slot ()
        {
          Image* settings = get_image();
          if (!settings)
            return;
          assert (!settings->is_SH);
          assert (settings->dixel.dir_type == Image::DixelPlugin::dir_t::DW_SCHEME);
          const size_t index = shell_selector->currentIndex();
          assert (index < settings->dixel.num_DW_shells());
          settings->dixel.set_shell (index);
          assert (settings->dixel.dirs);
          assert (renderer);
          renderer->dixel.update_mesh (*(settings->dixel.dirs));
          updateGL();
        }



        void ODF::adjust_scale_slot ()
        {
          scale->setRate (0.01 * scale->value());
          Image* settings = get_image();
          if (!settings)
            return;
          settings->scale = scale->value();
          if (preview)
            preview->render_frame->set_scale (scale->value());
          updateGL();
        }



        void ODF::use_lighting_slot (int)
        {
          if (preview) 
            preview->render_frame->set_use_lighting (use_lighting_box->isChecked());
          updateGL();
        }




        void ODF::lighting_settings_slot (bool)
        {
          if (!lighting_dock) {
            lighting_dock = new LightingDock("Advanced ODF lighting", *lighting);
            window().addDockWidget (Qt::RightDockWidgetArea, lighting_dock);
          }
          lighting_dock->show();
        }






        void ODF::close_event ()
        {
          if (preview) 
            preview->hide();
          if (lighting_dock)
            lighting_dock->hide();
        }



        void ODF::updateGL () 
        {
          if (!hide_all_button->isChecked())
            window().updateGL();
        }

        void ODF::update_preview()
        {
          if (!preview) 
            return;
          if (!preview->isVisible())
            return;
          Image* settings = get_image();
          if (!settings)
            return;
          MRView::Image& image (settings->image);
          Eigen::VectorXf values (settings->is_SH ? Math::SH::NforL (lmax_selector->value()) : settings->image.header().size (3));
          get_values (values, image, window().focus(), preview->interpolate());
          preview->set (values);
        }




        void ODF::selection_changed_slot (const QItemSelection &, const QItemSelection &)
        {
          Image* settings = get_image();
          if (!settings)
            return;
          if (!settings->is_SH)
            renderer->dixel.update_mesh (*(settings->dixel.dirs));
          setup_ODFtype_UI (settings);
          scale->setValue (settings->scale);
          hide_negative_values_box->setChecked (settings->hide_negative);
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





