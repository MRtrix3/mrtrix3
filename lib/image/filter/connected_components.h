/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt 08/03/12.

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

#ifndef __image_filter_connected_h__
#define __image_filter_connected_h__

#include "image/info.h"
#include "image/loop.h"
#include "image/voxel.h"
#include "image/buffer_scratch.h"
#include "math/matrix.h"


#include <iostream>

namespace MR
{
  namespace Image
  {
    namespace Filter
    {

      struct cluster {
          unsigned int label;
          unsigned int size;
      };

      bool compare_clusters (cluster i, cluster j) { return (i.size > j.size); }

      template <class MaskVoxelType>
      void compute_adjacency (MaskVoxelType & mask,
                              std::vector<Math::Matrix<float> > & adjacency_matrices,
                              std::vector<bool> & ignore_dim,
                              std::vector<std::vector<int> > & mask_indices,
                              std::vector<std::vector<uint32_t> > & adjacent_indices) {

        Image::BufferScratch<uint32_t> neigh_mask (mask);
        Image::BufferScratch<uint32_t>::voxel_type neigh_mask_vox (neigh_mask);

        // 1st pass, store mask values and their memory locations
        Image::LoopInOrder loop(mask);
        for (loop.start(mask, neigh_mask_vox); loop.ok(); loop.next(mask, neigh_mask_vox)) {
          if (mask.value() > 0.5) {
            neigh_mask_vox.value() = mask_indices.size(); // keep track of the index location for the second pass
            std::vector<int> index(4);
            for (size_t dim = 0; dim < mask.ndim(); dim++)
              index[dim] = mask[dim];
            mask_indices.push_back (index);
          } else {
            neigh_mask_vox.value() = 0;
          }
        }


        // 2nd pass, define adjacencies
        MaskVoxelType mask_neigh (mask);
        std::vector<std::vector<int> >::iterator it;
        for (it = mask_indices.begin(); it != mask_indices.end(); ++it) {
          std::vector<uint32_t> neighbour_ptrs;
          for (size_t dim = 0; dim < mask.ndim(); dim++)
            mask_neigh[dim] = (*it)[dim];

          for (size_t dim = 0; dim < mask.ndim(); dim++) {
//            std::cout << "Dimension " << dim << std::endl;
            if (!ignore_dim[dim]) {
              if (adjacency_matrices[dim].is_set()) {
                for (int i = 0; i < mask.dim(dim); i++) {  // for each index along this dimension, check for neighbours
                  if (adjacency_matrices[dim]((*it)[dim], i)) {  // do we have a neighbour
                    mask_neigh[dim] = i;
                    if (mask_neigh.value() > 0.5) {
                      voxel_assign (neigh_mask_vox, mask_neigh);
                      neighbour_ptrs.push_back (neigh_mask_vox.value()); // store the index to the neighbour
                    }
                  }
                }
              // we treat this dimension as having normal contiguous neighbours
              } else {
                if ((*it)[dim] - 1 > 0) { // boundary condition
                  mask_neigh[dim] = (*it)[dim] - 1;
                  if (mask_neigh.value() > 0.5) {
                    voxel_assign (neigh_mask_vox, mask_neigh);
                    neighbour_ptrs.push_back (neigh_mask_vox.value()); // store the index to the neighbour
                  }
                }
                if ((*it)[dim] + 1 < mask.dim (dim) - 1) { // boundary condition
                  mask_neigh[dim] = (*it)[dim] + 1;
                  if (mask_neigh.value() > 0.5) {
                    voxel_assign (neigh_mask_vox, mask_neigh);
//                    std::cout << neigh_mask_vox << std::endl;
                    neighbour_ptrs.push_back (neigh_mask_vox.value()); //  store the index to the neighbour
                  }
                }
              }
            }
            mask_neigh[dim] = (*it)[dim];
          }
//          std::cout << "position " << (*it) << std::endl;
//          std::cout << neighbour_ptrs << std::endl;
          adjacent_indices.push_back (neighbour_ptrs);
        }
      }


      void agglomerate (int index,
                        std::vector<std::vector<uint32_t> > & adjacent_indices,
                        std::vector<uint32_t> & traversed,
                        uint32_t current_label,
                        uint32_t & counter) {
//        std::cout << "index "  << index << std::endl;
//        std::cout << adjacent_indices[index]  << std::endl;
//        std::cout << "label " << current_label  << std::endl;
//        std::cout << "counter " << counter  << std::endl;
        counter++;
        traversed[index] = current_label;
        for (size_t n = 0; n < adjacent_indices[index].size(); n++) {
          if (!traversed[adjacent_indices[index][n]])
            agglomerate (adjacent_indices[index][n], adjacent_indices, traversed, current_label, counter);
        }
      }


      void agglomerate (int index,
                        std::vector<std::vector<uint32_t> > & adjacent_indices,
                        std::vector<uint32_t> & traversed,
                        uint32_t current_label,
                        uint32_t & counter,
                        std::vector<float> image_values,
                        float threshold) {
        counter++;
        traversed[index] = current_label;
        for (size_t n = 0; n < adjacent_indices[index].size(); n++) {
          if (image_values[adjacent_indices[index][n]] > threshold)
            if (!traversed[adjacent_indices[index][n]])
              agglomerate (adjacent_indices[index][n], adjacent_indices, traversed, current_label, counter);
        }
      }



      /** \addtogroup Filters
      @{ */

      /*! Label all connected components within a binary mask of n-dimensions.
       *  This filter will label each component in order of increasing component size
       *
       * Unless otherwise specified this filter will assume that for all dimensions
       * voxels are contiguous (in space, time or whatever you feel like). Therefore by
       * default, neighbours are defined as having contiguous indices. Alternatively
       * an adjacency matrix (that is symmetric) can be used to define neighbouring indices.
       * For example this can be used for defining adjacent neighbours in the orientation domain
       * (if the 4th dimension relates to different directions). This could also be used
       * to perform connected components within each 3D volume of a 4D image by setting
       * a zero adjacency matrix for the 4th dimension.
       *
       * Typical usage:
       * \code
       * Image::BufferPreload<float> src_data (argument[0]);
       * Image::BufferPreload<float>::voxel_type src (src_data);
       * Image::Filter::ConnectedComponents filter (src);
       *
       * Image::Header header (src_data);
       * header.info() = filter.info();
       * header.datatype() = src_data.datatype();
       *
       * Image::Buffer<float> dest_data (argument[1], src_data);
       * Image::Buffer<float>::voxel_type dest (dest_data);
       *
       * filter (src, dest);
       *
       * \endcode
       */
      class ConnectedComponents : public ConstInfo
      {
        public:

        template <class InputVoxelType>
        ConnectedComponents (const InputVoxelType& in) :
        ConstInfo (in) {
          datatype_ = DataType::Int16;
          Math::Matrix<float> empty;
          adjacency_matrices_.resize(this->ndim(), empty);
          ignore_dim_.resize(this->ndim(), false);
        }

        template <class InputVoxelType, class OutputVoxelType>
        void operator() (InputVoxelType& in, OutputVoxelType& out) {

          std::vector<std::vector<int> > mask_indices;
          std::vector<std::vector<uint32_t> > adjacent_indices;
          compute_adjacency (in, adjacency_matrices_, ignore_dim_, mask_indices, adjacent_indices);
          std::cout << "number of voxels " << mask_indices.size() << std::endl;

          std::vector<uint32_t> traversed (mask_indices.size(), 0);  // keep track of the already labelled voxels

          uint32_t current_label = 1;
          std::vector<cluster> clusters;
          for (unsigned int i = 0; i < traversed.size(); i++) {
            if (!traversed[i]) {
              cluster c;
              c.label = current_label;
              c.size = 0;
              agglomerate(i, adjacent_indices, traversed, current_label, c.size);
              clusters.push_back(c);
              current_label++;
            }
          }

          std::sort (clusters.begin(), clusters.end(), compare_clusters);

//          for (unsigned int c = 0; c < clusters.size(); c++)
//            std::cout << clusters[c].size << " " << clusters[c].label << std::endl;


          std::vector<int> label_lookup (clusters.size(), 0);
          for (unsigned int c = 0; c < clusters.size(); c++)
            label_lookup[clusters[c].label -1] = c + 1;

//          for (unsigned int c = 0; c < label_lookup.size(); c++)
//            std::cout << label_lookup[c] << std::endl;

          Image::LoopInOrder loop(out);
          for (loop.start(out); loop.ok(); loop.next(out))
            out.value() = 0;

          for (unsigned int i = 0; i < traversed.size(); i++) {
            for (size_t dim = 0; dim < out.ndim(); dim++)
              out[dim] = mask_indices[i][dim];
            out.value() = label_lookup[traversed[i] - 1];
          }
          std::cout << "number of clusters " << clusters.size() << std::endl;
        }

        void set_ignore_dim (bool ignore, size_t dim) {
          ignore_dim_[dim] = ignore;
        }

        void set_adjacency_matrix(Math::Matrix<float> & adj_matrix, size_t dim) {
          if (dim > this->ndim())
            throw Exception("The dimensions specified is larger than the number of input dimensions.");
          if ((int)adj_matrix.columns() != this->dim(dim) || (int)adj_matrix.rows() != this->dim(dim))
            throw Exception("The input adjacency matrix size does not match the number of elements in this dimension");

          adjacency_matrices_[dim] = adj_matrix;
        }

        protected:
          std::vector<Math::Matrix<float> > adjacency_matrices_;
          std::vector<bool> ignore_dim_;

      };
      //! @}
    }
  }
}


#endif
