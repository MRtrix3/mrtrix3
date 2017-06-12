/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "gui/mrview/tool/connectome/edge.h"

#include "math/rng.h"
#include "math/versor.h"

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







        Edge::Edge (const node_t one, const node_t two, const Eigen::Vector3f& c_one, const Eigen::Vector3f& c_two) :
            node_indices { one, two },
            node_centres { c_one, c_two },
            dir ((node_centres[1] - node_centres[0]).normalized()),
            rot_matrix (new GLfloat[9]),
            size (1.0f),
            colour (0.5f, 0.5f, 0.5f),
            alpha (1.0f),
            visible (one != two),
            line (new Line (*this))
        {
          static const Eigen::Vector3f z_axis (0.0f, 0.0f, 1.0f);
          if (is_diagonal()) {

            rot_matrix[0] = 0.0f; rot_matrix[1] = 0.0f; rot_matrix[2] = 0.0f;
            rot_matrix[3] = 0.0f; rot_matrix[4] = 0.0f; rot_matrix[5] = 0.0f;
            rot_matrix[6] = 0.0f; rot_matrix[7] = 0.0f; rot_matrix[8] = 0.0f;

          } else {

            // First, let's get an axis of rotation, s.t. the rotation angle is positive
            Eigen::Vector3f v = (z_axis.cross (dir)).normalized();
            // Now, a rotation angle
            const float angle = std::acos (z_axis.dot (dir));
            // Convert to rotation matrix representation
            const Eigen::Matrix<float, 3, 3> matrix (Math::Versorf (Eigen::AngleAxisf (angle, v)).matrix());
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

        Edge::~Edge()
        {
          if (rot_matrix) {
            delete[] rot_matrix;
            rot_matrix = nullptr;
          }
        }






        Edge::Line::Line (const Edge& parent)
        {
          vector<Eigen::Vector3f> data;
          data.push_back (parent.get_node_centre (0));
          data.push_back (parent.get_node_centre (1));

          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          vertex_buffer.gen();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, 2 * sizeof (Eigen::Vector3f), &data[0][0], gl::STATIC_DRAW);

          data.assign (2, parent.get_dir());

          tangent_buffer.gen();
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, 2 * sizeof (Eigen::Vector3f), &data[0][0], gl::STATIC_DRAW);

          vertex_array_object.gen();
          vertex_array_object.bind();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (1);
          gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }

        Edge::Line::~Line()
        {
          MRView::GrabContext context;
          vertex_buffer.clear();
          tangent_buffer.clear();
          vertex_array_object.clear();
        }

        void Edge::Line::render() const
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          if (!vertex_buffer || !tangent_buffer || !vertex_array_object)
            return;
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          vertex_array_object.bind();
          gl::DrawArrays (gl::LINES, 0, 2);
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }








        Edge::Exemplar::Exemplar (const Edge& parent, const MR::DWI::Tractography::Streamline<float>& data) :
            endpoints { parent.get_node_centre(0), parent.get_node_centre(1) }
        {
          Math::RNG::Normal<float> rng;
          for (size_t i = 0; i != data.size(); ++i) {
            vertices.push_back (data[i]);
            if (!i)
              tangents.push_back ((data[i+1] - data[i]).normalized());
            else if (i == data.size() - 1)
              tangents.push_back ((data[i] - data[i-1]).normalized());
            else
              tangents.push_back ((data[i+1] - data[i-1]).normalized());
            Eigen::Vector3f n;
            if (i)
              n = binormals.back().cross (tangents[i]).normalized();
            else
              n = Eigen::Vector3f (rng(), rng(), rng()).cross (tangents[i]).normalized();
            normals.push_back (n);
            binormals.push_back (tangents[i].cross (n).normalized());
          }
        }




        Edge::Streamline::Streamline (const Exemplar& data)
        {
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          assert (data.tangents.size() == data.vertices.size());

          count = data.vertices.size();

          vertex_buffer.gen();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          if (data.vertices.size())
            gl::BufferData (gl::ARRAY_BUFFER, data.vertices.size() * sizeof (Eigen::Vector3f), &data.vertices[0][0], gl::STATIC_DRAW);

          tangent_buffer.gen();
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          if (data.tangents.size())
            gl::BufferData (gl::ARRAY_BUFFER, data.tangents.size() * sizeof (Eigen::Vector3f), &data.tangents[0][0], gl::STATIC_DRAW);

          vertex_array_object.gen();
          vertex_array_object.bind();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (1);
          gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }

        Edge::Streamline::~Streamline()
        {
          MRView::GrabContext context;
          vertex_buffer.clear();
          tangent_buffer.clear();
          vertex_array_object.clear();
        }

        void Edge::Streamline::render() const
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          if (!vertex_buffer || !tangent_buffer || !vertex_array_object)
            return;
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          vertex_array_object.bind();
          gl::DrawArrays (gl::LINE_STRIP, 0, count);
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }










        Edge::Streamtube::Streamtube (const Exemplar& data) :
            count (data.vertices.size())
        {
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          assert (data.normals.size() == data.vertices.size());
          assert (data.binormals.size() == data.vertices.size());

          shared.check_num_points (count);

          vector<Eigen::Vector3f> vertices;
          const size_t N = shared.points_per_vertex();
          vertices.reserve (N * data.vertices.size());
          for (vector<Eigen::Vector3f>::const_iterator i = data.vertices.begin(); i != data.vertices.end(); ++i) {
            for (size_t j = 0; j != N; ++j)
              vertices.push_back (*i);
          }
          vertex_buffer.gen();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          if (vertices.size())
            gl::BufferData (gl::ARRAY_BUFFER, vertices.size() * sizeof (Eigen::Vector3f), &vertices[0][0], gl::STATIC_DRAW);

          vector<Eigen::Vector3f> tangents;
          tangents.reserve (vertices.size());
          for (vector<Eigen::Vector3f>::const_iterator i = data.tangents.begin(); i != data.tangents.end(); ++i) {
            for (size_t j = 0; j != N; ++j)
              tangents.push_back (*i);
          }
          tangent_buffer.gen();
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          if (tangents.size())
            gl::BufferData (gl::ARRAY_BUFFER, tangents.size() * sizeof (Eigen::Vector3f), &tangents[0][0], gl::STATIC_DRAW);

          vector< std::pair<float, float> > normal_multipliers;
          const float angle_multiplier = 2.0 * Math::pi / float(shared.points_per_vertex());
          for (size_t i = 0; i != shared.points_per_vertex(); ++i)
            normal_multipliers.push_back (std::make_pair (std::cos (i * angle_multiplier), std::sin (i * angle_multiplier)));
          vector<Eigen::Vector3f> normals;
          normals.reserve (vertices.size());
          for (size_t i = 0; i != data.vertices.size(); ++i) {
            for (vector< std::pair<float, float> >::const_iterator j = normal_multipliers.begin(); j != normal_multipliers.end(); ++j)
              normals.push_back ((j->first * data.normals[i]) + (j->second * data.binormals[i]));
          }
          normal_buffer.gen();
          normal_buffer.bind (gl::ARRAY_BUFFER);
          if (normals.size())
            gl::BufferData (gl::ARRAY_BUFFER, normals.size() * sizeof (Eigen::Vector3f), &normals[0][0], gl::STATIC_DRAW);

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
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }

        Edge::Streamtube::~Streamtube()
        {
          MRView::GrabContext context;
          vertex_buffer.clear();
          tangent_buffer.clear();
          normal_buffer.clear();
          vertex_array_object.clear();
        }

        void Edge::Streamtube::render() const
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          if (!vertex_buffer || !tangent_buffer || !normal_buffer || !vertex_array_object)
            return;
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          normal_buffer.bind (gl::ARRAY_BUFFER);
          vertex_array_object.bind();
          gl::MultiDrawElements (gl::TRIANGLE_STRIP, shared.element_counts, gl::UNSIGNED_INT, (const GLvoid* const*)shared.element_indices, count-1);
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
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





