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
            visible (one != two)
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
            visible (that.visible)
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
            visible (false) { }

        Edge::~Edge()
        {
          if (rot_matrix) {
            delete[] rot_matrix;
            rot_matrix = nullptr;
          }
        }





        void Edge::render_line() const
        {
          glColor3f (colour[0], colour[1], colour[2]);
          glBegin (GL_LINES);
          glVertex3f (node_centres[0][0], node_centres[0][1], node_centres[0][2]);
          glVertex3f (node_centres[1][0], node_centres[1][1], node_centres[1][2]);
          glEnd();
        }






        Edge::Exemplar::Exemplar (const std::string& path, const Edge& parent) :
            count (0)
        {
          if (!path.size()) {
            vertices.reset (new std::vector< Point<float> >());
            tangents.reset (new std::vector< Point<float> >());
            return;
          }

          MR::DWI::Tractography::Properties properties;
          MR::DWI::Tractography::Reader<float> reader (path, properties);

          if (properties["count"] == "0") {
            vertices.reset (new std::vector< Point<float> >());
            tangents.reset (new std::vector< Point<float> >());
            return;
          }

          // Eventually, the exemplars will be re-sampled to match the step size
          //   of the input file
          // However this information will also come in handy in determining an appropriate
          //   number of points to use in generating the exemplar
          float step_size = NAN;
          auto output_step_size_it = properties.find ("output_step_size");
          if (output_step_size_it == properties.end()) {
            auto step_size_it = properties.find ("step_size");
            if (step_size_it == properties.end())
              step_size = 1.0f;
            else
              step_size = to<float>(step_size_it->second);
          } else {
            step_size = to<float>(output_step_size_it->second);
          }

          // The number of points to initially use in representing the exemplar streamline
          // Make sure that if the pathway is of the maximum possible length, we generate
          //   enough points to adequately represent it; for anything shorter, we're
          //   just over-sampling a bit
          float max_dist = 0.0f;
          auto max_dist_it = properties.find ("max_dist");
          if (max_dist_it == properties.end())
            max_dist = 4.0 * dist (parent.node_centres[0], parent.node_centres[1]);
          else
            max_dist = to<float>(max_dist_it->second);
          const uint32_t num_points = std::round (max_dist / step_size) + 1;

          // Not too concerned about using Hermite interpolation here;
          //   differences between that and linear are likely to average out over many streamlines,
          //   and curvature undershoot doesn't matter too much in this context
          MR::DWI::Tractography::Streamline<float> streamline;
          std::vector< Point<float> > mean (num_points, Point<float> (0.0f, 0.0f, 0.0f));
          uint32_t count = 0;
          while (reader (streamline)) {
            ++count;

            // Determine whether or not this streamline is reversed w.r.t. the exemplar
            // The exemplar will be generated running from node [0] to node [1]
            bool is_reversed = false;
            float end_distances = dist2 (streamline.front(), parent.node_centres[0]) + dist2 (streamline.back(), parent.node_centres[1]);
            if (dist2 (streamline.front(), parent.node_centres[1]) + dist2 (streamline.back(), parent.node_centres[0]) < end_distances)
              is_reversed = true;

            // Contribute this streamline toward the mean exemplar streamline
            for (uint32_t i = 0; i != num_points; ++i) {
              float interp_pos = (streamline.size() - 1) * i / float(num_points);
              if (is_reversed)
                interp_pos = streamline.size() - 1 - interp_pos;
              const uint32_t lower = std::floor (interp_pos), upper (lower + 1);
              const float mu = interp_pos - lower;
              Point<float> pos;
              if (lower == streamline.size() - 1)
                pos = streamline.back();
              else
                pos = ((1.0f-mu) * streamline[lower]) + (mu * streamline[upper]);
              mean[i] += pos;
            }
          }

          const float scaling_factor = 1.0f / float(count);
          for (auto p = mean.begin(); p != mean.end(); ++p)
            *p *= scaling_factor;

          // Want to guarantee that the exemplar streamlines pass through the centre of mass
          //   of each of the connected nodes
          // Therefore, for the first and last e.g. 25% of the streamline, gradually add a
          //   weighted fraction of the node centre-of-mass to the exemplar position

#define STREAMTUBE_CONVERGE_COM_FRACTION 0.25

          uint32_t num_converging_points = STREAMTUBE_CONVERGE_COM_FRACTION * num_points;
          for (uint32_t i = 0; i != num_converging_points; ++i) {
            const float mu = i / float(num_converging_points);
            mean[i] = (mu * mean[i]) + ((1.0f-mu) * parent.node_centres[0]);
          }
          for (uint32_t i = num_points - 1; i != num_points - 1 - num_converging_points; --i) {
            const float mu = (num_points - 1 - i) / float(num_converging_points);
            mean[i] = (mu * mean[i]) + ((1.0f-mu) * parent.node_centres[1]);
          }

          // Resample the mean streamline to a constant step size
          // Ideally, start from the midpoint, resample backwards to the start of the exemplar,
          //   reverse the data, then do the second half of the exemplar
          int32_t index = (num_points + 1) / 2;
          vertices.reset (new std::vector< Point<float> > (1, mean[index]));
          tangents.reset (new std::vector< Point<float> > (1, (mean[index-1] - mean[index+1]).normalise()));
          const float step_sq = Math::pow2 (step_size);
          for (int32_t step = -1; step <= 1; step += 2) {
            if (step == 1) {
              std::reverse (vertices->begin(), vertices->end());
              index = (num_points + 1) / 2;
            }
            do {
              while ((index+step) >= 0 && (index+step) < int32_t(num_points) && dist2 (mean[index+step], vertices->back()) < step_sq)
                index += step;
              // Ideal point for fixed step size lies somewhere between mean[index] and mean[index+step]
              // Do a binary search to find this point
              // Unless we're at an endpoint...
              if (index == 0 || index == int32_t(num_points)-1) {
                vertices->push_back (mean[index]);
                tangents->push_back ((mean[index] - mean[index-step]).normalise());
              } else {
                float lower = 0.0f, mu = 0.5f, upper = 1.0f;
                Point<float> p ((mean[index] + mean[index+step]) * 0.5f);
                for (uint32_t iter = 0; iter != 6; ++iter) {
                  if (dist2 (p, vertices->back()) > step_sq)
                    upper = mu;
                  else
                    lower = mu;
                  mu = 0.5 * (lower + upper);
                  p = (mean[index] * (1.0-mu)) + (mean[index+step] * mu);
                }
                vertices->push_back (p);
                tangents->push_back ((mean[index+step] - mean[index]).normalise());
              }
            } while (index != 0 && index != int32_t(num_points)-1);
          }
        }



        void Edge::Exemplar::assign_buffers()
        {
          assert (vertices);
          assert (tangents);

          count = vertices->size();

          vertex_buffer.gen();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          if (vertices->size())
            gl::BufferData (gl::ARRAY_BUFFER, vertices->size() * sizeof (Point<float>), &(*vertices)[0][0], gl::STATIC_DRAW);

          tangent_buffer.gen();
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          if (tangents->size())
            gl::BufferData (gl::ARRAY_BUFFER, tangents->size() * sizeof (Point<float>), &(*tangents)[0][0], gl::STATIC_DRAW);

          vertex_array_object.gen();
          vertex_array_object.bind();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (1);
          gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));

          delete vertices.release();
          delete tangents.release();
        }



        void Edge::Exemplar::render()
        {
          if (!vertex_buffer || !tangent_buffer || !vertex_array_object)
            return;
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          tangent_buffer.bind (gl::ARRAY_BUFFER);
          vertex_array_object.bind();
          gl::DrawArrays (gl::LINE_STRIP, 0, count);
        }





      }
    }
  }
}





