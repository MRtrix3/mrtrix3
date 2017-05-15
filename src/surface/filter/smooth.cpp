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


#include "surface/filter/smooth.h"

#include <set>

#include "surface/utils.h"

namespace MR
{
  namespace Surface
  {
    namespace Filter
    {


      void Smooth::operator() (const Mesh& in, Mesh& out) const
      {
        std::unique_ptr<ProgressBar> progress;
        if (message.size())
          progress.reset (new ProgressBar (message, 8));
        out.clear();

        const size_t V = in.num_vertices();
        if (!V) return;

        if (in.num_quads())
          throw Exception ("For now, mesh smoothing is only supported for triangular meshes");
        const size_t T = in.num_triangles();
        if (V == 3*T)
          throw Exception ("Cannot perform smoothing on this mesh: no triangulation information");

        // Pre-compute polygon centroids and areas
        VertexList centroids;
        vector<default_type> areas;
        for (TriangleList::const_iterator p = in.triangles.begin(); p != in.triangles.end(); ++p) {
          centroids.push_back ((in.vertices[(*p)[0]] + in.vertices[(*p)[1]] + in.vertices[(*p)[2]]) * (1.0/3.0));
          areas.push_back (area (in, *p));
        }
        if (progress) ++(*progress);

        // Perform pre-calculation of an appropriate mesh neighbourhood for each vertex
        // Use knowledge of the connections between vertices provided by the triangles/quads to
        //   perform an iterative search outward from each vertex, selecting a subset of
        //   polygons for each vertex
        // Extent of window should be approximately the value of spatial_factor, though only an
        //   approximate windowing is likely to be used (i.e. number of iterations)
        //
        // Initialisation is different to iterations: Need a single pass to find those
        //   polygons that actually use the vertex
        vector< std::set<uint32_t> > vert_polys (V, std::set<uint32_t>());
        // For each vertex, don't just want to store the polygons within the neighbourhood;
        //   also want to store those that will be expanded from in the next iteration
        vector< vector<uint32_t> > vert_polys_to_expand (V, vector<uint32_t>());

        for (uint32_t t = 0; t != T; ++t) {
          for (uint32_t i = 0; i != 3; ++i) {
            vert_polys[(in.triangles[t])[i]].insert (t);
            vert_polys_to_expand[(in.triangles[t])[i]].push_back (t);
          }
        }
        if (progress) ++(*progress);

        // Now, we want to expand this selection outwards for each vertex
        // To do this, also want to produce a list for each polygon: containing those polygons
        //   that share a common edge (i.e. two vertices)
        vector< vector<uint32_t> > poly_neighbours (T, vector<uint32_t>());
        for (uint32_t i = 0; i != T; ++i) {
          for (uint32_t j = i+1; j != T; ++j) {
            if (in.triangles[i].shares_edge (in.triangles[j])) {
              poly_neighbours[i].push_back (j);
              poly_neighbours[j].push_back (i);

            }
          }
        }
        if (progress) ++(*progress);

        // TODO Will want to develop a better heuristic for this
        for (size_t iter = 0; iter != 8; ++iter) {
          for (uint32_t v = 0; v != V; ++v) {

            // Find polygons at the outer edge of this expanding front, and add them to the neighbourhood for this vertex
            vector<uint32_t> next_front;
            for (vector<uint32_t>::const_iterator front = vert_polys_to_expand[v].begin(); front != vert_polys_to_expand[v].end(); ++front) {
              for (vector<uint32_t>::const_iterator expansion = poly_neighbours[*front].begin(); expansion != poly_neighbours[*front].end(); ++expansion) {
                const std::set<uint32_t>::const_iterator existing = vert_polys[v].find (*expansion);
                if (existing == vert_polys[v].end()) {
                  vert_polys[v].insert (*expansion);
                  next_front.push_back (*expansion);
                }
              }
            }
            vert_polys_to_expand[v] = std::move (next_front);

          }
        }
        if (progress) ++(*progress);



        // Need to perform a first mollification pass, where the polygon normals are
        //   smoothed but the vertices are not perturbed
        // However, in order to calculate these new normals, we need to calculate new vertex positions!
        VertexList mollified_vertices;
        // Use half standard spatial factor for mollification
        // Denominator = 2(SF/2)^2
        const default_type spatial_mollification_power_multiplier = -2.0 / Math::pow2 (spatial);
        // No need to normalise the Gaussian; have to explicitly normalise afterwards
        for (uint32_t v = 0; v != V; ++v) {

          Vertex new_pos (0.0, 0.0, 0.0);
          default_type sum_weights = 0.0;

          // For now, just use every polygon as part of the estimate
          // Eventually, restrict this to some form of mesh neighbourhood
          //for (size_t i = 0; i != centroids.size(); ++i) {
          for (std::set<uint32_t>::const_iterator it = vert_polys[v].begin(); it != vert_polys[v].end(); ++it) {
            const uint32_t i = *it;
            default_type this_weight = areas[i];
            const default_type distance_sq = (centroids[i] - in.vertices[v]).squaredNorm();
            this_weight *= std::exp (distance_sq * spatial_mollification_power_multiplier);
            const Vertex prediction = centroids[i];
            new_pos += this_weight * prediction;
            sum_weights += this_weight;
          }

          new_pos *= (1.0 / sum_weights);
          mollified_vertices.push_back (new_pos);

        }
        if (progress) ++(*progress);

        // Have new vertices; compute polygon normals based on these vertices
        Mesh mollified_mesh;
        mollified_mesh.load (mollified_vertices, in.triangles);
        VertexList tangents;
        for (TriangleList::const_iterator p = mollified_mesh.triangles.begin(); p != mollified_mesh.triangles.end(); ++p)
          tangents.push_back (normal (mollified_mesh, *p));
        if (progress) ++(*progress);

        // Now perform the actual smoothing
        const default_type spatial_power_multiplier = -0.5 / Math::pow2 (spatial);
        const default_type influence_power_multiplier = -0.5 / Math::pow2 (influence);
        for (size_t v = 0; v != V; ++v) {

          Vertex new_pos (0.0, 0.0, 0.0);
          default_type sum_weights = 0.0;

          //for (size_t i = 0; i != centroids.size(); ++i) {
          for (std::set<uint32_t>::const_iterator it = vert_polys[v].begin(); it != vert_polys[v].end(); ++it) {
            const uint32_t i = *it;
            default_type this_weight = areas[i];
            const default_type distance_sq = (centroids[i] - in.vertices[v]).squaredNorm();
            this_weight *= std::exp (distance_sq * spatial_power_multiplier);
            const default_type prediction_distance = (centroids[i] - in.vertices[v]).dot (tangents[i]);
            const Vertex prediction = in.vertices[v] + (tangents[i] * prediction_distance);
            this_weight *= std::exp (Math::pow2 (prediction_distance) * influence_power_multiplier);
            new_pos += this_weight * prediction;
            sum_weights += this_weight;
          }

          new_pos *= (1.0 / sum_weights);
          out.vertices.push_back (new_pos);

        }
        if (progress) ++(*progress);

        out.triangles = in.triangles;

        // If the vertex normals were calculated previously, re-calculate them
        if (in.have_normals())
          out.calculate_normals();

      }



    }
  }
}

