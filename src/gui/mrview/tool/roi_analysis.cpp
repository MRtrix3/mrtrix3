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

//#define GL_DEBUG

#include <iomanip>

#include "mrtrix.h"
#include "gui/mrview/window.h"
#include "gui/mrview/tool/roi_analysis.h"
#include "gui/mrview/volume.h"
#include "gui/dialog/file.h"
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

            
       namespace {
         constexpr std::array<std::array<GLubyte,3>,6> preset_colours = {
           255, 255, 0,
           255, 0, 255,
           0, 255, 255,
           255, 0, 0,
           0, 255, 255,
           0, 0, 255
         };
       }


        class ROI::Item : public Volume {
          public:
            template <class InfoType>
              Item (const InfoType& src) : 
                Volume (src),
                current_undo (-1),
                saved (true) {
                  type = gl::UNSIGNED_BYTE;
                  format = gl::RED;
                  internal_format = gl::R8;
                  set_allowed_features (false, true, false);
                  set_interpolate (false);
                  set_use_transparency (true);
                  set_min_max (0.0, 1.0);
                  set_windowing (-1.0f, 0.0f);
                  alpha = 1.0f;
                  colour = preset_colours[current_preset_colour++];
                  if (current_preset_colour >= 6)
                    current_preset_colour = 0;
                  transparent_intensity = 0.4f;
                  opaque_intensity = 0.6f;
                  colourmap = ColourMap::index ("Colour");
                  float voxsize = std::min (src.vox(0), std::min (src.vox(1), src.vox(2)));
                  brush_size = min_brush_size = voxsize;
                  max_brush_size = 100.0f*min_brush_size;

                  std::stringstream name;
                  name << "ROI" << std::setfill('0') << std::setw(5) << new_roi_counter++ << ".mif";
                  filename = name.str();

                  bind();
                  allocate();
                }

            void zero () {
              bind();
              std::vector<GLubyte> data (info().dim(0)*info().dim(1));
              for (int n = 0; n < info().dim(2); ++n)
                upload_data ({ 0, 0, n }, { info().dim(0), info().dim(1), 1 }, reinterpret_cast<void*> (&data[0]));
            }

            void load (const MR::Image::Header& header) {
              bind();
              MR::Image::Buffer<bool> buffer (header);
              auto vox = buffer.voxel();
              std::vector<GLubyte> data (vox.dim(0)*vox.dim(1));
              ProgressBar progress ("loading ROI image \"" + header.name() + "\"...");
              for (auto outer = MR::Image::Loop(2,3) (vox); outer; ++outer) {
                auto p = data.begin();
                for (auto inner = MR::Image::Loop (0,2) (vox); inner; ++inner) 
                  *(p++) = vox.value();
                upload_data ({ 0, 0, vox[2] }, { vox.dim(0), vox.dim(1), 1 }, reinterpret_cast<void*> (&data[0]));
                ++progress;
              }
              filename = header.name();
            }


            template <class VoxelType> 
              void save (VoxelType&& vox, GLubyte* data) {
                for (auto l = MR::Image::Loop() (vox); l; ++l) 
                  vox.value() = data[vox[0] + vox.dim(0) * (vox[1] + vox.dim(1)*vox[2])];
                saved = true;
                filename = vox.name();
              }





            struct UndoEntry {

              UndoEntry (Item& roi, int current_axis, int current_slice) 
              {
                from = {0, 0, 0}; 
                from[current_axis] = current_slice;
                size = { roi.info().dim(0), roi.info().dim(1), roi.info().dim(2) };
                size[current_axis] = 1;

                if (current_axis == 0) { slice_axes[0] = 1; slice_axes[1] = 2; }
                else if (current_axis == 1) { slice_axes[0] = 0; slice_axes[1] = 2; }
                else { slice_axes[0] = 0; slice_axes[1] = 1; }
                tex_size = { roi.info().dim(slice_axes[0]), roi.info().dim(slice_axes[1]) };

                if (!copy_program) {
                  GL::Shader::Vertex vertex_shader (
                      "layout(location = 0) in ivec3 vertpos;\n"
                      "void main() {\n"
                      "  gl_Position = vec4 (vertpos,1);\n"
                      "}\n");
                  GL::Shader::Fragment fragment_shader (
                      "uniform isampler3D tex;\n"
                      "uniform ivec3 position;\n"
                      "uniform ivec2 axes;\n"
                      "layout (location = 0) out vec3 color0;\n"
                      "void main() {\n"
                      "  ivec3 pos = position;\n"
                      "  pos[axes.x] = int(gl_FragCoord.x);\n"
                      "  pos[axes.y] = int(gl_FragCoord.y);\n"
                      "  color0.r = texelFetch (tex, pos, 0).r;\n"
                      "}\n");

                  copy_program.attach (vertex_shader);
                  copy_program.attach (fragment_shader);
                  copy_program.link();
                }

                if (!copy_vertex_array_object) {
                  copy_vertex_buffer.gen();
                  copy_vertex_array_object.gen();

                  copy_vertex_buffer.bind (gl::ARRAY_BUFFER);
                  copy_vertex_array_object.bind();

                  gl::EnableVertexAttribArray (0);
                  gl::VertexAttribIPointer (0, 3, gl::INT, 3*sizeof(GLint), (void*)0);

                  GLint vertices[12] = { 
                    -1, -1, 0,
                    -1, 1, 0,
                    1, 1, 0,
                    1, -1, 0,
                  };
                  gl::BufferData (gl::ARRAY_BUFFER, sizeof(vertices), vertices, gl::STREAM_DRAW);
                }
                else copy_vertex_array_object.bind();

                from = {0, 0, 0}; 
                from[current_axis] = current_slice;

                if (current_axis == 0) { slice_axes[0] = 1; slice_axes[1] = 2; }
                else if (current_axis == 1) { slice_axes[0] = 0; slice_axes[1] = 2; }
                else { slice_axes[0] = 0; slice_axes[1] = 1; }
                tex_size = { roi.info().dim(slice_axes[0]), roi.info().dim(slice_axes[1]) };


                // set up 2D texture to store slice:
                GL::Texture tex;
                tex.gen (gl::TEXTURE_2D);
                gl::PixelStorei (gl::UNPACK_ALIGNMENT, 1);
                gl::TexImage2D (gl::TEXTURE_2D, 0, gl::R8, tex_size[0], tex_size[1], 0, gl::RED, gl::UNSIGNED_BYTE, nullptr);

                // set up off-screen framebuffer to map textures onto:
                GL::FrameBuffer framebuffer;
                framebuffer.gen();
                tex.set_interp_on (false);
                framebuffer.attach_color (tex, 0);
                framebuffer.draw_buffers (0);
                framebuffer.check();

                // render slice onto framebuffer:
                gl::Disable (gl::DEPTH_TEST);
                gl::Disable (gl::BLEND);
                gl::Viewport (0, 0, tex_size[0], tex_size[1]);
                roi.texture().bind();
                copy_program.start();
                gl::Uniform3iv (gl::GetUniformLocation (copy_program, "position"), 1, from.data());
                gl::Uniform2iv (gl::GetUniformLocation (copy_program, "axes"), 1, slice_axes.data());
                gl::DrawArrays (gl::TRIANGLE_FAN, 0, 4);
                copy_program.stop();
                framebuffer.unbind();

                // retrieve texture contents to main memory:
                before.resize (tex_size[0]*tex_size[1]);
                gl::GetTexImage (gl::TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, (void*)(&before[0]));
                after = before;
              }

              void draw_line (Item& roi, const Point<>& prev_pos, const Point<>& pos, bool insert_mode_value) {
                const GLubyte value = insert_mode_value ? 1 : 0;
                Point<> p = roi.transform().scanner2voxel (prev_pos);
                const Point<> final_pos = roi.transform().scanner2voxel (pos);
                const Point<> dir ((final_pos - p).normalise());
                Point<int> v (int(std::round (p[0])), int(std::round (p[1])), int(std::round (p[2])));
                const Point<int> final_vox (int(std::round (final_pos[0])), int(std::round (final_pos[1])), int(std::round (final_pos[2])));
                do {
                  if (v[0] >= 0 && v[0] < roi.info().dim(0) && v[1] >= 0 && v[1] < roi.info().dim(1) && v[2] >= 0 && v[2] < roi.info().dim(2))
                    after[v[0]-from[0] + size[0] * (v[1]-from[1] + size[1] * (v[2]-from[2]))] = value;
                  if (v != final_vox) {
                    Point<int> step (0, 0, 0);
                    float min_multiplier = std::numeric_limits<float>::infinity();
                    for (size_t axis = 0; axis != 3; ++axis) {
                      float this_multiplier;
                      if (dir[axis] > 0.0f)
                        this_multiplier = ((v[axis]+0.5f) - p[axis]) / dir[axis];
                      else
                        this_multiplier = ((v[axis]-0.5f) - p[axis]) / dir[axis];
                      if (std::isfinite (this_multiplier) && this_multiplier < min_multiplier) {
                        min_multiplier = this_multiplier;
                        step.set (0, 0, 0);
                        step[axis] = (dir[axis] > 0.0f) ? 1 : -1;
                      }
                    }
                    v += step;
                    p += dir * min_multiplier;
                  }
                } while (v != final_vox);
                roi.texture().bind();
                gl::TexSubImage3D (GL_TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], GL_RED, GL_UNSIGNED_BYTE, (void*) (&after[0]));
              }

              void draw_circle (Item& roi, const Point<>& pos, bool insert_mode_value, float diameter) {
                Point<> vox = roi.transform().scanner2voxel (pos);
                roi.brush_size = diameter;
                const float radius = 0.5f * diameter;
                const float radius_sq = Math::pow2 (radius);
                const GLubyte value = insert_mode_value ? 1 : 0;

                std::array<int,3> a = { int(std::lround (vox[0])), int(std::lround (vox[1])), int(std::lround (vox[2])) };
                std::array<int,3> b = { a[0]+1, a[1]+1, a[2]+1 };

                int rad[2] = { int(std::ceil (radius/roi.info().vox(slice_axes[0]))), int(std::ceil (radius/roi.info().vox(slice_axes[1]))) };
                a[slice_axes[0]] = std::max (0, a[slice_axes[0]]-rad[0]);
                a[slice_axes[1]] = std::max (0, a[slice_axes[1]]-rad[1]);
                b[slice_axes[0]] = std::min (roi.info().dim(slice_axes[0]), b[slice_axes[0]]+rad[0]);
                b[slice_axes[1]] = std::min (roi.info().dim(slice_axes[1]), b[slice_axes[1]]+rad[1]);

                for (int k = a[2]; k < b[2]; ++k)
                  for (int j = a[1]; j < b[1]; ++j)
                    for (int i = a[0]; i < b[0]; ++i)
                      if (Math::pow2 (roi.info().vox(0) * (vox[0]-i)) + 
                          Math::pow2 (roi.info().vox(1) * (vox[1]-j)) +
                          Math::pow2 (roi.info().vox(2) * (vox[2]-k)) < radius_sq)
                        after[i-from[0] + size[0] * (j-from[1] + size[1] * (k-from[2]))] = value;

                roi.texture().bind();
                gl::TexSubImage3D (GL_TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], GL_RED, GL_UNSIGNED_BYTE, (void*) (&after[0]));
              }

              void draw_rectangle (Item& roi, const Point<>& from_pos, const Point<>& to_pos, bool insert_mode_value) {
                Point<> vox = roi.transform().scanner2voxel (from_pos);
                const GLubyte value = insert_mode_value ? 1 : 0;
                std::array<int,3> a = { int(std::lround (vox[0])), int(std::lround (vox[1])), int(std::lround (vox[2])) };
                vox = roi.transform().scanner2voxel (to_pos);
                std::array<int,3> b = { int(std::lround (vox[0])), int(std::lround (vox[1])), int(std::lround (vox[2])) };

                if (a[0] > b[0]) std::swap (a[0], b[0]);
                if (a[1] > b[1]) std::swap (a[1], b[1]);
                if (a[2] > b[2]) std::swap (a[2], b[2]);

                a[slice_axes[0]] = std::max (0, a[slice_axes[0]]);
                a[slice_axes[1]] = std::max (0, a[slice_axes[1]]);
                b[slice_axes[0]] = std::min (roi.info().dim(slice_axes[0])-1, b[slice_axes[0]]);
                b[slice_axes[1]] = std::min (roi.info().dim(slice_axes[1])-1, b[slice_axes[1]]);

                after = before;
                for (int k = a[2]; k <= b[2]; ++k)
                  for (int j = a[1]; j <= b[1]; ++j)
                    for (int i = a[0]; i <= b[0]; ++i)
                      after[i-from[0] + size[0] * (j-from[1] + size[1] * (k-from[2]))] = value;

                roi.texture().bind();
                gl::TexSubImage3D (GL_TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], GL_RED, GL_UNSIGNED_BYTE, (void*) (&after[0]));
              }

              void undo (Item& roi) {
                roi.texture().bind();
                gl::TexSubImage3D (GL_TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], GL_RED, GL_UNSIGNED_BYTE, (void*) (&before[0]));
              }

              void redo (Item& roi) {
                roi.texture().bind();
                gl::TexSubImage3D (GL_TEXTURE_3D, 0, from[0], from[1], from[2], size[0], size[1], size[2], GL_RED, GL_UNSIGNED_BYTE, (void*) (&after[0]));
              }

              std::array<GLint,3> from, size;
              std::array<GLint,2> tex_size, slice_axes;
              std::vector<GLubyte> before, after;

              static GL::Shader::Program copy_program;
              static GL::VertexBuffer copy_vertex_buffer;
              static GL::VertexArrayObject copy_vertex_array_object;
            };
            std::vector<UndoEntry> undo_list;

            int current_undo;
            bool saved;
            float min_brush_size, max_brush_size, brush_size;

            bool has_undo () { return current_undo >= 0; }
            bool has_redo () { return current_undo < int(undo_list.size()-1); }

            UndoEntry& current () { return undo_list[current_undo]; }
            void start (UndoEntry&& entry) { 
              saved = false;
              if (current_undo < 0) 
                current_undo = -1;
              while (current_undo+1 > int(undo_list.size()))
                undo_list.erase (undo_list.end()-1);
              undo_list.push_back (std::move (entry));
              while (undo_list.size() > size_t (number_of_undos))
                undo_list.erase (undo_list.begin());
              current_undo = undo_list.size()-1;
            }

            void undo () {
              if (has_undo()) {
                undo_list[current_undo].undo (*this);
                --current_undo;
              }
            }

            void redo () {
              if (has_redo()) {
                ++current_undo;
                undo_list[current_undo].redo (*this);
              }
            }

            static int number_of_undos;
            static int current_preset_colour;
            static int new_roi_counter;
        };

        GL::Shader::Program ROI::Item::UndoEntry::copy_program;
        GL::VertexBuffer ROI::Item::UndoEntry::copy_vertex_buffer;
        GL::VertexArrayObject ROI::Item::UndoEntry::copy_vertex_array_object;



        int ROI::Item::number_of_undos = 0;
        int ROI::Item::current_preset_colour = 0;
        int ROI::Item::new_roi_counter = 0;



        class ROI::Model : public ListModelBase
        {
          public:
            Model (QObject* parent) : 
              ListModelBase (parent) { }

            void load (VecPtr<MR::Image::Header>& list);
            void create (MR::Image::Header& image);

            Item* get (QModelIndex& index) {
              return dynamic_cast<Item*>(items[index.row()]);
            }
        };


        void ROI::Model::load (VecPtr<MR::Image::Header>& list)
        {
          beginInsertRows (QModelIndex(), items.size(), items.size()+list.size());
          for (size_t i = 0; i < list.size(); ++i) {
            Item* roi = new Item (*list[i]);
            roi->load (*list[i]);
            items.push_back (roi);
          }
          endInsertRows();
        }

        void ROI::Model::create (MR::Image::Header& image)
        {
          beginInsertRows (QModelIndex(), items.size(), items.size()+1);
          Item* roi = new Item (image);
          roi->zero ();
          items.push_back (roi);
          endInsertRows();
        }


        ROI::ROI (Window& main_window, Dock* parent) :
          Base (main_window, parent),
          in_insert_mode (false) {
            Item::number_of_undos = MR::File::Config::get_int ("NumberOfUndos", 16);

            VBoxLayout* main_box = new VBoxLayout (this);
            HBoxLayout* layout = new HBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);

            QPushButton* button = new QPushButton (this);
            button->setToolTip (tr ("New ROI"));
            button->setIcon (QIcon (":/new.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (new_slot ()));
            layout->addWidget (button, 1);

            button = new QPushButton (this);
            button->setToolTip (tr ("Open ROI"));
            button->setIcon (QIcon (":/open.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (open_slot ()));
            layout->addWidget (button, 1);

            save_button = new QPushButton (this);
            save_button->setToolTip (tr ("Save ROI"));
            save_button->setIcon (QIcon (":/save.svg"));
            save_button->setEnabled (false);
            connect (save_button, SIGNAL (clicked()), this, SLOT (save_slot ()));
            layout->addWidget (save_button, 1);

            close_button = new QPushButton (this);
            close_button->setToolTip (tr ("Close ROI"));
            close_button->setIcon (QIcon (":/close.svg"));
            close_button->setEnabled (false);
            connect (close_button, SIGNAL (clicked()), this, SLOT (close_slot ()));
            layout->addWidget (close_button, 1);

            hide_all_button = new QPushButton (this);
            hide_all_button->setToolTip (tr ("Hide All"));
            hide_all_button->setIcon (QIcon (":/hide.svg"));
            hide_all_button->setCheckable (true);
            connect (hide_all_button, SIGNAL (clicked()), this, SLOT (hide_all_slot ()));
            layout->addWidget (hide_all_button, 1);

            main_box->addLayout (layout, 0);

            list_view = new QListView (this);
            list_view->setSelectionMode (QAbstractItemView::ExtendedSelection);
            list_view->setDragEnabled (true);
            list_view->viewport()->setAcceptDrops (true);
            list_view->setDropIndicatorShown (true);

            list_model = new Model (this);
            list_view->setModel (list_model);
            list_view->setSelectionMode (QAbstractItemView::SingleSelection);

            main_box->addWidget (list_view, 1);

            layout = new HBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);

            draw_button = new QToolButton (this);
            draw_button->setToolButtonStyle (Qt::ToolButtonTextBesideIcon);
            QAction* action = new QAction (QIcon (":/draw.svg"), tr ("Draw / erase"), this);
            action->setShortcut (tr ("D"));
            action->setToolTip (tr ("Add/remove voxels to/from ROI"));
            action->setCheckable (true);
            action->setEnabled (false);
            connect (action, SIGNAL (toggled(bool)), this, SLOT (draw_slot ()));
            draw_button->setDefaultAction (action);
            layout->addWidget (draw_button, 1);

            undo_button = new QToolButton (this);
            undo_button->setToolButtonStyle (Qt::ToolButtonTextBesideIcon);
            action = new QAction (QIcon (":/undo.svg"), tr ("Undo"), this);
            action->setShortcut (tr ("Ctrl+Z"));
            action->setToolTip (tr ("Undo last edit"));
            action->setCheckable (false);
            action->setEnabled (false);
            connect (action, SIGNAL (triggered()), this, SLOT (undo_slot ()));
            undo_button->setDefaultAction (action);
            layout->addWidget (undo_button, 1);

            redo_button = new QToolButton (this);
            redo_button->setToolButtonStyle (Qt::ToolButtonTextBesideIcon);
            action = new QAction (QIcon (":/redo.svg"), tr ("Redo"), this);
            action->setShortcut (tr ("Ctrl+Y"));
            action->setToolTip (tr ("Redo last edit"));
            action->setCheckable (false);
            action->setEnabled (false);
            connect (action, SIGNAL (triggered()), this, SLOT (redo_slot ()));
            redo_button->setDefaultAction (action);
            layout->addWidget (redo_button, 1);

            main_box->addLayout (layout, 0);

            layout = new HBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);

            edit_mode_group = new QActionGroup (this);
            edit_mode_group->setExclusive (true);
            edit_mode_group->setEnabled (false);
            connect (edit_mode_group, SIGNAL (triggered (QAction*)), this, SLOT (select_edit_mode (QAction*)));

            rectangle_button = new QToolButton (this);
            rectangle_button->setToolButtonStyle (Qt::ToolButtonTextBesideIcon);
            action = new QAction (QIcon (":/rectangle.svg"), tr ("Rectangle"), this);
            action->setShortcut (tr ("Ctrl+R"));
            action->setToolTip (tr ("Edit ROI using a rectangle"));
            action->setCheckable (true);
            action->setChecked (false);
            edit_mode_group->addAction (action);
            rectangle_button->setDefaultAction (action);
            layout->addWidget (rectangle_button, 1);

            brush_button = new QToolButton (this);
            brush_button->setToolButtonStyle (Qt::ToolButtonTextBesideIcon);
            action = new QAction (QIcon (":/brush.svg"), tr ("Brush"), this);
            action->setShortcut (tr ("Ctrl+B"));
            action->setToolTip (tr ("Edit ROI using a brush"));
            action->setCheckable (true);
            action->setChecked (true);
            edit_mode_group->addAction (action);
            brush_button->setDefaultAction (action);
            layout->addWidget (brush_button, 1);

            brush_size_button = new AdjustButton (this);
            brush_size_button->setToolTip (tr ("brush size"));
            brush_size_button->setEnabled (true);
            layout->addWidget (brush_size_button, 1);

            main_box->addLayout (layout, 0);

            layout = new HBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);


            colour_button = new QColorButton;
            colour_button->setEnabled (false);
            connect (colour_button, SIGNAL (clicked()), this, SLOT (colour_changed()));
            layout->addWidget (colour_button, 0);

            opacity_slider = new QSlider (Qt::Horizontal);
            opacity_slider->setToolTip (tr("ROI opacity"));
            opacity_slider->setRange (1,1000);
            opacity_slider->setSliderPosition (int (1000));
            connect (opacity_slider, SIGNAL (valueChanged (int)), this, SLOT (opacity_changed(int)));
            opacity_slider->setEnabled (false);
            layout->addWidget (opacity_slider, 1);

            main_box->addLayout (layout, 0);

            lock_to_axes_button = new QPushButton (tr("Lock to ROI axes"), this);
            lock_to_axes_button->setToolTip (tr (
                  "ROI editing inherently operates on a plane of the ROI image.\n"
                  "This can lead to confusing behaviour when the viewing plane\n"
                  "is not aligned with the ROI axes. When this button is set,\n"
                  "the viewing plane will automatically switch to that closest\n"
                  "to the ROI axes for every drawing operation."));
            lock_to_axes_button->setIcon (QIcon (":/lock.svg"));
            lock_to_axes_button->setCheckable (true);
            lock_to_axes_button->setChecked (true);
            lock_to_axes_button->setEnabled (false);
            connect (lock_to_axes_button, SIGNAL (clicked()), this, SLOT (update_slot ()));
            main_box->addWidget (lock_to_axes_button, 1);


            connect (list_view->selectionModel(),
                SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
                SLOT (update_selection()));

            connect (&window, SIGNAL (imageChanged()), this, SLOT (update_selection()));

            connect (list_model, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
                this, SLOT (toggle_shown_slot (const QModelIndex&, const QModelIndex&)));

            update_selection();
          }


        ROI::~ROI()
        {
          for (int i = 0; i != list_model->rowCount(); ++i) {
            QModelIndex index = list_model->index (i, 0);
            Item* roi = list_model->get (index);
            if (!roi->saved) {
              if (QMessageBox::question (&window, tr("ROI not saved"), tr (("Image " + roi->get_filename() + " has been modified. Do you want to save it?").c_str())) == QMessageBox::Yes)
                save (roi);
            }
          }
        }


        void ROI::new_slot ()
        {
          assert (window.image());
          list_model->create (window.image()->header());
          list_view->selectionModel()->clear();
          list_view->selectionModel()->select (list_model->index (list_model->rowCount()-1, 0, QModelIndex()), QItemSelectionModel::Select);
          updateGL ();
        }



        void ROI::open_slot ()
        {
          std::vector<std::string> names = Dialog::File::get_images (this, "Select ROI images to open");
          if (names.empty())
            return;
          VecPtr<MR::Image::Header> list;
          for (size_t n = 0; n < names.size(); ++n)
            list.push_back (new MR::Image::Header (names[n]));

          load (list);
        }



        void ROI::save (Item* roi)
        {
          roi->texture().bind();

          std::vector<GLubyte> data (roi->info().dim(0) * roi->info().dim(1) * roi->info().dim(2));
          gl::GetTexImage (gl::TEXTURE_3D, 0, GL_RED, GL_UNSIGNED_BYTE, (void*) (&data[0]));

          try {
            MR::Image::Header header;
            header.info() = roi->info();
            std::string name = GUI::Dialog::File::get_save_image_name (&window, "Select name of ROI to save", roi->get_filename());
            if (name.size()) {
              MR::Image::Buffer<bool> buffer (name, header);
              roi->save (buffer.voxel(), data.data());
            }
          }
          catch (Exception& E) {
            E.display();
          }
        }



        void ROI::save_slot ()
        {
          QModelIndexList indices = list_view->selectionModel()->selectedIndexes();
          assert (indices.size() == 1);
          Item* roi = dynamic_cast<Item*> (list_model->get (indices[0]));
          save (roi);
        }




        void ROI::load (VecPtr<MR::Image::Header>& list) 
        {
          list_model->load (list);
          list_view->selectionModel()->select (list_model->index (list_model->rowCount()-1, 0, QModelIndex()), QItemSelectionModel::Select);
          updateGL ();
        }



        void ROI::close_slot ()
        {
          QModelIndexList indices = list_view->selectionModel()->selectedIndexes();
          assert (indices.size() == 1);
          Item* roi = dynamic_cast<Item*> (list_model->get (indices[0]));
          if (!roi->saved) {
            if (QMessageBox::question (&window, tr("ROI not saved"), tr ("ROI has been modified. Do you want to save it?")) == QMessageBox::Yes)
              save_slot();
          }

          list_model->remove_item (indices.first());
          updateGL();
        }


        void ROI::draw_slot ()
        {
          if (draw_button->isChecked())
            grab_focus ();
          else
            release_focus ();
        }



        void ROI::undo_slot () 
        {
          QModelIndexList indices = list_view->selectionModel()->selectedIndexes();
          if (indices.size() != 1) {
            WARN ("FIXME: shouldn't be here!");
            return;
          }
          Item* roi = dynamic_cast<Item*> (list_model->get (indices[0]));

          roi->undo();
          update_undo_redo();
          updateGL();
        }





        void ROI::redo_slot () 
        {
          QModelIndexList indices = list_view->selectionModel()->selectedIndexes();
          if (indices.size() != 1) {
            WARN ("FIXME: shouldn't be here!");
            return;
          }
          Item* roi = dynamic_cast<Item*> (list_model->get (indices[0]));

          roi->redo();
          update_undo_redo();
          updateGL();
        }



        void ROI::select_edit_mode (QAction*) 
        {
          brush_size_button->setEnabled (brush_button->isChecked());
        }


        void ROI::hide_all_slot () 
        {
          updateGL();
        }


        void ROI::draw (const Projection& projection, bool is_3D, int, int)
        {
          if (is_3D) return;

          if (!is_3D) {
            // set up OpenGL environment:
            gl::Enable (gl::BLEND);
            gl::Disable (gl::DEPTH_TEST);
            gl::DepthMask (gl::FALSE_);
            gl::ColorMask (gl::TRUE_, gl::TRUE_, gl::TRUE_, gl::TRUE_);
            gl::BlendFunc (gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
            gl::BlendEquation (gl::FUNC_ADD);
          }

          for (int i = 0; i < list_model->rowCount(); ++i) {
            if (list_model->items[i]->show && !hide_all_button->isChecked()) {
              Item* roi = dynamic_cast<Item*>(list_model->items[i]);
              //if (is_3D) 
              //window.get_current_mode()->overlays_for_3D.push_back (image);
              //else
              roi->render (shader, projection, projection.depth_of (window.focus()));
            }
          }

          if (!is_3D) {
            // restore OpenGL environment:
            gl::Disable (gl::BLEND);
            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);
          }
        }





        void ROI::toggle_shown_slot (const QModelIndex& index, const QModelIndex& index2) {
          if (index.row() == index2.row()) {
            list_view->setCurrentIndex(index);
          } else {
            for (size_t i = 0; i < list_model->items.size(); ++i) {
              if (list_model->items[i]->show) {
                list_view->setCurrentIndex (list_model->index (i, 0));
                break;
              }
            }
          }
          updateGL();
        }


        void ROI::update_slot () {
          updateGL();
        }

        void ROI::colour_changed () 
        {
          QModelIndexList indices = list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            Item* roi = dynamic_cast<Item*> (list_model->get (indices[i]));
            QColor c = colour_button->color();
            roi->colour = { GLubyte (c.red()), GLubyte (c.green()), GLubyte (c.blue()) };
          }
          updateGL();
        }




        void ROI::opacity_changed (int)
        {
          QModelIndexList indices = list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            Item* roi = dynamic_cast<Item*> (list_model->get (indices[i]));
            roi->alpha = opacity_slider->value() / 1.0e3f;
          }
          window.updateGL();
        }


        void ROI::update_undo_redo () 
        {
          QModelIndexList indices = list_view->selectionModel()->selectedIndexes();

          if (indices.size()) {
            Item* roi = dynamic_cast<Item*> (list_model->get (indices[0]));
            undo_button->defaultAction()->setEnabled (roi->has_undo());
            redo_button->defaultAction()->setEnabled (roi->has_redo());
          }
          else {
            undo_button->defaultAction()->setEnabled (false);
            redo_button->defaultAction()->setEnabled (false);
          }
        }


        void ROI::update_selection () 
        {
          if (!window.image()) {
            setEnabled (false);
            return;
          }
          else 
            setEnabled (true);

          QModelIndexList indices = list_view->selectionModel()->selectedIndexes();
          bool enable = window.image() && indices.size();

          opacity_slider->setEnabled (enable);
          save_button->setEnabled (enable);
          close_button->setEnabled (enable);
          draw_button->defaultAction()->setEnabled (enable);
          colour_button->setEnabled (enable);
          edit_mode_group->setEnabled (enable);
          brush_size_button->setEnabled (enable && brush_button->isChecked());
          lock_to_axes_button->setEnabled (enable);

          update_undo_redo();

          if (!indices.size()) {
            release_focus();
            return;
          }

          Item* roi = dynamic_cast<Item*> (list_model->get (indices[0]));
          colour_button->setColor (QColor (roi->colour[0], roi->colour[1], roi->colour[2]));
          opacity_slider->setValue (1.0e3f * roi->alpha);

          brush_size_button->setMin (roi->min_brush_size);
          brush_size_button->setMax (roi->max_brush_size);
          brush_size_button->setRate (0.1f * roi->min_brush_size);
          brush_size_button->setValue (roi->brush_size);
        }









        bool ROI::mouse_press_event () 
        { 
          if (in_insert_mode || window.modifiers() != Qt::NoModifier)
            return false;

          if (window.mouse_buttons() != Qt::LeftButton && window.mouse_buttons() != Qt::RightButton)
            return false;

          in_insert_mode = true;
          insert_mode_value = (window.mouse_buttons() == Qt::LeftButton);
          update_cursor();

          QModelIndexList indices = list_view->selectionModel()->selectedIndexes();
          if (indices.size() != 1) {
            WARN ("FIXME: shouldn't be here!");
            return false;
          }

          const Projection* proj = window.get_current_mode()->get_current_projection();
          if (!proj) 
            return false;
          current_origin =  proj->screen_to_model (window.mouse_position(), window.focus());
          Point<> normal = proj->screen_normal();

          window.set_focus (current_origin);
          prev_pos = current_origin;

          // figure out the closest ROI axis, and lock to it:
          Item* roi = dynamic_cast<Item*> (list_model->get (indices[0]));
          float x_dot_n = std::abs (roi->transform().image2scanner_dir (Point<> (1.0, 0.0, 0.0)).dot (normal));
          float y_dot_n = std::abs (roi->transform().image2scanner_dir (Point<> (0.0, 1.0, 0.0)).dot (normal));
          float z_dot_n = std::abs (roi->transform().image2scanner_dir (Point<> (0.0, 0.0, 1.0)).dot (normal));

          if (x_dot_n > y_dot_n) 
            current_axis = x_dot_n > z_dot_n ? 0 : 2;
          else 
            current_axis = y_dot_n > z_dot_n ? 1 : 2;

          // figure out current slice in ROI:
          current_slice = std::lround (roi->transform().scanner2voxel (current_origin)[current_axis]);

          // floating-point version of slice location to keep it consistent on
          // mouse move:
          Point<> slice_axis (0.0, 0.0, 0.0);
          slice_axis[current_axis] = current_axis == 2 ? 1.0 : -1.0;
          slice_axis = roi->transform().image2scanner_dir (slice_axis);
          current_slice_loc = current_origin.dot (slice_axis);


          if (lock_to_axes_button->isChecked()) {
            Math::Versor<float> orient;
            orient.from_matrix (roi->info().transform());
            window.set_snap_to_image (false);
            window.set_orientation (orient);
          }

          roi->start (Item::UndoEntry (*roi, current_axis, current_slice));
         

          if (brush_button->isChecked()) {
            if (brush_size_button->value() == brush_size_button->getMin())
              roi->current().draw_line (*roi, prev_pos, current_origin, insert_mode_value);
            else
              roi->current().draw_circle (*roi, current_origin, insert_mode_value, brush_size_button->value());
          } else if (rectangle_button->isChecked()) {
            roi->current().draw_rectangle (*roi, current_origin, current_origin, insert_mode_value);
          }


          updateGL();

          return true; 
        }


        bool ROI::mouse_move_event () 
        { 
          if (!in_insert_mode)
            return false;

          QModelIndexList indices = list_view->selectionModel()->selectedIndexes();
          if (!indices.size()) {
            WARN ("FIXME: shouldn't be here!");
            return false;
          }
          Item* roi = dynamic_cast<Item*> (list_model->get (indices[0]));

          const Projection* proj = window.get_current_mode()->get_current_projection();
          if (!proj) 
            return false;
          Point<> pos =  proj->screen_to_model (window.mouse_position(), window.focus());
          Point<> slice_axis (0.0, 0.0, 0.0);
          slice_axis[current_axis] = current_axis == 2 ? 1.0 : -1.0;
          slice_axis = roi->transform().image2scanner_dir (slice_axis);
          float l = (current_slice_loc - pos.dot (slice_axis)) / proj->screen_normal().dot (slice_axis);
          window.set_focus (window.focus() + l * proj->screen_normal());
          const Point<> pos_adj = pos + l * proj->screen_normal();

          if (brush_button->isChecked()) {
            if (brush_size_button->value() == brush_size_button->getMin())
              roi->current().draw_line (*roi, prev_pos, pos_adj, insert_mode_value);
            else
              roi->current().draw_circle (*roi, pos_adj, insert_mode_value, brush_size_button->value());
          } else if (rectangle_button->isChecked()) {
            roi->current().draw_rectangle (*roi, current_origin, pos_adj, insert_mode_value);
          }

          updateGL();
          prev_pos = pos_adj;
          return true; 
        }

        bool ROI::mouse_release_event () 
        { 
          in_insert_mode = false;
          update_cursor();
          update_undo_redo();
          return true; 
        }

        QCursor* ROI::get_cursor ()
        {
          if (!draw_button->isChecked())
            return nullptr;
          if (in_insert_mode && !insert_mode_value)
            return &Cursor::erase;
          return &Cursor::draw;

        }





        bool ROI::process_batch_command (const std::string& cmd, const std::string& args)
        {
          (void)cmd;
          (void)args;
          /*

          // BATCH_COMMAND roi.load path # Loads the specified image on the roi tool.
          if (cmd == "roi.load") {
            VecPtr<MR::Image::Header> list;
            try { list.push_back (new MR::Image::Header (args)); }
            catch (Exception& e) { e.display(); }
            load (list);
            return true;
          }

          // BATCH_COMMAND roi.opacity value # Sets the roi opacity to floating value [0-1].
          else if (cmd == "roi.opacity") {
            try {
              float n = to<float> (args);
              opacity_slider->setSliderPosition(int(1.e3f*n));
            }
            catch (Exception& e) { e.display(); }
            return true;
          }

          */
          return false;
        }



      }
    }
  }
}




