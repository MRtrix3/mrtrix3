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





      }
    }
  }
}





