/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by Maximilian Pietsch 2015

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

#ifndef __image_average_space_h__
#define __image_average_space_h__

#include "transform.h"
#include <unsupported/Eigen/MatrixFunctions>
#include <Eigen/SVD>
#include <Eigen/Geometry> // Eigen::Translation


namespace MR 
{  
   namespace Math
   {
      template<class MatrixArrayType, class MatrixType>
      void matrix_average(MatrixArrayType const &mat_in, MatrixType & mat_avg, bool verbose = false){
        size_t rows = mat_in[0].rows();
        size_t cols = mat_in[0].cols();
        size_t N = mat_in.size();
        // check input
        for (const auto &mat: mat_in){
          assert( cols == (size_t) mat.cols());
          assert( rows == (size_t) mat.rows());
          cols = mat.cols(); // TODO hack to remove unused variable compiler warning
        }
        
        mat_avg.setIdentity();

        MatrixType mat_s(rows,cols);  // sum 
        MatrixType mat_l(rows,cols);  
        for (size_t i = 0; i<1000; ++i){
          mat_s.setZero(rows,cols);
          Eigen::ColPivHouseholderQR<MatrixType> dec(mat_avg); // QR decomposition with column pivoting
          for (size_t j=0; j<N; ++j){
            mat_l = dec.solve(mat_in[j]); // solve mat_avg * mat_l = mat_in[j] for mat_l
            // std::cout << "mat_avg*mat_l - mat_in[j]:\n" << mat_avg*mat_l - mat_in[j] <<std::endl;
            mat_s += mat_l.log().real();
          }
          mat_s *= 1./N;
          mat_avg *= mat_s.exp();
          if (verbose) std::cerr << i << " mat_s.squaredNorm(): " <<  mat_s.squaredNorm() <<std::endl;
          if (mat_s.squaredNorm() < 1E-20){
            break;
          }
        }
      }  
   }
}

namespace MR 
{ 

   template<class ComputeType>
   Eigen::Matrix<ComputeType, 8, 4> get_bounding_box(const Eigen::Matrix<ComputeType, 4, 1>& width){
    if (width.size() != 4)
      throw Exception("get_bounding_box: width has to be 4-dimensional");
    Eigen::Matrix<ComputeType, 8, 4>  corners;
    // MatrixType faces = MatrixType(8,4);
    // faces << 1, 2, 3, 4,   2, 6, 7, 3,   4, 3, 7, 8,   1, 5, 8, 4,   1, 2, 6, 5,   5, 6, 7, 8;
    corners << 0.0, 0.0, 0.0, 1.0,
               0.0, 1.0, 0.0, 1.0,
               1.0, 1.0, 0.0, 1.0, 
               1.0, 0.0, 0.0, 1.0, 
               0.0, 0.0, 1.0, 1.0, 
               0.0, 1.0, 1.0, 1.0, 
               1.0, 1.0, 1.0, 1.0, 
               1.0, 0.0, 1.0, 1.0;
    corners *= width.asDiagonal();  
    return corners;
  }

  template<class ComputeType, class TransformType>
  Eigen::Matrix<ComputeType, 8, 4> get_bounding_box(const Eigen::Matrix<ComputeType, 4, 1>& width, const TransformType& transformation){
    // get image boundary box corners in voxels
    Eigen::Matrix<ComputeType, 8, 4> corners = get_bounding_box<ComputeType>(width);
    // transform voxels into scanner coordinate system
    for (int i=0; i<corners.rows();i++){
      corners.transpose().col(i) = (transformation * corners.transpose().col(i));
    }
    return corners;
  }

  template<class ComputeType, class TransformType>
  Eigen::Matrix<ComputeType, 8, 4> get_bounding_box(const Header& header, const TransformType& voxel2scanner){
    if (header.ndim() < 3)
      throw Exception("get_bounding_box: image dimension has to be >= 3");
    Eigen::Matrix<ComputeType, 4, 1> width = Eigen::Matrix<ComputeType, 4, 1>::Ones(4);
    // width in voxels
    for (size_t i=0; i<3; i++){
      width(i) = header.size(i) - 1.0;
    }
    // get image boundary box corners in voxels
    Eigen::Matrix<ComputeType, 8, 4>  corners = get_bounding_box<ComputeType, TransformType>(width, voxel2scanner);
    return corners;
  }

   template<class ComputeType, class TransformType>
   Header  compute_minimum_average_header(
      std::vector<Header> input_headers, 
      ComputeType voxel_subsampling, 
      Eigen::Matrix<ComputeType, 4, 1>  padding,
      std::vector<TransformType>& transform_header_with){
      // typedef Eigen::Transform< ComputeType, 3, Eigen::Projective> TransformType;
      typedef Eigen::Matrix<ComputeType, Eigen::Dynamic, Eigen::Dynamic> MatrixType;
      typedef Eigen::Matrix<ComputeType, Eigen::Dynamic, 1> VectorType;
      typedef Eigen::Matrix<ComputeType, 4, 4> MatrixType4;
      typedef std::vector<MatrixType> MatrixArrayType;

      size_t num_images = input_headers.size();
      MatrixArrayType transformation_matrices;
      MatrixType bounding_box_corners = MatrixType::Zero(8*num_images,4);
      for (size_t iFile=0; iFile<num_images; iFile++) {
         // Image::ConstHeader header_in  (*input_headers[iFile]);

         // get voxel2scanner transformation
         auto v2s_trafo = (Eigen::Transform< ComputeType, 3, Eigen::Projective>) Transform(input_headers[iFile]).voxel2scanner; // cast to Projective trafo to fill the last row of the matrix
         // v2s_trafo.makeAffine(); // Sets the last row to [0 ... 0 1] 

         if (transform_header_with.size()>iFile)
            v2s_trafo = transform_header_with[iFile] * v2s_trafo;
         transformation_matrices.push_back(v2s_trafo.matrix());

         bounding_box_corners.template block<8,4>(iFile*8,0) = get_bounding_box<ComputeType, decltype(v2s_trafo)>(input_headers[iFile], v2s_trafo);
      }  
 
      // create average space header
      Header header_out (input_headers[0]);
      header_out.set_ndim(3); // TODO
      header_out.datatype() = DataType::Float32; 


      auto vox_scaling = Eigen::Matrix<ComputeType, Eigen::Dynamic, 1>(4);
      // set header voxel size: smallest voxel spacing in any template * voxel_subsampling
      for (size_t i=0;i<3;i++){
         for (size_t iFile=0; iFile<num_images; iFile++) {
            // for (auto in : input_headers){
               header_out.spacing(i) = std::min(double(header_out.spacing(i)), input_headers[iFile].spacing(i) * voxel_subsampling);
         }
         vox_scaling(i) = 1.0/header_out.spacing(i);
      }
      vox_scaling(3) = 1.0;
      DEBUG("vox_scaling: "+ str(vox_scaling.transpose(),10));

      // MatrixType4 mat_avg;
      MatrixType4 mat_avg = MatrixType4::Zero(4,4);
      Math::matrix_average<MatrixArrayType, MatrixType4>(transformation_matrices, mat_avg, false);
      DEBUG("mat_avg: " + str(mat_avg,10));

      auto average_v2s_trafo = Eigen::Transform< ComputeType, 3, Eigen::Projective>::Identity();
      average_v2s_trafo.matrix() = mat_avg;

      Eigen::Transform< ComputeType, 3, Eigen::Projective> average_s2v_trafo = average_v2s_trafo.inverse(Eigen::Projective);

      DEBUG("inverse sanity: " + str( (average_v2s_trafo * average_s2v_trafo).matrix().isApprox(MatrixType::Identity(4,4)) ));

      // transform all image corners into inverse average space
      MatrixType bounding_box_corners_inv = bounding_box_corners;
      for (int i=0; i<bounding_box_corners_inv.rows(); i++)
          bounding_box_corners_inv.transpose().col(i) = (average_s2v_trafo * bounding_box_corners.transpose().col(i));
      // minimum axis-aligned corners in inverse average space 
      VectorType bounding_box_corners_inv_min = bounding_box_corners_inv.colwise().minCoeff();
      VectorType bounding_box_corners_inv_max = bounding_box_corners_inv.colwise().maxCoeff();
      VectorType bounding_box_corners_inv_width = bounding_box_corners_inv_max - bounding_box_corners_inv_min;
      bounding_box_corners_inv_width += 2.0 * padding;
      bounding_box_corners_inv_width(3) = 1.0;

      bounding_box_corners = get_bounding_box<ComputeType>(bounding_box_corners_inv_width);

      // transform boundary box corners back into scanner space
      for (int i=0; i<bounding_box_corners.rows();i++){
        bounding_box_corners.row(i) += bounding_box_corners_inv_min - padding; 
        bounding_box_corners.row(i)(3) = 1.0;
        bounding_box_corners.row(i) = ( average_v2s_trafo * bounding_box_corners.row(i).transpose()).transpose();
      } 

      // std::cout << "average space axis-aligned minimum bounding box:\n" << bounding_box_corners << std::endl;

      VectorType bounding_box_corners_width = bounding_box_corners.colwise().maxCoeff() - bounding_box_corners.colwise().minCoeff();
      bounding_box_corners_width(3) = 1.0;

      // set translation to first corner (0, 0, 0, 1)
      average_v2s_trafo.matrix().col(3).template head<3>() = bounding_box_corners.row(0).template  head<3>();
      // average_s2v_trafo = average_v2s_trafo.inverse();

      // override header transformation
      Eigen::Transform< ComputeType, 3, Eigen::Projective> average_i2s_trafo = TransformType::Identity();
      average_i2s_trafo.matrix() = average_v2s_trafo.matrix();
      average_i2s_trafo.matrix() *= vox_scaling.asDiagonal();
      header_out.transform().matrix().block(0,0,3,4) = average_i2s_trafo.matrix().block(0,0,3,4);

      // get scanner 2 voxel trafo
      auto average_s2v = (Eigen::Transform< ComputeType, 3, Eigen::Projective>) Transform(header_out).scanner2voxel;

      // set header dimensions according to element 7 in bounding_box_corners:
      VectorType ext = VectorType(4);
      ext = bounding_box_corners.row(6);
      // VAR(average_s2v.matrix());
      // VAR(bounding_box_corners);
      ext(3) = 1.0;
      ext = average_s2v * ext;
      // VAR(ext);
      for (size_t i=0;i<3;i++){
        header_out.size(i) = std::ceil(ext(i)) ; 
      }    
      // return header_out;
      // auto scratch = Header::scratch (input_headers[0], "my buffer").get_image<uint32_t>();
      // std::cout << "average voxel to scanner transformation:\n" << average_v2s_trafo.matrix() <<std::endl;
      // std::cout << "average image to scanner transformation:\n" << average_i2s_trafo.matrix() <<std::endl;
      return header_out;
      }
}
#endif