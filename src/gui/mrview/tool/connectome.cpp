/*
   Copyright 2014 Brain Research Institute, Melbourne, Australia

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

#include "gui/mrview/tool/connectome.h"

#include "file/path.h"
#include "gui/dialog/file.h"
#include "image/buffer.h"
#include "image/header.h"
#include "image/loop.h"
#include "image/transform.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {




        Connectome::Connectome (Window& main_window, Dock* parent) :
          Base (main_window, parent) {

            VBoxLayout* main_box = new VBoxLayout (this);

            HBoxLayout* layout = new HBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);

            image_button = new QPushButton (this);
            image_button->setToolTip (tr ("Open parcellation image"));
            // TODO New icons
            // TODO Have the icons always there, but add the opened file name as text
            //image_button->setIcon (QIcon (":/open.svg"));
            connect (image_button, SIGNAL (clicked()), this, SLOT (image_open_slot ()));
            layout->addWidget (image_button, 1);

            lut_button = new QPushButton (this);
            lut_button->setToolTip (tr ("Open lookup table file"));
            //button->setIcon (QIcon (":/close.svg"));
            lut_button->setEnabled (false);
            connect (lut_button, SIGNAL (clicked()), this, SLOT (lut_open_slot ()));
            layout->addWidget (lut_button, 1);

            config_button = new QPushButton (this);
            config_button->setToolTip (tr ("Open connectome config file"));
            //config_button->setIcon (QIcon (":/close.svg"));
            config_button->setEnabled (false);
            connect (config_button, SIGNAL (clicked()), this, SLOT (config_open_slot ()));
            layout->addWidget (config_button, 1);

            hide_all_button = new QPushButton (this);
            hide_all_button->setToolTip (tr ("Hide all connectome visualisation"));
            hide_all_button->setIcon (QIcon (":/hide.svg"));
            hide_all_button->setCheckable (true);
            connect (hide_all_button, SIGNAL (clicked()), this, SLOT (hide_all_slot ()));
            layout->addWidget (hide_all_button, 1);

            main_box->addLayout (layout, 0);

            main_box->addStretch ();
            setMinimumSize (main_box->minimumSize());
        }


        Connectome::~Connectome () {}


        //void Connectome::draw (const Projection& transform, bool is_3D, int axis, int slice)
        void Connectome::draw (const Projection&, bool, int, int)
        {

        }


        //void Connectome::drawOverlays (const Projection& transform)
        void Connectome::drawOverlays (const Projection&)
        {
          if (hide_all_button->isChecked()) return;

        }


        //bool Connectome::process_batch_command (const std::string& cmd, const std::string& args)
        bool Connectome::process_batch_command (const std::string&, const std::string&)
        {
          return false;
        }


        void Connectome::image_open_slot()
        {
          const std::string path = Dialog::File::get_image (this, "Select connectome parcellation image");

          if (path.empty())
            return;

          // TODO If a new parcellation image is opened, all other data should be invalidated
          clear_all();

          // TODO Read in the image file, do the necessary conversions e.g. to mesh,
          //   store the number of nodes,
          initialise (path);

          image_button->setText (QString::fromStdString (Path::basename (path)));
        }


        void Connectome::lut_open_slot()
        {
          // TODO This may be difficult; don't necessarily know the format of the lookup table
          // Ideally also want to make use of the existing functions for importing these
        }
        void Connectome::config_open_slot()
        {

        }


        void Connectome::hide_all_slot()
        {
          window.updateGL();
        }







        Connectome::Node_mesh::Node_mesh (const Mesh::Mesh& in) :
            count (in.num_triangles()),
            vertex_buffer (0),
            vertex_array_object (0),
            index_buffer (0)
        {
          std::vector<float> vertices;
          vertices.reserve (3 * in.num_vertices());
          for (size_t v = 0; v != in.num_vertices(); ++v) {
            for (size_t axis = 0; axis != 3; ++axis)
              vertices.push_back (in.vert(v)[axis]);
          }
          gl::GenBuffers (1, &vertex_buffer);
          gl::BindBuffer (GL_ARRAY_BUFFER, vertex_buffer);
          gl::BufferData (GL_ARRAY_BUFFER, vertices.size() * sizeof (float), &vertices[0], GL_STATIC_DRAW);

          gl::GenVertexArrays (1, &vertex_array_object);
          gl::BindVertexArray (vertex_array_object);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));

          std::vector<unsigned int> indices;
          indices.reserve (3 * in.num_triangles());
          for (size_t i = 0; i != in.num_vertices(); ++i) {
            for (size_t v = 0; v != 3; ++v)
              indices.push_back (in.tri(i)[v]);
          }
          gl::GenBuffers (1, &index_buffer);
          gl::BindBuffer (GL_ELEMENT_ARRAY_BUFFER, index_buffer);
          gl::BufferData (GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof (unsigned int), &indices[0], GL_STATIC_DRAW);
        }


        Connectome::Node_mesh::~Node_mesh ()
        {
          gl::DeleteBuffers (1, &vertex_buffer);
          gl::DeleteVertexArrays (1, &vertex_array_object);
          gl::DeleteBuffers (1, &index_buffer);
        }


        void Connectome::Node_mesh::render()
        {
          gl::BindVertexArray (vertex_array_object);
          gl::BindBuffer (GL_ELEMENT_ARRAY_BUFFER, index_buffer);
          gl::DrawElements (gl::TRIANGLES, count, GL_UNSIGNED_INT, (void*)0);
        }







        void Connectome::clear_all()
        {
          image_button ->setText ("");
          lut_button   ->setText ("");
          config_button->setText ("");
          num_nodes = 0;
          lookup.clear();
          node_map.clear();
        }

        void Connectome::initialise (const std::string& path)
        {
          MR::Image::Header H (path);
          if (!H.datatype().is_integer())
            throw Exception ("Input parcellation image must have an integer datatype");
          MR::Image::Buffer<node_t> buffer (path);
          auto voxel = buffer.voxel();
          MR::Image::Transform transform (H);
          std::vector<size_t> node_volumes;
          MR::Image::LoopInOrder loop (voxel, "Importing parcellation image... ");
          for (loop.start (voxel); loop.ok(); loop.next (voxel)) {
            const node_t node = voxel.value();
            if (node) {
              if (node >= num_nodes) {
                num_nodes = node;
                centres_of_mass.resize (num_nodes+1, Point<float> (0.0f, 0.0f, 0.0f));
                node_volumes.resize (num_nodes+1, 0);
              }
              centres_of_mass[node] += transform.voxel2scanner (voxel);
              node_volumes[node]++;
            }
          }
          for (node_t n = 1; n != num_nodes; ++n)
            centres_of_mass[n] *= (1.0f / float(node_volumes[n]));
        }



      }
    }
  }
}





