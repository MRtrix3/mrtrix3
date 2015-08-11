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

#include "gui/mrview/tool/connectome/edge.h"

#include "math/rng.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"
#include "gui/mrview/tool/connectome/connectome.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {







        Edge::Edge (const node_t one, const node_t two, const Point<float>& c_one, const Point<float>& c_two) :
            node_indices { one, two },
            node_centres { c_one, c_two },
            dir ((node_centres[1] - node_centres[0]).normalise()),
            rot_matrix (new GLfloat[9]),
            size (1.0f),
            colour (0.5f, 0.5f, 0.5f),
            alpha (1.0f),
            visible (one != two),
            line (*this)
        {
          static const Point<float> z_axis (0.0f, 0.0f, 1.0f);
          if (is_diagonal()) {

            rot_matrix[0] = 0.0f; rot_matrix[1] = 0.0f; rot_matrix[2] = 0.0f;
            rot_matrix[3] = 0.0f; rot_matrix[4] = 0.0f; rot_matrix[5] = 0.0f;
            rot_matrix[6] = 0.0f; rot_matrix[7] = 0.0f; rot_matrix[8] = 0.0f;

          } else {

            // First, let's get an axis of rotation, s.t. the rotation angle is positive
            const Point<float> rot_axis = (z_axis.cross (dir).normalise());
            // Now, a rotation angle
            const float rot_angle = std::acos (z_axis.dot (dir));
            // Convert to versor representation
            const Math::Versor<float> versor (rot_angle, rot_axis);
            // Convert to a matrix
            Math::Matrix<float> matrix;
            versor.to_matrix (matrix);
            // Put into the GLfloat array
            rot_matrix[0] = matrix(0,0); rot_matrix[1] = matrix(0,1); rot_matrix[2] = matrix(0,2);
            rot_matrix[3] = matrix(1,0); rot_matrix[4] = matrix(1,1); rot_matrix[5] = matrix(1,2);
            rot_matrix[6] = matrix(2,0); rot_matrix[7] = matrix(2,1); rot_matrix[8] = matrix(2,2);

          }
        }

        Edge::Edge (Edge&& that) :
            node_indices { that.node_indices[0], that.node_indices[1] },
            node_centres { that.node_centres[0], that.node_centres[1] },
            dir (that.dir),
            rot_matrix (that.rot_matrix),
            size (that.size),
            colour (that.colour),
            alpha (that.alpha),
            visible (that.visible),
            line (std::move (that.line)),
            exemplar (std::move (that.exemplar)),
            streamline (std::move (that.streamline)),
            streamtube (std::move (that.streamtube))
        {
          that.rot_matrix = nullptr;
        }

        Edge::Edge () :
            node_indices { 0, 0 },
            node_centres { Point<float>(), Point<float>() },
            dir (Point<float>()),
            rot_matrix (nullptr),
            size (0.0f),
            colour (0.0f, 0.0f, 0.0f),
            alpha (0.0f),
            visible (false),
            line (*this) { }

        Edge::~Edge()
        {
          if (rot_matrix) {
            delete[] rot_matrix;
            rot_matrix = nullptr;
          }
        }






        Edge::Line::Line (const Edge& parent)
        {
          Window::GrabContext context;

          std::vector< Point<float> > data;
          data.push_back (parent.get_node_centre (0));
          data.push_back (parent.get_node_centre (1));

          vertex_buffer.gen();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, 2 * sizeof (Point<float>), &data[0][0], gl::STATIC_DRAW);

          data.assign (2, parent.get_dir());

          tangent_buffer.gen();
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, 2 * sizeof (Point<float>), &data[0][0], gl::STATIC_DRAW);

          vertex_array_object.gen();
          vertex_array_object.bind();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (1);
          gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));
        }

        void Edge::Line::render() const
        {
          if (!vertex_buffer || !tangent_buffer || !vertex_array_object)
            return;
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          vertex_array_object.bind();
          gl::DrawArrays (gl::LINES, 0, 2);
        }








        Edge::Exemplar::Exemplar (const Edge& parent, const MR::DWI::Tractography::Streamline<float>& data) :
            endpoints { parent.get_node_centre(0), parent.get_node_centre(1) }
        {
          Math::RNG::Normal<float> rng;
          for (size_t i = 0; i != data.size(); ++i) {
            vertices.push_back (data[i]);
            if (!i)
              tangents.push_back ((data[i+1] - data[i]).normalise());
            else if (i == data.size() - 1)
              tangents.push_back ((data[i] - data[i-1]).normalise());
            else
              tangents.push_back ((data[i+1] - data[i-1]).normalise());
            Point<float> n;
            if (i)
              n = binormals.back().cross (tangents[i]).normalise();
            else
              n = Point<float> (rng(), rng(), rng()).cross (tangents[i]).normalise();
            normals.push_back (n);
            binormals.push_back (tangents[i].cross (n).normalise());
          }
        }




        Edge::Streamline::Streamline (const Exemplar& data)
        {
          Window::GrabContext context;
          assert (data.tangents.size() == data.vertices.size());

          count = data.vertices.size();

          vertex_buffer.gen();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          if (data.vertices.size())
            gl::BufferData (gl::ARRAY_BUFFER, data.vertices.size() * sizeof (Point<float>), &data.vertices[0][0], gl::STATIC_DRAW);

          tangent_buffer.gen();
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          if (data.tangents.size())
            gl::BufferData (gl::ARRAY_BUFFER, data.tangents.size() * sizeof (Point<float>), &data.tangents[0][0], gl::STATIC_DRAW);

          vertex_array_object.gen();
          vertex_array_object.bind();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (1);
          gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));
        }



        void Edge::Streamline::render() const
        {
          if (!vertex_buffer || !tangent_buffer || !vertex_array_object)
            return;
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          vertex_array_object.bind();
          gl::DrawArrays (gl::LINE_STRIP, 0, count);
        }










        Edge::Streamtube::Streamtube (const Exemplar& data) :
            count (data.vertices.size())
        {
          Window::GrabContext context;
          assert (data.normals.size() == data.vertices.size());
          assert (data.binormals.size() == data.vertices.size());

          shared.check_num_points (count);

          std::vector< Point<float> > vertices;
          const size_t N = shared.points_per_vertex();
          vertices.reserve (N * data.vertices.size());
          for (std::vector< Point<float> >::const_iterator i = data.vertices.begin(); i != data.vertices.end(); ++i) {
            for (size_t j = 0; j != N; ++j)
              vertices.push_back (*i);
          }
          vertex_buffer.gen();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          if (vertices.size())
            gl::BufferData (gl::ARRAY_BUFFER, vertices.size() * sizeof (Point<float>), &vertices[0][0], gl::STATIC_DRAW);

          std::vector< Point<float> > tangents;
          tangents.reserve (vertices.size());
          for (std::vector< Point<float> >::const_iterator i = data.tangents.begin(); i != data.tangents.end(); ++i) {
            for (size_t j = 0; j != N; ++j)
              tangents.push_back (*i);
          }
          tangent_buffer.gen();
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          if (tangents.size())
            gl::BufferData (gl::ARRAY_BUFFER, tangents.size() * sizeof (Point<float>), &tangents[0][0], gl::STATIC_DRAW);

          std::vector< std::pair<float, float> > normal_multipliers;
          const float angle_multiplier = 2.0 * Math::pi / float(shared.points_per_vertex());
          for (size_t i = 0; i != shared.points_per_vertex(); ++i)
            normal_multipliers.push_back (std::make_pair (std::cos (i * angle_multiplier), std::sin (i * angle_multiplier)));
          std::vector< Point<float> > normals;
          normals.reserve (vertices.size());
          for (size_t i = 0; i != data.vertices.size(); ++i) {
            for (std::vector< std::pair<float, float> >::const_iterator j = normal_multipliers.begin(); j != normal_multipliers.end(); ++j)
              normals.push_back ((j->first * data.normals[i]) + (j->second * data.binormals[i]));
          }
          normal_buffer.gen();
          normal_buffer.bind (gl::ARRAY_BUFFER);
          if (normals.size())
            gl::BufferData (gl::ARRAY_BUFFER, normals.size() * sizeof (Point<float>), &normals[0][0], gl::STATIC_DRAW);

          vertex_array_object.gen();
          vertex_array_object.bind();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (1);
          gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));
          normal_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (2);
          gl::VertexAttribPointer (2, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));
        }



        void Edge::Streamtube::render() const
        {
          if (!vertex_buffer || !tangent_buffer || !normal_buffer || !vertex_array_object)
            return;
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          normal_buffer.bind (gl::ARRAY_BUFFER);
          vertex_array_object.bind();
          gl::MultiDrawElements (gl::TRIANGLE_STRIP, shared.element_counts, gl::UNSIGNED_INT, (const GLvoid* const*)shared.element_indices, count-1);
        }





        Edge::Streamtube::Shared Edge::Streamtube::shared;

        void Edge::Streamtube::Shared::regenerate()
        {
          clear();
          if (!max_num_points)
            return;

          const size_t N = points_per_vertex();

          element_indices = new GLuint* [max_num_points];
          size_t prev_set_first = 0;
          for (size_t i = 0; i != max_num_points; ++i) {
            element_indices[i] = new GLuint[2*(N+1)];
            const size_t this_set_first = (i+1)*N;
            for (size_t j = 0; j != N; ++j) {
              element_indices[i][2*j]   = this_set_first + j;
              element_indices[i][2*j+1] = prev_set_first + j;
            }
            element_indices[i][2*N]   = this_set_first;
            element_indices[i][2*N+1] = prev_set_first;
            prev_set_first = this_set_first;
          }
          element_counts = new GLsizei[max_num_points];
          for (size_t i = 0; i != max_num_points; ++i)
            element_counts[i] = 2*(N+1);

        }



        void Edge::Streamtube::Shared::clear()
        {
          if (element_counts) {
            delete[] element_counts;
            element_counts = nullptr;
          }
          if (element_indices) {
            for (size_t i = 0; i != max_num_points; ++i) {
              delete[] element_indices[i];
              element_indices[i] = nullptr;
            }
            delete[] element_indices;
            element_indices = nullptr;
          }
        }





      }
    }
  }
}





