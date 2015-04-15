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
#include "image/buffer_scratch.h"
#include "image/header.h"
#include "image/loop.h"
#include "image/nav.h"
#include "image/transform.h"

#include "mesh/mesh.h"
#include "mesh/vox2mesh.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {



        bool Connectome::Shader::need_update (const Connectome&) const { return true; }

        // For now, assume that all nodes are being drawn based on the mesh;
        //   branches can be added later
        void Connectome::Shader::update (const Connectome& /*parent*/)
        {
          vertex_shader_source =
              "layout (location = 0) in vec3 vertexPosition_modelspace;\n"
              "uniform mat4 MVP;\n";

          vertex_shader_source +=
              "void main() {\n"
              "  gl_Position = MVP * vec4 (vertexPosition_modelspace, 1);\n";

          vertex_shader_source += "}\n";

          fragment_shader_source =
              "out vec3 color;\n"
              "in vec3 fragmentColor;\n";

          fragment_shader_source +=
              "void main(){\n";

          fragment_shader_source +=
              //"  color = fragmentColor;\n";
              "  color = vec3(1,0,0);\n";

          fragment_shader_source += "}\n";
        }














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

            image_open_slot();
        }


        Connectome::~Connectome () {}


        void Connectome::draw (const Projection& projection, bool /*is_3D*/, int /*axis*/, int /*slice*/)
        {
          if (hide_all_button->isChecked()) return;

          node_shader.start (*this);
          projection.set (node_shader);

          gl::Disable (gl::BLEND);
          gl::Enable (gl::DEPTH_TEST);
          gl::DepthMask (gl::TRUE_);

          //for (size_t i = 1; i != num_nodes(); ++i)
          //  nodes[i].render_mesh();
          nodes[2].render_mesh();

          node_shader.stop();
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

          // If a new parcellation image is opened, all other data should be invalidated
          clear_all();

          // Read in the image file, do the necessary conversions e.g. to mesh, store the number of nodes, ...
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






        Connectome::Node::Node (const Point<float>& com, const size_t vol, MR::Image::BufferScratch<bool>& img) :
            centre_of_mass (com),
            volume (vol)
        {
          MR::Mesh::Mesh temp;
          auto voxel = img.voxel();
          {
            MR::LogLevelLatch latch (0);
            MR::Mesh::vox2mesh (voxel, temp);
            temp.transform_voxel_to_realspace (img);
          }
          mesh = Node::Mesh (temp);
        }

        Connectome::Node::Node () :
            centre_of_mass (),
            volume (0) { }






        Connectome::Node::Mesh::Mesh (const MR::Mesh::Mesh& in) :
            count (in.num_triangles()),
            index_buffer (0)
        {
          std::vector<float> vertices;
          vertices.reserve (3 * in.num_vertices());
          for (size_t v = 0; v != in.num_vertices(); ++v) {
            for (size_t axis = 0; axis != 3; ++axis)
              vertices.push_back (in.vert(v)[axis]);
          }
          vertex_buffer.gen();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, vertices.size() * sizeof (float), &vertices[0], gl::STATIC_DRAW);

          vertex_array_object.gen();
          vertex_array_object.bind();
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));

          std::vector<unsigned int> indices;
          indices.reserve (3 * in.num_triangles());
          for (size_t i = 0; i != in.num_triangles(); ++i) {
            for (size_t v = 0; v != 3; ++v)
              indices.push_back (in.tri(i)[v]);
          }
          gl::GenBuffers (1, &index_buffer);
          gl::BindBuffer (GL_ELEMENT_ARRAY_BUFFER, index_buffer);
          gl::BufferData (GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof (unsigned int), &indices[0], GL_STATIC_DRAW);
        }

        Connectome::Node::Mesh::Mesh (Mesh&& that) :
            count (that.count),
            vertex_buffer (std::move (that.vertex_buffer)),
            vertex_array_object (std::move (that.vertex_array_object)),
            index_buffer (that.index_buffer)
        {
          that.count = that.index_buffer = 0;
        }

        Connectome::Node::Mesh::Mesh () :
            count (0),
            index_buffer (0) { }

        Connectome::Node::Mesh::~Mesh ()
        {
          if (count) {
            gl::DeleteBuffers (1, &index_buffer);
          }
        }

        Connectome::Node::Mesh& Connectome::Node::Mesh::operator= (Connectome::Node::Mesh&& that)
        {
          count = that.count; that.count = 0;
          vertex_buffer = std::move (that.vertex_buffer);
          vertex_array_object = std::move (that.vertex_array_object);
          index_buffer = that.index_buffer; that.index_buffer = 0;
          return *this;
        }

        void Connectome::Node::Mesh::render() const
        {
          assert (count);
          vertex_array_object.bind();
          gl::BindBuffer (GL_ELEMENT_ARRAY_BUFFER, index_buffer);
          gl::DrawArrays (gl::TRIANGLES, 0, count);
        }







        void Connectome::clear_all()
        {
          image_button ->setText ("");
          lut_button   ->setText ("");
          config_button->setText ("");
          nodes.clear();
          lookup.clear();
          node_map.clear();
        }

        void Connectome::initialise (const std::string& path)
        {

          // TODO This could be made faster by constructing the meshes on-the-fly

          MR::Image::Header H (path);
          if (!H.datatype().is_integer())
            throw Exception ("Input parcellation image must have an integer datatype");
          MR::Image::Buffer<node_t> buffer (path);
          auto voxel = buffer.voxel();
          MR::Image::Transform transform (H);
          std::vector< Point<float> > node_coms;
          std::vector<size_t> node_volumes;
          VecPtr< MR::Image::BufferScratch<bool> > node_masks (1);
          VecPtr< MR::Image::BufferScratch<bool>::voxel_type > node_mask_voxels (1);
          size_t node_count = 0;

          {
            MR::Image::LoopInOrder loop (voxel, "Importing parcellation image... ");
            for (loop.start (voxel); loop.ok(); loop.next (voxel)) {
              const node_t node_index = voxel.value();
              if (node_index) {

                if (node_index >= node_count) {
                  node_coms       .resize (node_index+1, Point<float> (0.0f, 0.0f, 0.0f));
                  node_volumes    .resize (node_index+1, 0);
                  node_masks      .resize (node_index+1);
                  node_mask_voxels.resize (node_index+1);
                  for (size_t i = node_count+1; i <= node_index; ++i) {
                    node_masks[i] = new MR::Image::BufferScratch<bool> (H, "Bitwise mask for node " + str(i));
                    node_mask_voxels[i] = new MR::Image::BufferScratch<bool>::voxel_type (*node_masks[i]);
                  }
                  node_count = node_index;
                }

                MR::Image::Nav::set_pos (*node_mask_voxels[node_index], voxel);
                node_mask_voxels[node_index]->value() = true;

                node_coms   [node_index] += transform.voxel2scanner (voxel);
                node_volumes[node_index]++;
              }
            }
          }
          for (node_t n = 1; n != node_count; ++n)
            node_coms[n] *= (1.0f / float(node_volumes[n]));

          // TODO In its current state, this could be multi-threaded
          {
            MR::ProgressBar progress ("Triangulating nodes...", node_count);
            nodes.push_back (Node());
            for (size_t i = 1; i != node_count; ++i) {
              nodes.push_back (Node (node_coms[i], node_volumes[i], *node_masks[i]));
              ++progress;
            }
          }

        }



      }
    }
  }
}





