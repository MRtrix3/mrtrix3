/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __gui_mrview_tool_connectome_node_h__
#define __gui_mrview_tool_connectome_node_h__

#include "image.h"

#include "gui/opengl/gl.h"
#include "surface/mesh.h"

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
          Node (const Eigen::Vector3f&, const size_t, const size_t, const MR::Image<bool>&);
          Node ();

          void assign_mesh (MR::Surface::Mesh& in) { clear_mesh(); mesh.reset (new Node::Mesh (in)); }
          void render_mesh() const { if (!mesh) return; mesh->render(); }
          void clear_mesh() { if (mesh) delete mesh.release(); }

          const Eigen::Vector3f& get_com() const { return centre_of_mass; }
          size_t get_volume() const { return volume; }

          void set_name (const std::string& i) { name = i; }
          const std::string& get_name() const { return name; }
          void set_size (const float i) { size = i; }
          float get_size() const { return size; }
          void set_colour (const Eigen::Array3f& i) { colour = i; pixmap.fill (QColor (i[0] * 255.0f, i[1] * 255.0f, i[2] * 255.0f)); }
          const Eigen::Array3f& get_colour() const { return colour; }
          const QPixmap get_pixmap() const { return pixmap; }
          void set_alpha (const float i) { alpha = i; }
          float get_alpha() const { return alpha; }
          void set_visible (const bool i) { visible = i; }
          bool is_visible() const { return visible; }

          bool to_draw() const { return (visible && (alpha > 0.0f) && (size > 0.0f)); }

        private:
          const Eigen::Vector3f centre_of_mass;
          const size_t volume;
          MR::Image<bool> mask;

          std::string name;
          float size;
          Eigen::Array3f colour;
          float alpha;
          bool visible;

          QPixmap pixmap;

          // Helper class to manage the storage and display of the mesh for each node
          class Mesh {
            public:
              Mesh (MR::Surface::Mesh&);
              Mesh (const Mesh&) = delete;
              Mesh (Mesh&&);
              Mesh () = delete;
              ~Mesh();
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




