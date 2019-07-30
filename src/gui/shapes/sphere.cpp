/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "gui/shapes/sphere.h"

#include <map>

#define ONE .525731112119133606
#define TAU .850650808352039932

#define NUM_VERTICES 12
#define NUM_INDICES 20

namespace
{

  static float initial_vertices[NUM_VERTICES][3] = {
      {-ONE,  TAU,  0.0},
      { ONE,  TAU,  0.0},
      {-ONE, -TAU,  0.0},
      { ONE, -TAU,  0.0},

      { 0.0, -ONE,  TAU},
      { 0.0,  ONE,  TAU},
      { 0.0, -ONE, -TAU},
      { 0.0,  ONE, -TAU},

      { TAU,  0.0, -ONE},
      { TAU,  0.0,  ONE},
      {-TAU,  0.0, -ONE},
      {-TAU,  0.0,  ONE} };


  static GLuint initial_indices[NUM_INDICES][3] = {
      // 5 faces around point 0
      {0, 11, 5},
      {0, 5, 1},
      {0, 1, 7},
      {0, 7, 10},
      {0, 10, 11},

      // 5 adjacent faces
      {1, 5, 9},
      {5, 11, 4},
      {11, 10, 2},
      {10, 7, 6},
      {7, 1, 8},

      // 5 faces around point 3
      {3, 9, 4},
      {3, 4, 2},
      {3, 2, 6},
      {3, 6, 8},
      {3, 8, 9},

      // 5 adjacent faces
      {4, 9, 5},
      {2, 4, 11},
      {6, 2, 10},
      {8, 6, 7},
      {9, 8, 1} };

}



namespace MR
{
  namespace GUI
  {
    namespace Shapes
    {

    namespace {



      class Triangle
      { NOMEMALIGN
        public:
          Triangle () { }
          Triangle (const GLuint x[3]) {
            index[0] = x[0];
            index[1] = x[1];
            index[2] = x[2];
          }
          Triangle (size_t i1, size_t i2, size_t i3) {
            index[0] = i1;
            index[1] = i2;
            index[2] = i3;
          }
          void set (size_t i1, size_t i2, size_t i3) {
            index[0] = i1;
            index[1] = i2;
            index[2] = i3;
          }
          GLuint& operator[] (int n) {
            return index[n];
          }
        protected:
          GLuint  index[3];
      };

      class Edge
      { NOMEMALIGN
        public:
          Edge (const Edge& E) {
            set (E.i1, E.i2);
          }
          Edge (GLuint a, GLuint b) {
            set (a,b);
          }
          bool operator< (const Edge& E) const {
            return (i1 < E.i1 ? true : i2 < E.i2);
          }
          void set (GLuint a, GLuint b) {
            if (a < b) {
              i1 = a;
              i2 = b;
            }
            else {
              i1 = b;
              i2 = a;
            }
          }
          GLuint i1;
          GLuint i2;
      };


    }





    void Sphere::LOD (const size_t level_of_detail)
    {
      //vector<Vertex> vertices;
      vertices.clear();
      vector<Triangle> indices;

      for (size_t n = 0; n < NUM_VERTICES; n++)
        vertices.push_back (initial_vertices[n]);

      for (size_t n = 0; n < NUM_INDICES; n++)
        indices.push_back (initial_indices[n]);

      std::map<Edge,GLuint> edges;

      for (size_t lod = 0; lod < level_of_detail; lod++) {
        GLuint num = indices.size();
        for (GLuint n = 0; n < num; n++) {
          GLuint index1, index2, index3;

          Edge E (indices[n][0], indices[n][1]);
          std::map<Edge,GLuint>::const_iterator iter;
          if ( (iter = edges.find (E)) == edges.end()) {
            index1 = vertices.size();
            edges.insert (std::make_pair (E, index1));
            vertices.push_back (Vertex (vertices, indices[n][0], indices[n][1]));
          }
          else index1 = iter->second;

          E.set (indices[n][1], indices[n][2]);
          if ( (iter = edges.find (E)) == edges.end()) {
            index2 = vertices.size();
            edges.insert (std::make_pair (E, index2));
            vertices.push_back (Vertex (vertices, indices[n][1], indices[n][2]));
          }
          else index2 = iter->second;

          E.set (indices[n][2], indices[n][0]);
          if ( (iter = edges.find (E)) == edges.end()) {
            index3 = vertices.size();
            edges.insert (std::make_pair (E, index3));
            vertices.push_back (Vertex (vertices, indices[n][2], indices[n][0]));
          }
          else index3 = iter->second;

          indices.push_back (Triangle (indices[n][0], index1, index3));
          indices.push_back (Triangle (indices[n][1], index2, index1));
          indices.push_back (Triangle (indices[n][2], index3, index2));
          indices[n].set (index1, index2, index3);
        }
      }

      vertex_buffer.gen();
      vertex_buffer.bind (gl::ARRAY_BUFFER);
      gl::BufferData (gl::ARRAY_BUFFER, vertices.size()*sizeof(Vertex), &vertices[0][0], gl::STATIC_DRAW);

      num_indices = 3*indices.size();
      index_buffer.gen();
      index_buffer.bind();
      gl::BufferData (gl::ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(Triangle), &indices[0], gl::STATIC_DRAW);

    }



    }
  }
}




