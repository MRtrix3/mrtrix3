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

#ifndef __gui_mrview_tool_connectome_edge_h__
#define __gui_mrview_tool_connectome_edge_h__

#include <memory>

#include "math/math.h"
#include "connectome/connectome.h"
#include "dwi/tractography/streamline.h"
#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

      class Connectome;

      // Stores all information relating to the drawing of individual edges, both fixed and variable
      // Try to store more than would otherwise be optimal in here, in order to simplify the drawing process
      class Edge
      { MEMALIGN(Edge)

          typedef MR::Connectome::node_t node_t;

        public:
          Edge (const node_t, const node_t, const Eigen::Vector3f&, const Eigen::Vector3f&);
          Edge (Edge&&);
          Edge () = delete;
          ~Edge();

          void render_line() const { assert (line); line->render(); }

          void load_exemplar (const MR::DWI::Tractography::Streamline<float>& data) { assert (!exemplar); exemplar.reset (new Exemplar (*this, data)); }
          void clear_exemplar() { if (streamtube) delete streamtube.release(); if (streamline) delete streamline.release(); if (exemplar) delete exemplar.release(); }

          void create_streamline() { assert (!streamline); assert (exemplar); streamline.reset (new Streamline (*exemplar)); }
          void render_streamline() const { assert (streamline); streamline->render(); }
          void clear_streamline() { if (streamline) delete streamline.release(); }

          void create_streamtube() { assert (!streamtube); assert (exemplar); streamtube.reset (new Streamtube (*exemplar)); }
          void render_streamtube() const { assert (streamtube); streamtube->render(); }
          void clear_streamtube() { if (streamtube) delete streamtube.release(); }

          static void set_streamtube_LOD (const size_t lod) { Streamtube::LOD (lod); }

          node_t get_node_index (const size_t i) const { assert (i==0 || i==1); return node_indices[i]; }
          const Eigen::Vector3f& get_node_centre (const size_t i) const { assert (i==0 || i==1); return node_centres[i]; }
          Eigen::Vector3f get_com() const { return (node_centres[0] + node_centres[1]) * 0.5; }

          const GLfloat* get_rot_matrix() const { return rot_matrix; }

          const Eigen::Vector3f& get_dir() const { return dir; }
          void set_size (const float i) { size = i; }
          float get_size() const { return size; }
          void set_colour (const Eigen::Array3f& i) { colour = i; }
          const Eigen::Array3f& get_colour() const { return colour; }
          void set_alpha (const float i) { alpha = i; }
          float get_alpha() const { return alpha; }
          void set_visible (const bool i) { visible = i; }
          bool is_visible() const { return visible; }
          bool is_diagonal() const { return (node_indices[0] == node_indices[1]); }

          bool to_draw() const { return (visible && (alpha > 0.0f) && (size > 0.0f)); }

        private:
          const node_t node_indices[2];
          const Eigen::Vector3f node_centres[2];
          const Eigen::Vector3f dir;

          GLfloat* rot_matrix;

          float size;
          Eigen::Array3f colour;
          float alpha;
          bool visible;

          class Line;
          class Exemplar;
          class Streamline;
          class Streamtube;

          class Line
          { MEMALIGN(Line)
            public:
              Line (const Edge& parent);
              Line (Line&& that) :
                  vertex_buffer (std::move (that.vertex_buffer)),
                  tangent_buffer (std::move (that.tangent_buffer)),
                  vertex_array_object (std::move (that.vertex_array_object)) { }
              Line () = delete;
              ~Line();
              void render() const;
            private:
              GL::VertexBuffer vertex_buffer, tangent_buffer;
              GL::VertexArrayObject vertex_array_object;
          };
          std::unique_ptr<Line> line;

          // Raw data for exemplar; need to hold on to this
          class Exemplar
          { MEMALIGN(Exemplar)
            public:
              Exemplar (const Edge&, const MR::DWI::Tractography::Streamline<float>&);
              Exemplar (Exemplar&& that) :
                  endpoints { that.endpoints[0], that.endpoints[1] },
                  vertices (std::move (that.vertices)),
                  tangents (std::move (that.tangents)),
                  normals (std::move (that.normals)),
                  binormals (std::move (that.binormals)) { }
              Exemplar () = delete;
            private:
              const Eigen::Vector3f endpoints[2];
              vector<Eigen::Vector3f> vertices, tangents, normals, binormals;
              friend class Streamline;
              friend class Streamtube;
          };
          std::unique_ptr<Exemplar> exemplar;

          // Class to store data relating to storing and displaying the exemplar as a streamline
          class Streamline
          { MEMALIGN(Streamline)
            public:
              Streamline (const Exemplar& exemplar);
              Streamline (Streamline&& that) :
                  count (that.count),
                  vertex_buffer (std::move (that.vertex_buffer)),
                  tangent_buffer (std::move (that.tangent_buffer)),
                  vertex_array_object (std::move (that.vertex_array_object)) { that.count = 0; }
              Streamline () = delete;
              ~Streamline();
              void render() const;
            private:
              // The master thread must assign the VBOs and VAO
              GLuint count;
              GL::VertexBuffer vertex_buffer, tangent_buffer;
              GL::VertexArrayObject vertex_array_object;
          };
          std::unique_ptr<Streamline> streamline;

          // Class to store data for plotting each edge exemplar as a streamtube
          class Streamtube
          { MEMALIGN(Streamtube)
            public:
              Streamtube (const Exemplar&);
              Streamtube (Streamtube&& that) :
                  count (that.count),
                  vertex_buffer (std::move (that.vertex_buffer)),
                  tangent_buffer (std::move (that.tangent_buffer)),
                  normal_buffer (std::move (that.normal_buffer)),
                  vertex_array_object (std::move (that.vertex_array_object)) { that.count = 0; }
              ~Streamtube();
              void render() const;
              static void LOD (const size_t lod) { shared.set_LOD (lod); }
            private:
              GLuint count;
              GL::VertexBuffer vertex_buffer, tangent_buffer, normal_buffer;
              GL::VertexArrayObject vertex_array_object;

              class Shared
              { MEMALIGN(Shared)
                public:
                  Shared() : max_num_points (0), LOD (0), element_counts (nullptr) { }
                  ~Shared() { clear(); }
                  void check_num_points (const size_t num_points) { if (num_points > max_num_points) { clear(); max_num_points = num_points; regenerate(); } }
                  void set_LOD (const size_t v) { if (LOD != v) { clear(); LOD = v; regenerate(); } }
                  size_t points_per_vertex() const { return Math::pow2 (LOD + 1); }
                protected:
                  size_t max_num_points;
                  size_t LOD;
                  GLsizei* element_counts;
                  GLuint** element_indices;
                private:
                  void regenerate();
                  void clear();
                  friend class Streamtube;
              };
              static Shared shared;

          };
          std::unique_ptr<Streamtube> streamtube;

      };



      }
    }
  }
}

#endif




