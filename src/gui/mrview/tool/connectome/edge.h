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

#ifndef __gui_mrview_tool_connectome_edge_h__
#define __gui_mrview_tool_connectome_edge_h__

#include "point.h"

#include "dwi/tractography/connectomics/connectomics.h"
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
      {

          typedef MR::DWI::Tractography::Connectomics::node_t node_t;

        public:
          Edge (const node_t, const node_t, const Point<float>&, const Point<float>&);
          Edge (Edge&&);
          Edge ();
          ~Edge();

          void render_line() const;

          void calculate_exemplar (const std::string& path) { assert (!exemplar); exemplar.reset (new Exemplar (*this, path)); }
          void clear_exemplar() { if (streamtube) delete streamtube.release(); if (streamline) delete streamline.release(); if (exemplar) delete exemplar.release(); }

          void create_streamline() { assert (!streamline); assert (exemplar); streamline.reset (new Streamline (*exemplar)); }
          void render_streamline() const { assert (streamline); streamline->render(); }
          void clear_streamline() { if (streamline) delete streamline.release(); }

          void create_streamtube() { assert (!streamtube); assert (exemplar); streamtube.reset (new Streamtube (*exemplar)); }
          void render_streamtube() const { assert (streamtube); streamtube->render(); }
          void clear_streamtube() { if (streamtube) delete streamtube.release(); }

          static void set_streamtube_LOD (const size_t lod) { Streamtube::LOD (lod); }

          node_t get_node_index (const size_t i) const { assert (i==0 || i==1); return node_indices[i]; }
          const Point<float> get_node_centre (const size_t i) const { assert (i==0 || i==1); return node_centres[i]; }
          Point<float> get_com() const { return (node_centres[0] + node_centres[1]) * 0.5; }

          const GLfloat* get_rot_matrix() const { return rot_matrix; }

          const Point<float>& get_dir() const { return dir; }
          void set_size (const float i) { size = i; }
          float get_size() const { return size; }
          void set_colour (const Point<float>& i) { colour = i; }
          const Point<float>& get_colour() const { return colour; }
          void set_alpha (const float i) { alpha = i; }
          float get_alpha() const { return alpha; }
          void set_visible (const bool i) { visible = i; }
          bool is_visible() const { return visible; }
          bool is_diagonal() const { return (node_indices[0] == node_indices[1]); }

        private:
          const node_t node_indices[2];
          const Point<float> node_centres[2];
          const Point<float> dir;

          GLfloat* rot_matrix;

          float size;
          Point<float> colour;
          float alpha;
          bool visible;

          class Exemplar;
          class Streamline;
          class Streamtube;

          // Raw data for exemplar; need to hold on to this
          class Exemplar
          {
            public:
              Exemplar (const Edge& parent, const std::string& path);
              Exemplar (Exemplar&& that) :
                  endpoints { that.endpoints[0], that.endpoints[1] },
                  vertices (std::move (that.vertices)),
                  tangents (std::move (that.tangents)),
                  normals (std::move (that.normals)),
                  binormals (std::move (that.binormals)) { }
              Exemplar () = delete;
            private:
              const Point<float> endpoints[2];
              std::vector< Point<float> > vertices, tangents, normals, binormals;
              friend class Streamline;
              friend class Streamtube;
          };
          std::unique_ptr<Exemplar> exemplar;

          // Class to store data relating to storing and displaying the exemplar as a streamline
          class Streamline
          {
            public:
              Streamline (const Exemplar& exemplar);
              Streamline (Streamline&& that) :
                  count (that.count),
                  vertex_buffer (std::move (that.vertex_buffer)),
                  tangent_buffer (std::move (that.tangent_buffer)),
                  vertex_array_object (std::move (that.vertex_array_object)) { that.count = 0; }
              Streamline () = delete;
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
          {
            public:
              Streamtube (const Exemplar&);
              Streamtube (Streamtube&& that) :
                  count (that.count),
                  vertex_buffer (std::move (that.vertex_buffer)),
                  tangent_buffer (std::move (that.tangent_buffer)),
                  normal_buffer (std::move (that.normal_buffer)),
                  vertex_array_object (std::move (that.vertex_array_object)) { that.count = 0; }
              void render() const;
              static void LOD (const size_t lod) { shared.set_LOD (lod); }
            private:
              GLuint count;
              GL::VertexBuffer vertex_buffer, tangent_buffer, normal_buffer;
              GL::VertexArrayObject vertex_array_object;

              class Shared
              {
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




