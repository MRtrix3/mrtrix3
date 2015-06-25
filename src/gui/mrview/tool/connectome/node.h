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

#ifndef __gui_mrview_tool_connectome_node_h__
#define __gui_mrview_tool_connectome_node_h__

#include "point.h"
#include "image/buffer_scratch.h"

#include "gui/opengl/gl.h"
#include "mesh/mesh.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


      // Stores all information relating to the drawing of individual nodes, both fixed and variable
      class Node
      {
        public:
          Node (const Point<float>&, const size_t, std::shared_ptr< MR::Image::BufferScratch<bool> >&);
          Node ();

          void assign_mesh (MR::Mesh::Mesh& in) { clear_mesh(); mesh.reset (new Node::Mesh (in)); }
          void render_mesh() const { if (!mesh) return; mesh->render(); }
          void clear_mesh() { if (mesh) delete mesh.release(); }

          const Point<float>& get_com() const { return centre_of_mass; }
          size_t get_volume() const { return volume; }

          void set_name (const std::string& i) { name = i; }
          const std::string& get_name() const { return name; }
          void set_size (const float i) { size = i; }
          float get_size() const { return size; }
          void set_colour (const Point<float>& i) { colour = i; }
          const Point<float>& get_colour() const { return colour; }
          void set_alpha (const float i) { alpha = i; }
          float get_alpha() const { return alpha; }
          void set_visible (const bool i) { visible = i; }
          bool is_visible() const { return visible; }

          bool to_draw() const { return (visible && (alpha > 0.0f) && (size > 0.0f)); }


        private:
          const Point<float> centre_of_mass;
          const size_t volume;
          std::shared_ptr< MR::Image::BufferScratch<bool> > mask;

          std::string name;
          float size;
          Point<float> colour;
          float alpha;
          bool visible;

          // Helper class to manage the storage and display of the mesh for each node
          class Mesh {
            public:
              Mesh (MR::Mesh::Mesh&);
              Mesh (const Mesh&) = delete;
              Mesh (Mesh&&);
              Mesh ();
              ~Mesh() { }
              Mesh& operator= (Mesh&&);
              void render() const;
            private:
              GLsizei count;
              GL::VertexBuffer vertex_buffer, normal_buffer;
              GL::VertexArrayObject vertex_array_object;
              GL::IndexBuffer index_buffer;
          };
          std::unique_ptr<Mesh> mesh;

      };



      }
    }
  }
}

#endif




